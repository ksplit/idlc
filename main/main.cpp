#include <iostream>
#include <string>
#include <sstream>
#include <fstream>

#include "node_map.h"
#include "dump.h"
#include "visit.h"
#include "generic_pass.h"
#include "rpc_import.h"
#include "marshaling.h"
#include "../parser/parser.h"

namespace fs = std::filesystem;

namespace idlc {
	class verify_kernel_idl_pass : public generic_pass<verify_kernel_idl_pass> {
	public:
		void visit_require(const require&)
		{
			std::cout << "[error] It is illegal to import a module in kernel-side modules\n";
			throw std::exception {};
		}

		void visit_include(const include&)
		{
			std::cout << "[warning] IDL inclusion in kernel-side IDL is presently unsupported\n";
		}
	};

	class type_collection_pass : public generic_pass<type_collection_pass> {
	public:
		void visit_module(module& module) noexcept
		{
			m_scope = module.identifier();
			m_types = &module.types;
		}

		void visit_projection(const projection& projection)
		{
			if (!m_types->insert(projection)) {
				std::cout << "[error] Encountered projection redefinition: " << m_scope << "::" << projection.identifier() << "\n";
				throw std::exception {};
			}
		}

	private:
		node_map<const projection>* m_types;
		gsl::czstring<> m_scope;
	};

	// TODO: would be nice to have an error context
	// NOTE: I don't think exceptions are terribly appropriate for this

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
		void visit_module(module& module) noexcept
		{
			m_module = module.identifier();
			m_types = &module.types;
		}

		// Should account for parent scope of RPC pointers

		void visit_projection(const projection& proj)
		{
			m_item = proj.identifier();
		}

		void visit_rpc(const rpc& rpc)
		{
			m_item = rpc.identifier();
		}

		void visit_projection_type(projection_type& proj)
		{
			const projection* def {m_types->get(proj.identifier())};
			if (def) {
				proj.definition(def);
			}
			else {
				std::cout << "[error] Could not resolve projection: " << proj.identifier() << "\n";
				std::cout << "[note] In: " << m_module << "::" << m_item <<  "\n";
				throw std::exception {};
			}
		}

	private:
		node_map<const projection>* m_types {};
		gsl::czstring<> m_module {};
		gsl::czstring<> m_item {};
	};

	class module_collection_pass : public generic_pass<module_collection_pass> {
	public:
		module_collection_pass(node_map<module>& modules) noexcept : m_modules {&modules}
		{
		}

		void visit_module(module& module)
		{
			if (!m_modules->insert(module)) {
				std::cout << "[error] Encountered module redefinition: " << module.identifier() << "\n";
				throw std::exception {};
			}
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

		void visit_include(include& include)
		{
			const fs::path path {m_relative_to / include.path()};
			if (!fs::exists(path)) {
				std::cout << "[error] Could not find include " << include.path().generic_string() << "\n";
				throw std::exception {};
			}

			std::cout << "[info] Processing included file: " << path.generic_string() << "\n";

			auto& file = *const_cast<idlc::file*>(
				static_cast<const idlc::file*>(
					Parser::parse(path.generic_string())));

			include.parsed_file.reset(&file);
			verify_kernel_idl_pass vki;
			type_collection_pass tc;
			type_resolve_pass tr;
			module_collection_pass mc {m_modules};
			visit(vki, file);
			visit(tc, file);
			visit(tr, file);
			visit(mc, file);
		}

	private:
		fs::path m_relative_to;
		node_map<module>& m_modules;
	};

	class verify_driver_idl_pass : public generic_pass<verify_driver_idl_pass> {
	public:
		void visit_rpc(const rpc& rpc)
		{
			std::cout << "[error] Rpc definition " << rpc.identifier() << " illegal in driver module\n";
			throw std::exception {};
		}

		void visit_projection(const projection& projection) {
			std::cout << "[error] Projection definition " << projection.identifier() << " illegal in driver module\n";
			throw std::exception {};
		}

		void visit_module(const module& module)
		{
			if (is_module_found) {
				std::cout << "[error] Driver definition IDL may only define the driver module\n";
				throw std::exception {};
			}

			is_module_found = true;
		}

	private:
		bool is_module_found {false};
	};
}


int main(int argc, gsl::czstring<>* argv) {
	const gsl::span args {argv, gsl::narrow_cast<std::size_t>(argc)};

	if (args.size() != 2 && args.size() != 3) {
		std::cout << "Usage: idlc <source-file> [<dump-file>]\n";
		return 0;
	}

	const fs::path idl_path {gsl::at(args, 1)};

	try {
		auto top_node = std::unique_ptr<idlc::file> {
			const_cast<idlc::file*>(
				static_cast<const idlc::file*>(
					Parser::parse(idl_path.generic_string())))};

		auto& file = *top_node;

		std::cout << "[info] Verified IDL syntax\n";

		if (args.size() == 3) {
			std::ofstream dump {gsl::at(args, 2)};
			std::cout << "[info] Doing AST dump\n";
			idlc::dump(file, dump);
		}

		idlc::verify_driver_idl_pass vdi_pass;
		visit(vdi_pass, file);

		idlc::node_map<idlc::module> imports;
		idlc::include_file_pass if_pass {idl_path.parent_path(), imports};
		visit(if_pass, file);

		std::vector<idlc::marshal_unit> rpcs;
		std::vector<idlc::marshal_unit> rpc_pointers;
		idlc::rpc_import_pass mi_pass {rpcs, rpc_pointers, imports};
		visit(mi_pass, file);

		for (const auto& unit : rpcs) {
			std::cout << "[info] RPC marshal unit " << unit.identifier << "\n";
		}

		for (const auto& unit : rpc_pointers) {
			std::cout << "[info] RPC pointer marshal unit " << unit.identifier << "\n";
		}

		return 0;
	}
	catch (const Parser::ParseException& e) {
		std::cerr << "[error] Parsing failed\n" << std::endl;
		std::cerr << e.getReason() << std::endl;
		return 1;
	}
	catch (const std::exception&) {
		std::cout << "[error] Compilation failed\n";
		return 1;
	}
}
