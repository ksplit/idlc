#include "code_generation.h"

#include <fstream>
#include <vector>

#include "marshaling.h"
#include "../frontend/dump.h" // for tab_over()

// Here be tamer dragons
// It's not bad, it's just not as clean as I'd like it

namespace fs = std::filesystem;

namespace idlc {
	void write_marshal_ops(
		std::ofstream& file,
		const std::vector<marshal_op>& ops,
		unsigned int indent
	);

	void write_pointer_stubs(
		std::ofstream& file,
		gsl::span<const marshal_unit_lists> rpc_pointer_lists
	);

	void generate_common_header(
		const fs::path& root,
		std::string_view module_name,
		gsl::span<const gsl::czstring<>> headers,
		gsl::span<const marshal_unit_lists> rpc_lists,
		gsl::span<const marshal_unit_lists> rpc_ptr_lists
	);

	void generate_common_source(const fs::path& root);

	void generate_klcd_source(
		const fs::path& root,
		gsl::span<const marshal_unit_lists> rpc_lists,
		gsl::span<const marshal_unit_lists> rpc_pointer_lists
	);

	void generate_lcd_source(
		const fs::path& root,
		gsl::span<const marshal_unit_lists> rpc_lists,
		gsl::span<const marshal_unit_lists> rpc_pointer_lists
	);

	void generate_klcd(
		const fs::path& root,
		std::string_view driver_name,
		gsl::span<const marshal_unit_lists> rpc_lists,
		gsl::span<const marshal_unit_lists> rpc_pointer_lists
	);

	void generate_lcd(
		const fs::path& root,
		std::string_view driver_name,
		gsl::span<const marshal_unit_lists> rpc_lists,
		gsl::span<const marshal_unit_lists> rpc_pointer_lists
	);

	// Currently a criminal hack, since we don't handle identifier variants uniformly (yet)
	inline std::string to_upper(std::string str)
	{
		std::transform(str.begin(), str.end(), str.begin(), [](char c) { return std::toupper(c); });
		return str;
	}
}

void idlc::generate_common_source(const fs::path& root)
{
	std::ofstream source {root};
	source.exceptions(std::fstream::badbit | std::fstream::failbit);

	source << "#include \"common.h\"\n\n";
	source << "void* inject_trampoline_impl(lcd_trampoline_handle* handle, void* impl) {\n";
	source << "\thandle->hidden_args = impl;\n";
	source << "\treturn handle->trampoline;\n";
	source << "}\n\n";
}

void idlc::generate_klcd(
	const fs::path& root,
	std::string_view driver_name,
	gsl::span<const marshal_unit_lists> rpc_lists,
	gsl::span<const marshal_unit_lists> rpc_pointer_lists
)
{
	std::string name {driver_name.data(), driver_name.size()};
	name += "_klcd";

	const fs::path source_path {root / (name + ".c")};
	const fs::path header_path {root / (name + ".h")};	
	const fs::path kbuild_path {root / "Kbuild"};
	generate_klcd_source(source_path, rpc_lists, rpc_pointer_lists);

	std::ofstream kbuild {kbuild_path};
	kbuild.exceptions(kbuild.badbit | kbuild.failbit);
	kbuild << "obj-m += " << driver_name << "_klcd.o\n";
	kbuild << "obj-m += common.o\n";
	kbuild << "ccflags-y += $(NONISOLATED_CFLAGS) -Wno-error=declaration-after-statement -Wno-error=discarded-qualifiers\n";
}

void idlc::generate_lcd(
	const fs::path& root,
	std::string_view driver_name,
	gsl::span<const marshal_unit_lists> rpc_lists,
	gsl::span<const marshal_unit_lists> rpc_pointer_lists
)
{
	std::string name {driver_name.data(), driver_name.size()};
	name += "_lcd";

	const fs::path source_path {root / (name + ".c")};
	const fs::path header_path {root / (name + ".h")};
	const fs::path kbuild_path {root / "Kbuild"};
	generate_lcd_source(source_path, rpc_lists, rpc_pointer_lists);

	std::ofstream kbuild {kbuild_path};
	kbuild.exceptions(kbuild.badbit | kbuild.failbit);
	kbuild << "obj-m += " << driver_name << "_lcd.o\n";
	kbuild << "obj-m += common.o\n";
	kbuild << "cppflags-y += $(ISOLATED_CFLAGS)\n";
	kbuild << "ccflags-y += $(ISOLATED_CFLAGS) -Wno-error=declaration-after-statement -Wno-error=discarded-qualifiers\n";
	kbuild << "extra-y += ../../../liblcd_build/common/vmfunc.lds\n";
	kbuild << "ldflags-y += -T $(LIBLCD_BUILD_DIR)/common/vmfunc.lds\n";
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
			tab_over(file, indent) << "fipc_send(" << to_upper(args.rpc) << ", message);\n";
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

void idlc::write_pointer_stubs(std::ofstream& file, gsl::span<const marshal_unit_lists> rpc_pointer_lists)
{
	for (const marshal_unit_lists& unit : rpc_pointer_lists) {
		file << "void " << unit.identifier << "_callee(struct fipc_message* message) {\n";
		file << "\tunsigned int marshal_slot = 0;\n";
		write_marshal_ops(file, unit.callee_ops, 1);
		file << "}\n\n";
	}

	for (const marshal_unit_lists& unit : rpc_pointer_lists) {
		file << "LCD_TRAMPOLINE_DATA(trampoline" << unit.identifier << ")\n";
		file << "LCD_TRAMPOLINE_LINKAGE(trampoline" << unit.identifier << ")\n";
		file << unit.header << " {\n";
		file << "\tvoid* real_pointer;\n\tLCD_TRAMPOLINE_PROLOGUE(real_pointer, trampoline" << unit.identifier << ");\n";
		file << "\tunsigned int marshal_slot = 0;\n";
		file << "\tstruct fipc_message message_buffer = {0};\n";
		file << "\tstruct fipc_message* message = &message_buffer;\n";
		write_marshal_ops(file, unit.caller_ops, 1);
		file << "}\n\n";
	}
}

void idlc::generate_common_header(
	const std::filesystem::path& root,
	std::string_view module_name,
	gsl::span<const gsl::czstring<>> headers,
	gsl::span<const marshal_unit_lists> rpc_lists,
	gsl::span<const marshal_unit_lists> rpc_ptr_lists
)
{
	std::ofstream header {root};
	header.exceptions(std::fstream::badbit | std::fstream::failbit);

	header << "#ifndef _COMMON_H_\n#define _COMMON_H_\n\n";
	header << "#include <linux/types.h>\n";
	header << "#include <liblcd/trampoline.h>\n\n";

	header << "#include \"" << module_name << "_user.h\"\n\n";

	for (const auto imp : headers) {
		header << "#include <" << imp << ">\n";
	}

	header << "\n";

	header << "#define MAX_MESSAGE_SLOTS 64\n\n";
	header << "#define fipc_marshal(value) message->slots[marshal_slot++] = *(uint64_t*)&value\n";
	header << "#define fipc_unmarshal(type) *(type*)&message->slots[marshal_slot++]\n";
	header << "#define fipc_send(rpc, msg_ptr) /* TODO */\n";
	header << "#define fipc_get_remote(local) NULL; (void)local\n";
	header << "#define fipc_get_local(remote) NULL; (void)remote\n";
	header << "#define fipc_create_shadow(remote) NULL; (void)remote\n";
	header << "#define fipc_destroy_shadow(remote) (void)remote\n\n";
	header << "#define inject_trampoline(id, pointer) inject_trampoline_impl(LCD_DUP_TRAMPOLINE(trampoline##id), pointer)\n\n";
	header << "enum dispatch_id {\n";

	for (const marshal_unit_lists& unit : rpc_lists) {
		header << "\tRPC_" << to_upper(unit.identifier) << ",\n";
	}

	for (const marshal_unit_lists& unit : rpc_ptr_lists) {
		header << "\tRPC_PTR" << to_upper(unit.identifier) << ",\n";
	}

	header << "};\n\n";
	header << "struct fipc_message {\n\tenum dispatch_id host_id;\n\tuint64_t slots[MAX_MESSAGE_SLOTS];\n};\n\n";
	header << "void* inject_trampoline_impl(lcd_trampoline_handle* handle, void* impl);\n\n";
	header << "#endif";
}

void idlc::generate_klcd_source(
	const std::filesystem::path& root,
	gsl::span<const marshal_unit_lists> rpc_lists,
	gsl::span<const marshal_unit_lists> rpc_pointer_lists
)
{
	std::ofstream source {root};
	source.exceptions(std::fstream::badbit | std::fstream::failbit);

	source << "#include \"../common.h\"\n\n";
	for (const marshal_unit_lists& unit : rpc_lists) {
		source << "void " << unit.identifier << "_callee(struct fipc_message* message) {\n";
		source << "\tunsigned int marshal_slot = 0;\n";
		write_marshal_ops(source, unit.callee_ops, 1);
		source << "}\n\n";
	}

	write_pointer_stubs(source, rpc_pointer_lists);

	source << "void dispatch(struct fipc_message* message) {\n";
	source << "\tswitch (message->host_id) {\n";

	for (const marshal_unit_lists& unit : rpc_lists) {
		source << "\tcase RPC_" << to_upper(unit.identifier) << ":\n";
		source << "\t\t" << unit.identifier << "_callee(message)" << ";\n";
		source << "\t\tbreak;\n\n";
	}

	for (const marshal_unit_lists& unit : rpc_pointer_lists) {
		source << "\tcase RPC_PTR" << to_upper(unit.identifier) << ":\n";
		source << "\t\t" << unit.identifier << "_callee(message)" << ";\n";
		source << "\t\tbreak;\n\n";
	}

	source << "\tdefault:\n\t\tbreak;\n";
	source << "\t}\n}\n\n";
}

void idlc::generate_lcd_source(
	const std::filesystem::path& root,
	gsl::span<const marshal_unit_lists> rpc_lists,
	gsl::span<const marshal_unit_lists> rpc_pointer_lists
)
{
	std::ofstream source {root};
	source.exceptions(std::fstream::badbit | std::fstream::failbit);

	source << "#include \"../common.h\"\n\n";
	for (const marshal_unit_lists& unit : rpc_lists) {
		source << unit.header << " {\n";
		source << "\tunsigned int marshal_slot = 0;\n";
		source << "\tstruct fipc_message message_buffer = {0};\n";
		source << "\tstruct fipc_message* message = &message_buffer;\n";
		write_marshal_ops(source, unit.caller_ops, 1);
		source << "}\n\n";
	}

	write_pointer_stubs(source, rpc_pointer_lists);

	source << "void dispatch(struct fipc_message* message) {\n";
	source << "\tswitch (message->host_id) {\n";

	for (const marshal_unit_lists& unit : rpc_pointer_lists) {
		source << "\tcase RPC_PTR" << to_upper(unit.identifier) << ":\n";
		source << "\t\t" << unit.identifier << "_callee(message)" << ";\n";
		source << "\t\tbreak;\n\n";
	}

	source << "\tdefault:\n\t\tbreak;\n";
	source << "\t}\n}\n\n";
}

void idlc::generate_module(
	const std::filesystem::path& root,
	std::string_view driver_name,
	gsl::span<const gsl::czstring<>> headers,
	gsl::span<const marshal_unit_lists> rpc_lists,
	gsl::span<const marshal_unit_lists> rpc_pointer_lists
)
{
	namespace fs = std::filesystem;

	if (!fs::exists(root)) {
		fs::create_directories(root);
	}

	// TODO: problem: we don't know which side the function pointers end up on
	// TODO: we'll just put rpc pointers on both sides, they don't have the same name conflicts as RPCs do (the func-named facade and the actual func)

	const auto canon_root = fs::canonical(root);
	const fs::path klcd_dir {canon_root / "klcd"};
	const fs::path lcd_dir {canon_root / "lcd"};
	const fs::path kbuild_path {canon_root / "Kbuild"};
	fs::create_directories(klcd_dir);
	fs::create_directories(lcd_dir);

	generate_klcd(klcd_dir, driver_name, rpc_lists, rpc_pointer_lists);
	generate_lcd(lcd_dir, driver_name, rpc_lists, rpc_pointer_lists);
	
	const fs::path common_h {canon_root / "common.h"};
	const fs::path common_c {canon_root / "common.c"};
	generate_common_header(common_h, driver_name, headers, rpc_lists, rpc_pointer_lists);
	generate_common_source(common_c);

	std::ofstream kbuild {kbuild_path};
	kbuild.exceptions(kbuild.badbit | kbuild.failbit);
	kbuild << "obj-$(LCD_CONFIG_BUILD_" << to_upper(canon_root.filename()) << "_LCD) += lcd/\n";
	kbuild << "obj-$(LCD_CONFIG_BUILD_" << to_upper(canon_root.filename()) << "_KLCD) += klcd/\n";
}