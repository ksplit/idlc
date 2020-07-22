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
#include "log.h"
#include "../parser/parser.h"

namespace fs = std::filesystem;

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
			if (is_module_found) {
				log_error("Driver definition IDL may only define the driver module");
				return false;
			}

			is_module_found = true;

			return true;
		}

	private:
		bool is_module_found {false};
	};

	struct rpc_imports {
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

		return rpc_imports {rpcs, rpc_pointers};
	}

	void write_marshal_ops(
		std::ofstream& file,
		marshal_op_list& ops,
		gsl::czstring<> host_id,
		unsigned int indent_level
	)
	{
		unsigned int next_var {0};

		file << "\tunsigned int slot = 0;\n";

		while (!ops.finished()) {
			const marshal_op op {ops.get_next_op()};
			switch (op) {
			case marshal_op::unmarshal: {
				const unmarshal_data& data {ops.get_next_unmarshal()};
				tab_over(indent_level, file) << data.type << " var_" << next_var++ << " = *(" << data.type << "*)&message->slots[slot++];\n";
				break;
			}

			case marshal_op::marshal: {
				const marshal_data& data {ops.get_next_marshal()};
				tab_over(indent_level, file) << "message->slots[slot++] = *(uint64_t*)&var_" << data.source << ";\n";
				break;
			}

			case marshal_op::call: {
				const call_data& data {ops.get_next_call()};
				if (data.return_type == "void") {
					tab_over(indent_level, file) << host_id << "(" << data.arguments << "); slot = 0;\n";
				}
				else {
					tab_over(indent_level, file) << data.return_type << " var_return_value = " << host_id << "(" << data.arguments << "); slot = 0;\n";
				}
				break;
			}

			case marshal_op::inject_trampoline: {
				const inject_trampoline_data& data {ops.get_next_inject_trampoline()};
				tab_over(indent_level, file) << "void* var_" << next_var++ << " = INJECT_TRAMPOLINE(" << data.rpc_id << ", var_" << data.fptr << ");\n";
				break;
			}

			case marshal_op::get_return_value: {
				const get_return_value_data& data {ops.get_next_get_return_value()};
				tab_over(indent_level, file) << data.type << " var_" << next_var++ << " = var_return_value;\n";
				break;
			}

			case marshal_op::parameter: {
				const parameter_data& data {ops.get_next_parameter()};
				tab_over(indent_level, file) << data.type << " " << data.argument << " = var_" << data.source << ";\n";
				break;
			}

			case marshal_op::if_not_null: {
				const if_not_null_data& data {ops.get_next_if_not_null()};
				tab_over(indent_level, file) << "if (var_" << data.pointer << " != NULL) {\n";
				++indent_level;
				break;
			}

			case marshal_op::end_if_not_null: {
				--indent_level;
				tab_over(indent_level, file) << "}\n";
				break;
			}

			case marshal_op::set: {
				const set_data& data {ops.get_next_set()};
				tab_over(indent_level, file) << "var_" << data.parent << "->" << data.child_field << " = var_" << data.source << ";\n";
				break;
			}

			case marshal_op::get: {
				const get_data& data {ops.get_next_get()};
				tab_over(indent_level, file) << data.type << " var_" << next_var++ << " = var_" << data.parent << "->" << data.child_field << ";\n";
				break;
			}

			default:
				tab_over(indent_level, file) << "// TODO\n";
				break;
			}
		}
	}

	void do_code_generation(
		const std::filesystem::path& destination,
		gsl::span<marshal_unit_lists> rpc_lists,
		gsl::span<marshal_unit_lists> rpc_pointer_lists
	)
	{
		namespace fs = std::filesystem;

		if (!fs::exists(destination)) {
			fs::create_directories(destination);
		}

		// TODO: problem: we don't know which side the function pointers end up on
		// TODO: we'll just put rpc pointers on both sides, they don't have the same name conflicts as RPCs do (the func-named facade and the actual func)

		const fs::path kernel_dispatch_source_path {destination / "kernel_dispatch.c"}; // klcd
		const fs::path kernel_dispatch_header_path {destination / "kernel_dispatch.h"}; // klcd
		const fs::path driver_dispatch_source_path {destination / "driver_dispatch.c"}; // lcd
		const fs::path driver_dispatch_header_path {destination / "driver_dispatch.h"}; // lcd
		const fs::path common_header_path {destination / "common.h"}; // lcd

		std::ofstream common_header {common_header_path};
		common_header.exceptions(std::fstream::badbit | std::fstream::failbit);

		common_header << "#ifndef _COMMON_H_\n#define _COMMON_H_\n\n";
		common_header << "#include <stddef.h>\n";
		common_header << "#include <stdint.h>\n\n";
		common_header << "#define MAX_MESSAGE_SLOTS 64\n\n";
		common_header << "// TODO: implement trampoline injection\n";
		common_header << "#define INJECT_TRAMPOLINE(id, pointer) NULL\n\n";
		common_header << "enum dispatch_id {\n";

		for (const marshal_unit_lists& unit : rpc_lists) {
			common_header << "\trpc_" << unit.identifier << ",\n";
		}

		for (const marshal_unit_lists& unit : rpc_pointer_lists) {
			common_header << "\trpc_ptr" << unit.identifier << ",\n";
		}

		common_header << "};\n\n";
		common_header << "struct fipc_message {\n\tenum dispatch_id host_id;\n\tuint64_t slots[MAX_MESSAGE_SLOTS];\n};\n\n";
		common_header << "#endif";
		common_header.close();

		std::ofstream kernel_dispatch_source {kernel_dispatch_source_path};
		kernel_dispatch_source.exceptions(std::fstream::badbit | std::fstream::failbit);

		kernel_dispatch_source << "#include \"common.h\"\n\n";
		for (marshal_unit_lists& unit : rpc_lists) {
			kernel_dispatch_source << "void " << unit.identifier << "_callee(struct fipc_message* message) {\n";
			write_marshal_ops(kernel_dispatch_source, unit.callee_ops, unit.identifier, 1);
			kernel_dispatch_source << "}\n\n";
		}

		kernel_dispatch_source << "int dispatch(struct fipc_message* message) {\n";
		kernel_dispatch_source << "\tswitch (message->host_id) {\n";

		for (const marshal_unit_lists& unit : rpc_lists) {
			kernel_dispatch_source << "\tcase rpc_" << unit.identifier << ":\n";
			kernel_dispatch_source << "\t\t" << unit.identifier << "_callee(message)" << ";\n";
			kernel_dispatch_source << "\t\tbreak;\n\n";
		}

		kernel_dispatch_source << "\t}\n}\n\n";
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
		auto top_node = std::unique_ptr<idlc::file> {
			const_cast<idlc::file*>(
				static_cast<const idlc::file*>(
					Parser::parse(idl_path.generic_string())))};

		auto& file = *top_node;

		idlc::log_note("Verified IDL syntax");

		const std::optional<idlc::rpc_imports> imports_opt {idlc::import_rpcs(file, idl_path)};
		if (!imports_opt) {
			idlc::log_error("Compilation failed");
			return 1;
		}

		auto& [rpcs, rpc_pointers] = *imports_opt;

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

		idlc::do_code_generation(destination_path, rpc_lists, rpc_ptr_lists);

		return 0;
	}
	catch (const Parser::ParseException& e) {
		idlc::log_error("Parsing failed");
		std::cout << e.getReason();
		return 1;
	}
}
