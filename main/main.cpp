#include <iostream>
#include <string>
#include <sstream>
#include <fstream>

#include "node_map.h"
#include "dump.h"
#include "visit.h"
#include "generic_pass.h"
#include "rpc_import.h"
#include "log.h"
#include "../backend/marshaling.h"
#include "../backend/code_generation.h"
#include "../parser/parser.h"

namespace fs = std::filesystem;

/*
	BIG TASKS LEFT
		- Write in support for shadow copies
			- DONE bind(caller/callee) appears to be working for arguments/subfields and return values
			- DONE then alloc / dealloc (much simpler, hopefully)
			- The logical structure seems to have solidified there
			- Note: If we had a passing tree, it's be possible to add default
			annotations more intelligently (instead of the weird need to
			always specify in or out for subfields with bind)
			- this is honestly the final nail in the coffin
		- Marshaling is too big of a mess presently to estimate
		work needed to support unions / arrays. Grammar can still pull it off, though.
			- Issue is that union vars are a new kind of marshaling, and thanks to the
			poor choices in marshaling.cpp, that could result in as much as *double* the code,
			making it even *harder* to work with.
			- So I legitimately believe we should proceed with the move to marshal passing trees
			before even considering unions / arrays / etc.
		- Finish code generation
			- DONE function trampolines (mostly done, need to add the custom sections and inject_trampoline)
			- DONE command -> code translation
			- DONE user hooks (to provide declarations, etc.)
			- much simpler (code generator is a *joy* to work in, compared to marshaler)
*/

/*
	POST DEADLINE TODOs (Beside the ones that have sat, ignored, for the past four-and-a-half weeks)
		- Re-write marshaling in terms of passes over a "passing tree"
		- The marshaling code currently walks the AST in a very specific, very odd way
		(following type definitions), and is often force to recompute defaulted values;
		it's also possible that there are ways to memoize marshaling work that just aren't terribly visible
		- There is definitely extensive code duplication, not all of which is obvious
		- The essence of the marshaling algorithm is clouded by the traversal method,
		and the absence of a good data structure makes memory management somewhat opaque
		- std::string_view was a bad idea, const std::string& would've made the ownership clearer
		(I think it's all owned by the op buffer)
		- Some of these "strings" need to be strongly typed, and the aggregate initialization makes the code brittle
		(not always obvious when something breaks)
		- I cannot emphasize enough how much this code needs a better data structure.
		The IR buffers are quite clear in their meaning, making code generation somewhat trivial.
		But the actual marshaling analysis is cloudy, and doesn't happen at syntax-level.
		- More brittleness for the eyes: naming conventions are implied, with the various name variants
		(trampoline, rpc id, rpc ptr id, etc.) being recomputed. Sure, you can Ctrl-F, but it doesn't have to be this way
		And it really shouldn't
		- "Strongly typed C template strings" - do you mean C syntax nodes, David? It's not too late to add an LLVM dependency...
		- I see two major refactoring points in idlc's future: moving to a C syntax tree representation,
		and extracting passing trees from the AST
*/

namespace idlc {
	class verify_kernel_idl_pass : public generic_pass<verify_kernel_idl_pass> {
	public:
		bool visit_require(const require&)
		{
			log_error("It is illegal to import a module in kernel-side modules");
			return false;
		}

		bool visit_include(const include&)
		{
			log_warning("IDL inclusion in kernel-side IDL is currently unsupported");
			log_warning("Include will be ignored");
			return true;
		}

		bool visit_type(const type& ty)
		{
			if (ty.stars() > 1) {
				// When I say undefined, I mean it
				// Generated code may *even* fail to build
				// Unlikely, but I won't actively try to prevent it
				log_warning("Multi-level pointer types are unsupported");
				log_warning("\tMarshaling semantics are undefined");
				log_warning("\tIn module: ", m_module, ", context: ", m_container, ", field: ", m_field);
			}

			if (ty.stars() && ty.get_copy_type() && ty.get_copy_type()->kind() == copy_type_kind::primitive) {
				log_debug(m_field, " is a non-passing pointer");
				log_debug("\tIn module: ", m_module, ", context: ", m_container);
			}

			if (!ty.stars() && ty.get_copy_type() && ty.get_copy_type()->kind() == copy_type_kind::projection) {
				// Have not decided how these will be handled by default
				log_warning("Pass-by-value projections are unsupported");
				log_warning("\tMarshaling semantics are undefined");
				log_warning("\tIn module: ", m_module, ", context: ", m_container, ", field: ", m_field);
			}

			return true;
		}

		bool visit_var_field(const var_field& var)
		{
			m_field = var.identifier();
			return true;
		}

		bool visit_projection(const projection& proj)
		{
			m_container = proj.identifier();
			return true;
		}

		bool visit_rpc(const rpc& rpc)
		{
			m_container = rpc.identifier();
			return true;
		}

		bool visit_module(const module& mod)
		{
			m_module = mod.identifier();
			return true;
		}

	private:
		gsl::czstring<> m_module {};
		gsl::czstring<> m_container {};
		gsl::czstring<> m_field {};
	};

	class type_collection_pass : public generic_pass<type_collection_pass> {
	public:
		bool visit_module(module& module) noexcept
		{
			m_scope = module.identifier();
			m_types = &module.types;
			return true;
		}

		bool visit_projection(projection& projection)
		{
			if (!m_types->insert(projection)) {
				log_error("Encountered projection redefinition: ", m_scope, "::", projection.identifier());
				return false;
			}

			projection.parent_module = m_scope;

			return true;
		}

	private:
		node_map<projection>* m_types;
		gsl::czstring<> m_scope;
	};

	// TODO: would be nice to have an error context

	// TODO: there is a subtle bug here
	// If a projection contains itself, even indirectly, it will still parse and resolve,
	// even though it will likely crash the marshaling code by triggerring some form of infinite looping
	// It is not clear how to resolve this without extensive dependency tracking
	// Ironically, this issue does not exist in C++, since the order-dependency of type declarations
	// makes it impossible to circularly define a type in this way
	// This is not very high-priority, since the situation is theoretically
	// impossible in machine-generated IDL
	// A possible band-aid fix is just to ban by-value projections
	class type_resolve_pass : public generic_pass<type_resolve_pass> {
	public:
		bool visit_module(module& module) noexcept
		{
			m_module = module.identifier();
			m_types = &module.types;
			return true;
		}

		// Should account for parent scope of RPC pointers

		bool visit_projection(const projection& proj)
		{
			m_item = proj.identifier();
			return true;
		}

		bool visit_rpc(const rpc& rpc)
		{
			m_item = rpc.identifier();
			return true;
		}

		bool visit_projection_type(projection_type& proj)
		{
			projection* const def {m_types->get(proj.identifier())};
			if (def) {
				proj.definition(def);
			}
			else {
				log_error("Could not resolve projection: ", proj.identifier());
				log_note("In: ", m_module, "::", m_item);
				return false;
			}

			return true;
		}

	private:
		node_map<projection>* m_types {};
		gsl::czstring<> m_module {};
		gsl::czstring<> m_item {};
	};

	class module_collection_pass : public generic_pass<module_collection_pass> {
	public:
		module_collection_pass(node_map<module>& modules) noexcept : m_modules {&modules}
		{
		}

		bool visit_module(module& module)
		{
			if (!m_modules->insert(module)) {
				log_error("Encountered module redefinition: ", module.identifier());
				return false;
			}

			return true;
		}

	private:
		node_map<module>* m_modules;
	};

	class include_file_pass : public generic_pass<include_file_pass> {
	public:
		include_file_pass(const fs::path& relative_to, node_map<module>& modules) :
			m_relative_to {relative_to},
			m_modules {modules}
		{
		}

		bool visit_include(include& include)
		{
			const fs::path path {m_relative_to / include.path()};
			if (!fs::exists(path)) {
				log_error("Could not find include ", include.path().generic_string(), "\n");
				return false;
			}

			log_note("Processing included file: ", path.generic_string());

			auto& file = *const_cast<idlc::file*>(
				static_cast<const idlc::file*>(
					Parser::parse(path.generic_string())));

			include.parsed_file.reset(&file);
			verify_kernel_idl_pass vki;
			type_collection_pass tc;
			type_resolve_pass tr;
			module_collection_pass mc {m_modules};
			if (!visit(vki, file)) {
				return false;
			}

			if (!visit(tc, file)) {
				return false;
			}

			if (!visit(tr, file)) {
				return false;
			}

			return visit(mc, file);
		}

	private:
		fs::path m_relative_to;
		node_map<module>& m_modules;
	};

	class verify_driver_idl_pass : public generic_pass<verify_driver_idl_pass> {
	public:
		bool visit_rpc(const rpc& rpc)
		{
			log_error("Rpc definition ", rpc.identifier(), " illegal in driver module");
			return false;
		}

		bool visit_projection(const projection& projection) {
			log_error("Projection definition ", projection.identifier(), " illegal in driver module");
			return false;
		}

		bool visit_module(const module& module)
		{
			if (m_driver_name) {
				log_error("Driver definition IDL may only define the driver module");
				return false;
			}

			m_driver_name = module.identifier();

			return true;
		}

		gsl::czstring<> get_driver_name()
		{
			return m_driver_name;
		}

	private:
		gsl::czstring<> m_driver_name {};
	};

	struct rpc_imports {
		gsl::czstring<> driver_name;
		std::vector<idlc::marshal_unit> rpcs;
		std::vector<idlc::marshal_unit> rpc_pointers;
	};

	std::optional<rpc_imports> import_rpcs(file& file, const std::filesystem::path& idl_path)
	{
		idlc::verify_driver_idl_pass vdi_pass;
		if (!visit(vdi_pass, file)) {
			return std::nullopt;
		}

		idlc::node_map<idlc::module> imports;
		idlc::include_file_pass if_pass {idl_path.parent_path(), imports};
		if (!visit(if_pass, file)) {
			return std::nullopt;
		}

		std::vector<idlc::marshal_unit> rpcs;
		std::vector<idlc::marshal_unit> rpc_pointers;
		idlc::rpc_import_pass mi_pass {rpcs, rpc_pointers, imports};
		if (!visit(mi_pass, file)) {
			return std::nullopt;
		}

		return rpc_imports {vdi_pass.get_driver_name(), rpcs, rpc_pointers};
	}
}


int main(int argc, gsl::czstring<>* argv) {
	const gsl::span args {argv, gsl::narrow_cast<std::size_t>(argc)};

	if (args.size() != 3) {
		std::cout << "Usage: idlc <source-file> <destination-directory>\n";
		return 0;
	}

	const fs::path idl_path {gsl::at(args, 1)};
	const fs::path destination_path {gsl::at(args, 2)};

	try {
		std::unique_ptr<idlc::file> top_node {
			const_cast<idlc::file*>(
				static_cast<const idlc::file*>(
					Parser::parse(idl_path.generic_string())))};

		auto& file = *top_node;
		const auto imports_opt {idlc::import_rpcs(file, idl_path)};

		idlc::log_note("Verified IDL syntax");

		if (!imports_opt) {
			idlc::log_error("Compilation failed");
			return 1;
		}

		auto& [driver, rpcs, rpc_pointers] = *imports_opt;

		std::vector<idlc::marshal_unit_lists> rpc_lists;
		if (!idlc::process_marshal_units(rpcs, idlc::marshal_unit_kind::direct, rpc_lists)) {
			idlc::log_error("Compilation failed");
			return 1;
		}

		std::vector<idlc::marshal_unit_lists> rpc_ptr_lists;
		if (!idlc::process_marshal_units(rpc_pointers, idlc::marshal_unit_kind::indirect, rpc_ptr_lists)) {
			idlc::log_error("Compilation failed");
			return 1;
		}

		idlc::do_code_generation(driver, destination_path, rpc_lists, rpc_ptr_lists);
	}
	catch (const Parser::ParseException& e) {
		idlc::log_error("Parsing failed");
		std::cout << e.getReason();
		return 1;
	}

	return 0;
}
