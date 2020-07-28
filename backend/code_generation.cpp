#include "code_generation.h"

#include <fstream>
#include <vector>

#include "marshaling.h"
#include "../main/dump.h" // for tab_over()

// Here be tamer dragons
// It's not bad, it's just not as clean as I'd like it

namespace idlc {
	void write_marshal_ops(
		std::ofstream& file,
		const std::vector<marshal_op>& ops,
		unsigned int indent
	);

	void write_pointer_stubs(
		std::ofstream& file,
		gsl::span<marshal_unit_lists> rpc_pointer_lists
	);

	void generate_common_header(
		const std::filesystem::path& destination,
		std::string_view module_name,
		gsl::span<marshal_unit_lists> rpc_lists,
		gsl::span<marshal_unit_lists> rpc_ptr_lists
	);

	void generate_klcd_source(
		const std::filesystem::path& destination,
		gsl::span<marshal_unit_lists> rpc_lists,
		gsl::span<marshal_unit_lists> rpc_pointer_lists
	);

	void generate_lcd_source(
		const std::filesystem::path& destination,
		gsl::span<marshal_unit_lists> rpc_lists,
		gsl::span<marshal_unit_lists> rpc_pointer_lists
	);

	void do_code_generation(
		std::string_view driver_name,
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
			tab_over(file, indent) << "fipc_marshal(" << args.name << ");\n";
			break;
		}

		case marshal_op_kind::unmarshal: {
			const auto args = std::get<unmarshal>(op);
			tab_over(file, indent) << args.declaration << " = fipc_unmarshal(" << args.type << ");\n";
			break;
		}

		case marshal_op_kind::marshal_field: {
			const auto args = std::get<marshal_field>(op);
			tab_over(file, indent) << "fipc_marshal(" << args.parent << "->" << args.field << ");\n";
			break;
		}

		case marshal_op_kind::unmarshal_field: {
			const auto args = std::get<unmarshal_field>(op);
			tab_over(file, indent) << args.parent << "->" << args.field << " = fipc_unmarshal(" << args.type << ");\n";
			break;
		}

		case marshal_op_kind::block_if_not_null: {
			const auto args = std::get<block_if_not_null>(op);
			tab_over(file, indent) << "if (" << args.pointer << ") {\n";
			++indent;
			break;
		}

		case marshal_op_kind::end_block: {
			--indent;
			tab_over(file, indent) << "}\n";
			break;
		}

		case marshal_op_kind::load_field_indirect: {
			const auto args = std::get<load_field_indirect>(op);
			tab_over(file, indent) << args.declaration << " = " << args.parent << "->" << args.field << ";\n";
			break;
		}

		case marshal_op_kind::store_field_indirect: {
			const auto args = std::get<store_field_indirect>(op);
			tab_over(file, indent) << args.parent << "->" << args.field << " = " << args.name << ";\n";
			break;
		}

		case marshal_op_kind::inject_trampoline: {
			const auto args = std::get<inject_trampoline>(op);
			tab_over(file, indent) << args.declaration << " = inject_trampoline(" << args.mangled_name << ", " << args.pointer << ");\n";
			break;
		}

		case marshal_op_kind::call_direct: {
			const auto args = std::get<call_direct>(op);
			if (args.declaration.empty()) {
				tab_over(file, indent) << args.function << "(" << args.arguments_list << ");\n";
			}
			else {
				tab_over(file, indent) << args.declaration << " = " << args.function << "(" << args.arguments_list << ");\n";
			}

			tab_over(file, indent) << "marshal_slot = 0;\n";

			break;
		}

		case marshal_op_kind::call_indirect: {
			const auto args = std::get<call_indirect>(op);
			if (args.declaration.empty()) {
				tab_over(file, indent) << args.pointer << "(" << args.arguments_list << ");\n";
			}
			else {
				tab_over(file, indent) << args.declaration << " = " << args.pointer << "(" << args.arguments_list << ");\n";
			}

			tab_over(file, indent) << "marshal_slot = 0;\n";

			break;
		}

		case marshal_op_kind::create_shadow: {
			const auto args = std::get<create_shadow>(op);
			tab_over(file, indent) << args.declaration << " = fipc_create_shadow(" << args.remote_pointer << ");\n";
			break;
		}

		case marshal_op_kind::destroy_shadow: {
			const auto args = std::get<destroy_shadow>(op);
			tab_over(file, indent) << "fipc_destroy_shadow(" << args.remote_pointer << ");\n";
			break;
		}

		case marshal_op_kind::get_local: {
			const auto args = std::get<get_local>(op);
			tab_over(file, indent) << args.local_declaration << " = fipc_get_local(" << args.remote_pointer << ");\n";
			break;
		}

		case marshal_op_kind::get_remote: {
			const auto args = std::get<get_remote>(op);
			tab_over(file, indent) << args.remote_declaration << " = fipc_get_remote(" << args.local_pointer << ");\n";
			break;
		}

		case marshal_op_kind::send_rpc: {
			const auto args = std::get<send_rpc>(op);
			tab_over(file, indent) << "fipc_send(" << args.rpc << ", message);\n";
			tab_over(file, indent) << "marshal_slot = 0;\n";
			break;
		}

		case marshal_op_kind::return_to_caller: {
			const auto args = std::get<return_to_caller>(op);
			tab_over(file, indent) << "return " << args.name << ";\n";
			break;
		}

		default:
			tab_over(file, indent) << "// TODO\n";
			break;
		}
	}
}

void idlc::write_pointer_stubs(std::ofstream& file, gsl::span<marshal_unit_lists> rpc_pointer_lists)
{
	for (marshal_unit_lists& unit : rpc_pointer_lists) {
		file << "void " << unit.identifier << "_callee(struct fipc_message* message) {\n";
		file << "\tunsigned int marshal_slot = 0;\n";
		write_marshal_ops(file, unit.callee_ops, 1);
		file << "}\n\n";
	}

	for (marshal_unit_lists& unit : rpc_pointer_lists) {
		file << "LCD_TRAMPOLINE_DATA(" << unit.identifier << ")\n";
		file << "LCD_TRAMPOLINE_LINKAGE(" << unit.identifier << ")\n";
		file << unit.header << " {\n";
		file << "\tvoid* real_pointer;\n\tLCD_TRAMPOLINE_PROLOGUE(real_pointer, " << unit.identifier << ");\n";
		file << "\tunsigned int marshal_slot = 0;\n";
		file << "\tstruct fipc_message message_buffer = {0};\n";
		file << "\tstruct fipc_message* message = &message_buffer;\n";
		write_marshal_ops(file, unit.caller_ops, 1);
		file << "}\n\n";
	}
}

void idlc::generate_common_header(
	const std::filesystem::path& destination,
	std::string_view module_name,
	gsl::span<marshal_unit_lists> rpc_lists,
	gsl::span<marshal_unit_lists> rpc_ptr_lists
)
{
	std::ofstream common_header {destination};
	common_header.exceptions(std::fstream::badbit | std::fstream::failbit);

	common_header << "#ifndef _COMMON_H_\n#define _COMMON_H_\n\n";
	common_header << "#include <stddef.h>\n";
	common_header << "#include <string.h>\n";
	common_header << "#include \"trampoline.h\"\n";
	common_header << "#include <stdint.h>\n\n";

	common_header << "#include \"" << module_name << "_user.h\"\n\n";

	common_header << "#define MAX_MESSAGE_SLOTS 64\n\n";

	common_header << "#define fipc_marshal(value) message->slots[marshal_slot++] = *(uint64_t*)&value\n";
	common_header << "#define fipc_unmarshal(type) *(type*)&message->slots[marshal_slot++]\n";
	common_header << "#define fipc_send(rpc, msg_ptr) /* TODO */\n";
	common_header << "#define fipc_get_remote(local) NULL\n";
	common_header << "#define fipc_get_local(remote) NULL\n";
	common_header << "#define fipc_create_shadow(remote) NULL\n";
	common_header << "#define fipc_destroy_shadow(remote)\n\n";

	common_header << "#define inject_trampoline(id, pointer) LCD_DUP_TRAMPOLINE(id) + offsetof(struct lcd_trampoline_handle, trampoline)\n\n";

	common_header << "enum dispatch_id {\n";

	for (const marshal_unit_lists& unit : rpc_lists) {
		common_header << "\trpc_" << unit.identifier << ",\n";
	}

	for (const marshal_unit_lists& unit : rpc_ptr_lists) {
		common_header << "\trpc_ptr" << unit.identifier << ",\n";
	}

	common_header << "};\n\n";

	common_header << "struct fipc_message {\n\tenum dispatch_id host_id;\n\tuint64_t slots[MAX_MESSAGE_SLOTS];\n};\n\n";

	common_header << "#endif";
}

void idlc::generate_klcd_source(
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

	write_pointer_stubs(kernel_dispatch_source, rpc_pointer_lists);

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

void idlc::generate_lcd_source(
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

	write_pointer_stubs(driver_dispatch_source, rpc_pointer_lists);
}

void idlc::do_code_generation(
	std::string_view driver_name,
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

	std::string klcd_name {driver_name.data(), driver_name.size()};
	klcd_name += "_klcd";

	std::string lcd_name {driver_name.data(), driver_name.size()};
	lcd_name += "_lcd";

	const fs::path klcd_path {destination / (klcd_name + ".c")}; // klcd
	const fs::path klcd_h_path {destination / (klcd_name + ".h")}; // klcd
	const fs::path lcd_path {destination / (lcd_name + ".c")}; // lcd
	const fs::path lcd_h_path {destination / (lcd_name + ".h")}; // lcd
	const fs::path comm_path {destination / "common.h"}; // lcd

	generate_common_header(comm_path, driver_name, rpc_lists, rpc_pointer_lists);
	generate_klcd_source(klcd_path, rpc_lists, rpc_pointer_lists);
	generate_lcd_source(lcd_path, rpc_lists, rpc_pointer_lists);
}