#include "code_generation.h"

#include <fstream>
#include <vector>

#include "marshaling.h"
#include "../main/dump.h" // for tab_over()

namespace idlc {
	void write_marshal_ops(std::ofstream& file, const std::vector<marshal_op>& ops, unsigned int indent);

	void generate_kernel_dispatch_source(
		const std::filesystem::path& destination,
		gsl::span<marshal_unit_lists> rpc_lists,
		gsl::span<marshal_unit_lists> rpc_pointer_lists
	);

	void generate_driver_dispatch_source(
		const std::filesystem::path& destination,
		gsl::span<marshal_unit_lists> rpc_lists,
		gsl::span<marshal_unit_lists> rpc_pointer_lists
	);

	void do_code_generation(
		const std::filesystem::path& destination,
		gsl::span<marshal_unit_lists> rpc_lists,
		gsl::span<marshal_unit_lists> rpc_pointer_lists
	);
}

// Oversized, but clear

void idlc::write_marshal_ops(std::ofstream& file, const std::vector<marshal_op>& ops, unsigned int indent)
{
	for (const marshal_op op : ops) {
		switch (static_cast<marshal_op_kind>(op.index())) {
		case marshal_op_kind::marshal: {
			const auto args = std::get<marshal>(op);
			tab_over(indent, file) << "fipc_marshal(" << args.name << ");\n";
			break;
		}

		case marshal_op_kind::unmarshal: {
			const auto args = std::get<unmarshal>(op);
			tab_over(indent, file) << args.declaration << " = fipc_unmarshal(" << args.type << ");\n";
			break;
		}

		case marshal_op_kind::marshal_field: {
			const auto args = std::get<marshal_field>(op);
			tab_over(indent, file) << "fipc_marshal(" << args.parent << "->" << args.field << ");\n";
			break;
		}

		case marshal_op_kind::unmarshal_field: {
			const auto args = std::get<unmarshal_field>(op);
			tab_over(indent, file) << args.parent << "->" << args.field << " = fipc_unmarshal(" << args.type << ");\n";
			break;
		}

		case marshal_op_kind::block_if_not_null: {
			const auto args = std::get<block_if_not_null>(op);
			tab_over(indent, file) << "if (" << args.pointer << ") {\n";
			++indent;
			break;
		}

		case marshal_op_kind::end_block: {
			--indent;
			tab_over(indent, file) << "}\n";
			break;
		}

		case marshal_op_kind::load_field_indirect: {
			const auto args = std::get<load_field_indirect>(op);
			tab_over(indent, file) << args.declaration << " = " << args.parent << "->" << args.field << ";\n";
			break;
		}

		case marshal_op_kind::store_field_indirect: {
			const auto args = std::get<store_field_indirect>(op);
			tab_over(indent, file) << args.parent << "->" << args.field << " = " << args.name << ";\n";
			break;
		}

		case marshal_op_kind::inject_trampoline: {
			const auto args = std::get<inject_trampoline>(op);
			tab_over(indent, file) << args.declaration << " = inject_trampoline(" << args.mangled_name << ", " << args.pointer << ");\n";
			break;
		}

		case marshal_op_kind::call_direct: {
			const auto args = std::get<call_direct>(op);
			if (args.declaration.empty()) {
				tab_over(indent, file) << args.function << "(" << args.arguments_list << ");\n";
			}
			else {
				tab_over(indent, file) << args.declaration << " = " << args.function << "(" << args.arguments_list << ");\n";
			}

			tab_over(indent, file) << "marshal_slot = 0;\n";

			break;
		}

		case marshal_op_kind::get_local: {
			const auto args = std::get<get_local>(op);
			tab_over(indent, file) << args.local_declaration << " = fipc_get_local(" << args.remote_pointer << ");\n";
			break;
		}

		case marshal_op_kind::get_remote: {
			const auto args = std::get<get_remote>(op);
			tab_over(indent, file) << args.remote_declaration << " = fipc_get_remote(" << args.local_pointer << ");\n";
			break;
		}

		case marshal_op_kind::send_rpc: {
			const auto args = std::get<send_rpc>(op);
			tab_over(indent, file) << "fipc_send(" << args.rpc << ", message);\n";
			tab_over(indent, file) << "marshal_slot = 0;\n";
			break;
		}

		case marshal_op_kind::return_to_caller: {
			const auto args = std::get<return_to_caller>(op);
			tab_over(indent, file) << "return " << args.name << ";\n";
			break;
		}

		default:
			tab_over(indent, file) << "// TODO\n";
			break;
		}
	}
}

void idlc::generate_kernel_dispatch_source(
	const std::filesystem::path& destination,
	gsl::span<marshal_unit_lists> rpc_lists,
	gsl::span<marshal_unit_lists> rpc_pointer_lists
)
{
	std::ofstream kernel_dispatch_source {destination};
	kernel_dispatch_source.exceptions(std::fstream::badbit | std::fstream::failbit);

	kernel_dispatch_source << "#include \"common.h\"\n\n";
	for (marshal_unit_lists& unit : rpc_lists) {
		kernel_dispatch_source << "void " << unit.identifier << "_callee(struct fipc_message* message) {\n";
		kernel_dispatch_source << "\tunsigned int marshal_slot = 0;\n";
		write_marshal_ops(kernel_dispatch_source, unit.callee_ops, 1);
		kernel_dispatch_source << "}\n\n";
	}

	for (marshal_unit_lists& unit : rpc_pointer_lists) {
		kernel_dispatch_source << "void " << unit.identifier << "_callee(struct fipc_message* message) {\n";
		kernel_dispatch_source << "\tunsigned int marshal_slot = 0;\n";
		write_marshal_ops(kernel_dispatch_source, unit.callee_ops, 1);
		kernel_dispatch_source << "}\n\n";
	}

	for (marshal_unit_lists& unit : rpc_pointer_lists) {
		kernel_dispatch_source << unit.header << " {\n";
		kernel_dispatch_source << "\tunsigned int marshal_slot = 0;\n";
		kernel_dispatch_source << "\tstruct fipc_message message_buffer = {0};\n";
		kernel_dispatch_source << "\tstruct fipc_message* message = &message_buffer;\n";
		write_marshal_ops(kernel_dispatch_source, unit.caller_ops, 1);
		kernel_dispatch_source << "}\n\n";
	}

	kernel_dispatch_source << "int dispatch(struct fipc_message* message) {\n";
	kernel_dispatch_source << "\tswitch (message->host_id) {\n";

	for (const marshal_unit_lists& unit : rpc_lists) {
		kernel_dispatch_source << "\tcase rpc_" << unit.identifier << ":\n";
		kernel_dispatch_source << "\t\t" << unit.identifier << "_callee(message)" << ";\n";
		kernel_dispatch_source << "\t\tbreak;\n\n";
	}

	for (const marshal_unit_lists& unit : rpc_pointer_lists) {
		kernel_dispatch_source << "\tcase rpc_ptr" << unit.identifier << ":\n";
		kernel_dispatch_source << "\t\t" << unit.identifier << "_callee(message)" << ";\n";
		kernel_dispatch_source << "\t\tbreak;\n\n";
	}

	kernel_dispatch_source << "\t}\n}\n\n";
}

void idlc::generate_driver_dispatch_source(
	const std::filesystem::path& destination,
	gsl::span<marshal_unit_lists> rpc_lists,
	gsl::span<marshal_unit_lists> rpc_pointer_lists
)
{
	std::ofstream driver_dispatch_source {destination};
	driver_dispatch_source.exceptions(std::fstream::badbit | std::fstream::failbit);

	driver_dispatch_source << "#include \"common.h\"\n\n";
	for (marshal_unit_lists& unit : rpc_lists) {
		driver_dispatch_source << unit.header << " {\n";
		driver_dispatch_source << "\tunsigned int marshal_slot = 0;\n";
		driver_dispatch_source << "\tstruct fipc_message message_buffer = {0};\n";
		driver_dispatch_source << "\tstruct fipc_message* message = &message_buffer;\n";
		write_marshal_ops(driver_dispatch_source, unit.caller_ops, 1);
		driver_dispatch_source << "}\n\n";
	}

	for (marshal_unit_lists& unit : rpc_pointer_lists) {
		driver_dispatch_source << unit.header << " {\n";
		driver_dispatch_source << "\tunsigned int marshal_slot = 0;\n";
		driver_dispatch_source << "\tstruct fipc_message message_buffer = {0};\n";
		driver_dispatch_source << "\tstruct fipc_message* message = &message_buffer;\n";
		write_marshal_ops(driver_dispatch_source, unit.caller_ops, 1);
		driver_dispatch_source << "}\n\n";
	}

	for (marshal_unit_lists& unit : rpc_pointer_lists) {
		driver_dispatch_source << "void rpc_ptr" << unit.identifier << "_callee(struct fipc_message* message) {\n";
		driver_dispatch_source << "\tunsigned int marshal_slot = 0;\n";
		write_marshal_ops(driver_dispatch_source, unit.callee_ops, 1);
		driver_dispatch_source << "}\n\n";
	}
}

void idlc::do_code_generation(
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
	common_header << "#define fipc_marshal(value) message->slots[marshal_slot++] = *(uint64_t*)&value;\n";
	common_header << "#define fipc_unmarshal(type) *(type*)&message->slots[marshal_slot++];\n";
	common_header << "#define inject_trampoline(id, pointer) NULL\n\n";
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

	generate_kernel_dispatch_source(kernel_dispatch_source_path, rpc_lists, rpc_pointer_lists);
	generate_driver_dispatch_source(driver_dispatch_source_path, rpc_lists, rpc_pointer_lists);
}