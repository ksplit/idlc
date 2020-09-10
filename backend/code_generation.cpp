#include "code_generation.h"

#include <fstream>
#include <vector>

#include "marshaling.h"
#include "../frontend/dump.h" // for tab_over()

// Here be tamer dragons
// It's not bad, it's just not as clean as I'd like it

namespace fs = std::filesystem;

namespace idlc {
	enum lcd_side {
		klcd,
		lcd
	};

	void write_marshal_ops(
		std::ofstream& file,
		const std::vector<marshal_op>& ops,
		lcd_side side,
		unsigned int indent
	);

	void write_pointer_stubs(
		std::ofstream& file,
		gsl::span<const rpc_pointer_unit> rpc_pointer_lists,
		lcd_side side
	);

	void generate_common_header(
		const fs::path& root,
		std::string_view module_name,
		gsl::span<const gsl::czstring<>> headers,
		gsl::span<const rpc_unit> rpc_lists,
		gsl::span<const rpc_pointer_unit> rpc_ptr_lists
	);

	void generate_common_source(const fs::path& root);

	void generate_klcd_source(
		const fs::path& root,
		gsl::span<const rpc_unit> rpc_lists,
		gsl::span<const rpc_pointer_unit> rpc_pointer_lists
	);

	void generate_lcd_source(
		const fs::path& root,
		gsl::span<const rpc_unit> rpc_lists,
		gsl::span<const rpc_pointer_unit> rpc_pointer_lists
	);

	void generate_klcd(
		const fs::path& root,
		std::string_view driver_name,
		std::string_view module_root,
		gsl::span<const rpc_unit> rpc_lists,
		gsl::span<const rpc_pointer_unit> rpc_pointer_lists
	);

	void generate_lcd(
		const fs::path& root,
		std::string_view driver_name,
		std::string_view module_root,
		gsl::span<const rpc_unit> rpc_lists,
		gsl::span<const rpc_pointer_unit> rpc_pointer_lists
	);

	void generate_linker_script(const fs::path& root, gsl::span<const rpc_pointer_unit> rpc_pointers);

	// Currently a criminal hack, since we don't handle identifier variants uniformly (yet)
	inline std::string to_upper(std::string str)
	{
		std::transform(str.begin(), str.end(), str.begin(), [](char c) { return std::toupper(c); });
		return str;
	}
}

void idlc::generate_linker_script(const fs::path& root, gsl::span<const rpc_pointer_unit> rpc_pointers)
{
	std::ofstream script {root};
	script.exceptions(script.badbit | script.failbit);
	script << "#include <liblcd/trampoline_link.h>\n\n";
	script << "SECTIONS\n{\n";

	for (const auto& rpc : rpc_pointers) {
		script << "\tLCD_TRAMPOLINE_LINKER_SECTION(trampoline" << rpc.identifier << ")\n";
	}

	script << "}\n";
	script << "INSERT AFTER .text\n\n";
}

void idlc::generate_common_source(const fs::path& root)
{
	std::ofstream source {root};
	source.exceptions(std::fstream::badbit | std::fstream::failbit);

	source << "#include <lcd_config/pre_hook.h>\n#include \"../common.h\"\n#include <lcd_config/post_hook.h>\n\n";

	source << "DEFINE_HASHTABLE(locals, 5);\n";
	source << "DEFINE_HASHTABLE(remotes, 5);\n\n";

	source << "void* fipc_get_remote(void* local) {\n";
	source << "\tLIBLCD_MSG(\"Finding remote for %p\", local);\n";
	source << "\tstruct ptr_node* node;\n";
	source << "\thash_for_each_possible(remotes, node, hentry, (unsigned long)local) {\n";
	source << "\t\tif (node->key == local)\n\t\treturn node->ptr;\n";
	source << "\t}\n\n";
	source << "\tBUG();\n";
	source << "\treturn NULL;\n";
	source << "}\n\n";

	source << "void* fipc_get_local(void* remote) {\n";
	source << "\tLIBLCD_MSG(\"Finding shadow for %p\", remote);\n";
	source << "\tstruct ptr_node* node;\n";
	source << "\thash_for_each_possible(locals, node, hentry, (unsigned long)remote) {\n";
	source << "\t\tLIBLCD_MSG(\"Observed %p\", node->key);\n";
	source << "\t\tif (node->key == remote)\n\t\treturn node->ptr;\n";
	source << "\t}\n\n";
	source << "\tBUG();\n";
	source << "\treturn NULL;\n";
	source << "}\n\n";

	source << "void* fipc_create_shadow_impl(void* remote, size_t size) {\n";
	source << "\tvoid* local = kmalloc(size, GFP_KERNEL);\n";
	source << "\tstruct ptr_node* local_node = kmalloc(sizeof(struct ptr_node), GFP_KERNEL);\n";
	source << "\tstruct ptr_node* remote_node = kmalloc(sizeof(struct ptr_node), GFP_KERNEL);\n";
	source << "\tlocal_node->ptr = local;\n";
	source << "\tlocal_node->key = remote;\n";
	source << "\tremote_node->ptr = remote;\n";
	source << "\tremote_node->key = local;\n";
	source << "\thash_add(locals, &local_node->hentry, (unsigned long)remote);\n";
	source << "\thash_add(remotes, &remote_node->hentry, (unsigned long)local);\n";
	source << "\tLIBLCD_MSG(\"Pointer %p assigned shadow %p\", remote, local);\n";
	source << "\treturn local;\n";
	source << "}\n\n";

	source << "void fipc_destroy_shadow(void* remote) {\n";
	source << "\tstruct ptr_node* local_node;\n";
	source << "\tstruct ptr_node* remote_node;\n";
	source << "\thash_for_each_possible(locals, local_node, hentry, (unsigned long)remote) {\n";
	source << "\t\tif (local_node->key == remote) {\n";
	source << "\t\t\thash_del(&local_node->hentry);\n";
	source << "\t\t\tbreak;\n";
	source << "\t\t}\n";
	source << "\t}\n\n";
	source << "\tvoid* local = local_node->ptr;\n";
	source << "\thash_for_each_possible(remotes, remote_node, hentry, (unsigned long)local) {\n";
	source << "\t\tif (remote_node->key == local) {\n";
	source << "\t\t\thash_del(&remote_node->hentry);\n";
	source << "\t\t\tbreak;\n";
	source << "\t\t}\n";
	source << "\t}\n\n";
	source << "\tkfree(local);\n";
	source << "\tkfree(local_node);\n";
	source << "\tkfree(remote_node);\n";
	source << "\tLIBLCD_MSG(\"Destroyed shadow for %p\", remote);\n";
	source << "}\n\n";

	source << "void* inject_trampoline_impl(struct lcd_trampoline_handle* handle, void* impl, size_t size, void* real_ptr) {\n";
	source << "\tstruct trampoline_args* args = kmalloc(sizeof(struct trampoline_args), GFP_KERNEL);\n";
	source << "\targs->impl = impl;\n";
	source << "\targs->real_ptr = real_ptr;\n";
	source << "\thandle->hidden_args = args;\n";
	source << "\tset_memory_x(((unsigned long)handle) & PAGE_MASK, ALIGN(size, PAGE_SIZE) >> PAGE_SHIFT);\n";
	source << "\treturn handle->trampoline;\n";
	source << "}\n\n";

	source << "void fipc_translate(struct fipc_message* msg, enum dispatch_id* rpc, struct rpc_message* pckt) {\n";
	source << "\tpckt->end_slot = pckt->slots;\n\n";
	source << "\tunsigned slots_used = msg->rpc_id >> 16;\n";
	source << "\t*rpc = msg->rpc_id & 0xFFFF;\n\n";
	source << "\tunsigned fast_slots = min(slots_used, (unsigned)(FIPC_NR_REGS));\n";
	source << "\tunsigned slow_slots = (slots_used > FIPC_NR_REGS) ? slots_used - FIPC_NR_REGS : 0;\n\n";
	source << "\tfor (unsigned i = 0; i < fast_slots; ++i) {\n";
	source << "\t\tpckt->slots[i] = msg->regs[i];\n";
	source << "\t}\n\n";
	source << "\tif (slow_slots) {\n";
	source << "\t\tstruct ext_registers* ext = get_register_page(smp_processor_id());\n";
	source << "\t\tfor (unsigned i = 0; i < slow_slots; ++i) {\n";
	source << "\t\t\tpckt->slots[i + FIPC_NR_REGS] = ext->regs[i];\n";
	source << "\t\t}\n";
	source << "\t}\n\n";
	source << "}\n\n";

	source << "void fipc_pack(struct fipc_message* fmsg, enum dispatch_id rpc, struct rpc_message* msg) {\n";
	source << "\tunsigned slots_used = msg->end_slot - msg->slots;\n";
	source << "\tfmsg->vmfunc_id = VMFUNC_RPC_CALL;\n";
	source << "\tfmsg->rpc_id = rpc | (slots_used << 16);\n\n";
	source << "\tunsigned fast_slots = min(slots_used, (unsigned)(FIPC_NR_REGS));\n";
	source << "\tunsigned slow_slots = (slots_used > FIPC_NR_REGS) ? slots_used - FIPC_NR_REGS : 0;\n\n";
	source << "\tfor (unsigned i = 0; i < fast_slots; ++i) {\n";
	source << "\t\tfmsg->regs[i] = msg->slots[i];\n";
	source << "\t}\n\n";
	source << "\tif (slow_slots) {\n";
	source << "\t\tstruct ext_registers* ext = get_register_page(smp_processor_id());\n";
	source << "\t\tfor (unsigned i = 0; i < slow_slots; ++i) {\n";
	source << "\t\t\text->regs[i] = msg->slots[i + FIPC_NR_REGS];\n";
	source << "\t\t}\n";
	source << "\t}\n\n";
	source << "}\n\n";

	source << "void fipc_send_to_klcd(enum dispatch_id rpc, struct rpc_message* msg) {\n";
	source << "\tstruct fipc_message fmsg;\n";
	source << "\tfipc_pack(&fmsg, rpc, msg);\n";
	source << "\tvmfunc_wrapper(&fmsg);\n";
	source << "\tfipc_translate(&fmsg, &rpc, msg);\n";
	source << "}\n\n";

	source << "void fipc_send_to_lcd(enum dispatch_id rpc, struct rpc_message* msg) {\n";
	source << "\tstruct fipc_message fmsg;\n";
	source << "\tfipc_pack(&fmsg, rpc, msg);\n";
	source << "\tvmfunc_klcd_wrapper(&fmsg, OTHER_DOMAIN);\n";
	source << "\tfipc_translate(&fmsg, &rpc, msg);\n";
	source << "}\n\n";
}

void idlc::generate_klcd(
	const fs::path& root,
	std::string_view driver_name,
	std::string_view module_root,
	gsl::span<const rpc_unit> rpc_lists,
	gsl::span<const rpc_pointer_unit> rpc_pointer_lists
)
{
	std::string module_name {"lcd_test_mod_"};
	module_name += root.parent_path().filename();
	module_name += "_klcd";

	std::string glue_name {driver_name.data(), driver_name.size()};
	glue_name += "_klcd";

	const fs::path source_path {root / (glue_name + ".c")};
	const fs::path header_path {root / (glue_name + ".h")};
	const fs::path comm_path {root / "common.c"};	
	const fs::path kbuild_path {root / "Kbuild"};
	const fs::path lnks_path {root / "trampolines.lds.S"};
	generate_klcd_source(source_path, rpc_lists, rpc_pointer_lists);
	generate_linker_script(lnks_path, rpc_pointer_lists);
	generate_common_source(comm_path);

	std::ofstream kbuild {kbuild_path};
	kbuild.exceptions(kbuild.badbit | kbuild.failbit);
	kbuild << "obj-m += " << module_name << ".o\n";
	kbuild << module_name << "-y += " << driver_name << "_klcd.o\n";
	kbuild << module_name << "-y += common.o\n\n";
	kbuild << "extra-y += trampolines.lds\n\n";
	kbuild << "ldflags-y += -T $(LCD_TEST_MODULES_BUILD_DIR)/" << module_root << "/klcd/trampolines.lds\n\n";
	kbuild << "ccflags-y += $(NONISOLATED_CFLAGS) -std=gnu99 -Wno-error=declaration-after-statement -Wno-error=discarded-qualifiers\n";
	kbuild << "cppflags-y += $(NONISOLATED_CFLAGS)\n";
}

void idlc::generate_lcd(
	const fs::path& root,
	std::string_view driver_name,
	std::string_view module_root,
	gsl::span<const rpc_unit> rpc_lists,
	gsl::span<const rpc_pointer_unit> rpc_pointer_lists
)
{
	std::string module_name {"lcd_test_mod_"};
	module_name += root.parent_path().filename();
	module_name += "_lcd";

	std::string glue_name {driver_name.data(), driver_name.size()};
	glue_name += "_lcd";

	const fs::path source_path {root / (glue_name + ".c")};
	const fs::path header_path {root / (glue_name + ".h")};
	const fs::path comm_path {root / "common.c"};
	const fs::path kbuild_path {root / "Kbuild"};
	const fs::path lnks_path {root / "trampolines.lds.S"};
	generate_lcd_source(source_path, rpc_lists, rpc_pointer_lists);
	generate_linker_script(lnks_path, rpc_pointer_lists);
	generate_common_source(comm_path);

	std::ofstream kbuild {kbuild_path};
	kbuild.exceptions(kbuild.badbit | kbuild.failbit);
	kbuild << "obj-m += " << module_name << ".o\n";
	kbuild << module_name << "-y += " << driver_name << "_lcd.o\n";
	kbuild << module_name << "-y += common.o\n";
	kbuild << module_name << "-y += $(LIBLCD)\n\n";
	kbuild << "extra-y += trampolines.lds\n";
	kbuild << "extra-y += ../../../liblcd_build/common/vmfunc.lds\n\n";
	kbuild << "cppflags-y += $(ISOLATED_CFLAGS)\n";
	kbuild << "ccflags-y += $(ISOLATED_CFLAGS) -std=gnu99 -Wno-error=declaration-after-statement -Wno-error=discarded-qualifiers\n\n";
	kbuild << "ldflags-y += -T $(LCD_TEST_MODULES_BUILD_DIR)/" << module_root << "/lcd/trampolines.lds\n";
	kbuild << "ldflags-y += -T $(LIBLCD_BUILD_DIR)/common/vmfunc.lds\n";
}

// Oversized, but clear

void idlc::write_marshal_ops(std::ofstream& file, const std::vector<marshal_op>& ops, lcd_side side, unsigned int indent)
{
	const auto send_str = (side == lcd_side::klcd) ? "fipc_send_to_lcd" : "fipc_send_to_klcd";

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

			tab_over(file, indent) << "message->end_slot = message->slots;\n";

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

			tab_over(file, indent) << "message->end_slot = message->slots;\n";

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
			tab_over(file, indent) << send_str << "(" << to_upper(args.rpc) << ", message);\n";
			tab_over(file, indent) << "message->end_slot = message->slots;\n";
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

void idlc::write_pointer_stubs(std::ofstream& file, gsl::span<const rpc_pointer_unit> rpc_pointer_lists, lcd_side side)
{
	for (const auto& unit : rpc_pointer_lists) {
		file << "void " << unit.identifier << "_callee(struct rpc_message* message)\n{\n";
		write_marshal_ops(file, unit.callee_ops, side, 1);
		file << "}\n\n";
	}

	for (const auto& unit : rpc_pointer_lists) {
		file << unit.impl_header << "\n{\n";
		file << "\tstruct rpc_message message_buffer = {0};\n";
		file << "\tmessage_buffer.end_slot = message_buffer.slots;\n";
		file << "\tstruct rpc_message* message = &message_buffer;\n";
		write_marshal_ops(file, unit.caller_ops, side, 1);
		file << "}\n\n";
	}

	for (const auto& unit : rpc_pointer_lists) {
		file << "LCD_TRAMPOLINE_LINKAGE(trampoline" << unit.identifier << ")\n";
		file << unit.header << "\n{\n";
		file << "\tstruct trampoline_args* args;\n\tLCD_TRAMPOLINE_PROLOGUE(args, trampoline" << unit.identifier << ");\n";
		file << "\t" << unit.impl_call << ";\n";
		file << "}\n\n";
	}
}

void idlc::generate_common_header(
	const std::filesystem::path& root,
	std::string_view module_name,
	gsl::span<const gsl::czstring<>> headers,
	gsl::span<const rpc_unit> rpc_lists,
	gsl::span<const rpc_pointer_unit> rpc_ptr_lists
)
{
	std::ofstream header {root};
	header.exceptions(std::fstream::badbit | std::fstream::failbit);

	header << "#ifndef _COMMON_H_\n#define _COMMON_H_\n\n";
	header << "#include <asm/pgtable_types.h>\n";
	header << "#include <asm/cacheflush.h>\n";
	header << "#include <linux/types.h>\n";
	header << "#include <linux/hashtable.h>\n";
	header << "#include <libfipc.h>\n";
	header << "#include <liblcd/boot_info.h>\n";
	header << "#include <liblcd/trampoline.h>\n\n";

	header << "#include \"" << module_name << "_user.h\"\n\n";

	for (const auto imp : headers) {
		header << "#include <" << imp << ">\n";
	}

	header << "\n";

	header << "#define MAX_MESSAGE_SLOTS 64\n\n";
	header << "#define fipc_marshal(value) *(message->end_slot++) = (uint64_t)value\n";
	header << "#define fipc_unmarshal(type) (type)*(message->end_slot++)\n";
	header << "#define fipc_create_shadow(remote) fipc_create_shadow_impl(remote, sizeof(*remote))\n\n";
	header << "#define inject_trampoline(id, pointer) inject_trampoline_impl(LCD_DUP_TRAMPOLINE(trampoline##id), trampoline_impl##id, LCD_TRAMPOLINE_SIZE(trampoline##id), pointer)\n\n";
	header << "struct ptr_node {\n";
	header << "\tvoid* ptr;\n";
	header << "\tvoid* key;\n";
	header << "\tstruct hlist_node hentry;\n";
	header << "};\n\n";

	header << "enum dispatch_id {\n";
	header << "\tRPC_LCD_INIT,\n";
	header << "\tRPC_LCD_EXIT,\n";

	for (const auto& unit : rpc_lists) {
		header << "\tRPC_" << to_upper(unit.identifier) << ",\n";
	}

	for (const auto& unit : rpc_ptr_lists) {
		header << "\tRPC_PTR" << to_upper(unit.identifier) << ",\n";
	}

	header << "};\n\n";

	header << "struct rpc_message {\n\tuint64_t* end_slot;\n\tuint64_t slots[MAX_MESSAGE_SLOTS];\n};\n\n";
	header << "struct trampoline_args {\n\tvoid* impl;\n\tvoid* real_ptr;\n};\n\n";
	
	header << "extern DECLARE_HASHTABLE(locals, 5);\n";
	header << "extern DECLARE_HASHTABLE(remotes, 5);\n\n";

	header << "void* fipc_get_remote(void* local);\n";
	header << "void* fipc_get_local(void* remote);\n";
	header << "void* fipc_create_shadow_impl(void* remote, size_t size);\n";
	header << "void fipc_destroy_shadow(void* remote);\n";
	header << "void* inject_trampoline_impl(struct lcd_trampoline_handle* handle, void* impl, size_t size, void* real_ptr);\n";
	header << "void fipc_translate(struct fipc_message* msg, enum dispatch_id* rpc, struct rpc_message* pckt);\n";
	header << "void fipc_pack(struct fipc_message* fmsg, enum dispatch_id rpc, struct rpc_message* msg);\n";
	header << "void fipc_send_to_klcd(enum dispatch_id rpc, struct rpc_message* msg);\n";
	header << "void fipc_send_to_lcd(enum dispatch_id rpc, struct rpc_message* msg);\n\n";

	for (const auto& unit : rpc_ptr_lists) {
		header << "LCD_TRAMPOLINE_DATA(trampoline" << unit.identifier << ")\n";
		header << unit.header << ";\n\n";
	}

	for (const auto& unit : rpc_ptr_lists) {
		header << unit.impl_header << ";\n\n";
	}

	header << "\n";
	header << "#endif";
}

void idlc::generate_klcd_source(
	const std::filesystem::path& root,
	gsl::span<const rpc_unit> rpc_lists,
	gsl::span<const rpc_pointer_unit> rpc_pointer_lists
)
{
	std::ofstream source {root};
	source.exceptions(std::fstream::badbit | std::fstream::failbit);

	source << "#include \"../common.h\"\n\n";
	for (const rpc_unit& unit : rpc_lists) {
		source << "void " << unit.identifier << "_callee(struct rpc_message* message) {\n";
		write_marshal_ops(source, unit.callee_ops, lcd_side::klcd, 1);
		source << "}\n\n";
	}

	write_pointer_stubs(source, rpc_pointer_lists, lcd_side::klcd);

	source << "int dispatch(struct fipc_message* received) {\n";
	source << "\tenum dispatch_id rpc;\n";
	source << "\tstruct rpc_message message_buffer;\n";
	source << "\tstruct rpc_message* message = &message_buffer;\n";
	source << "\tfipc_translate(received, &rpc, message);\n";
	source << "\tswitch (rpc) {\n";

	for (const auto& unit : rpc_lists) {
		const auto id = to_upper(unit.identifier);
		source << "\tcase RPC_" << id << ":\n";
		source << "\t\tklcd_trace(RPC_" << id << ");\n";
		source << "\t\t" << unit.identifier << "_callee(message)" << ";\n";
		source << "\t\tbreak;\n\n";
	}

	for (const auto& unit : rpc_pointer_lists) {
		const auto id = to_upper(unit.identifier);
		source << "\tcase RPC_PTR" << id << ":\n";
		source << "\t\tklcd_trace(RPC_PTR" << id << ");\n";
		source << "\t\t" << unit.identifier << "_callee(message)" << ";\n";
		source << "\t\tbreak;\n\n";
	}

	source << "\tdefault:\n\t\tbreak;\n";
	source << "\t}\n\n\tfipc_pack(received, rpc, message);\n\n\treturn 0;\n}\n\n";
}

void idlc::generate_lcd_source(
	const std::filesystem::path& root,
	gsl::span<const rpc_unit> rpc_lists,
	gsl::span<const rpc_pointer_unit> rpc_pointer_lists
)
{
	std::ofstream source {root};
	source.exceptions(std::fstream::badbit | std::fstream::failbit);

	source << "#include \"../common.h\"\n\n";
	for (const rpc_unit& unit : rpc_lists) {
		source << unit.header << " {\n";
		source << "\tstruct rpc_message message_buffer = {0};\n";
		source << "\tmessage_buffer.end_slot = message_buffer.slots;\n";
		source << "\tstruct rpc_message* message = &message_buffer;\n";
		write_marshal_ops(source, unit.caller_ops, lcd_side::lcd, 1);
		source << "}\n\n";
	}

	write_pointer_stubs(source, rpc_pointer_lists, lcd_side::lcd);

	source << "int handle_rpc_calls(struct fipc_message* received) {\n";
	source << "\tenum dispatch_id rpc;\n";
	source << "\tstruct rpc_message message_buffer;\n";
	source << "\tstruct rpc_message* message = &message_buffer;\n";
	source << "\tfipc_translate(received, &rpc, message);\n";
	source << "\tswitch (rpc) {\n";
	source << "\tcase RPC_LCD_INIT:\n";
	source << "\t\tlcd_trace(RPC_LCD_INIT);\n";
	source << "\t\tbreak;\n\n";
	source << "\tcase RPC_LCD_EXIT:\n";
	source << "\t\tlcd_trace(RPC_LCD_EXIT);\n";
	source << "\t\tbreak;\n\n";

	for (const auto& unit : rpc_pointer_lists) {
		const auto id = to_upper(unit.identifier);
		source << "\tcase RPC_PTR" << id << ":\n";
		source << "\t\tlcd_trace(RPC_PTR" << id << ");\n";
		source << "\t\t" << unit.identifier << "_callee(message)" << ";\n";
		source << "\t\tbreak;\n\n";
	}

	source << "\tdefault:\n\t\tbreak;\n";
	source << "\t}\n\n\tfipc_pack(received, rpc, message);\n\n\treturn 0;\n}\n\n";
}

void idlc::generate_module(
	const std::filesystem::path& root,
	std::string_view driver_name,
	gsl::span<const gsl::czstring<>> headers,
	gsl::span<const rpc_unit> rpc_lists,
	gsl::span<const rpc_pointer_unit> rpc_pointer_lists
)
{
	namespace fs = std::filesystem;

	if (!fs::exists(root)) {
		fs::create_directories(root);
	}

	// TODO: problem: we don't know which side the function pointers end up on
	// TODO: we'll just put rpc pointers on both sides, they don't have the same name conflicts as RPCs do (the func-named facade and the actual func)

	const auto canon_root = fs::canonical(root);
	const auto module_root = canon_root.filename();
	const auto klcd_dir {canon_root / "klcd"};
	const auto lcd_dir {canon_root / "lcd"};
	const auto kbuild_path {canon_root / "Kbuild"};
	fs::create_directories(klcd_dir);
	fs::create_directories(lcd_dir);

	generate_klcd(klcd_dir, driver_name, module_root.generic_string(), rpc_lists, rpc_pointer_lists);
	generate_lcd(lcd_dir, driver_name, module_root.generic_string(), rpc_lists, rpc_pointer_lists);
	
	const fs::path common_h {canon_root / "common.h"};
	const fs::path common_c {canon_root / "common.c"};
	generate_common_header(common_h, driver_name, headers, rpc_lists, rpc_pointer_lists);

	std::ofstream kbuild {kbuild_path};
	kbuild.exceptions(kbuild.badbit | kbuild.failbit);
	kbuild << "obj-$(LCD_CONFIG_BUILD_" << to_upper(module_root) << "_LCD) += lcd/\n";
	kbuild << "obj-$(LCD_CONFIG_BUILD_" << to_upper(module_root) << "_KLCD) += klcd/\n";
	kbuild << "obj-$(LCD_CONFIG_BUILD_" << to_upper(module_root) << "_BOOT) += boot/\n";
}