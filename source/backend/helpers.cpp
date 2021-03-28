#include "helpers.h"

#include <iostream>

void idlc::generate_helpers(std::ostream& file)
{
    file << "#define verbose_debug 0\n";
    file << "#define glue_pack(pos, msg, ext, value) glue_pack_impl((pos), (msg), (ext), (uint64_t)(value))\n";
    file << "#define glue_pack_shadow(pos, msg, ext, value) glue_pack_shadow_impl((pos), (msg), (ext), (value))\n";
    file << "#define glue_unpack(pos, msg, ext, type) (type)glue_unpack_impl((pos), (msg), (ext))\n";
    file << "#define glue_unpack_shadow(pos, msg, ext, type) \\\n"
        << "(type)glue_unpack_shadow_impl(glue_unpack(pos, msg, ext, void*));\n\n";

    file << "#define glue_unpack_new_shadow(pos, msg, ext, type, size) \\\n"
        << "\t(type)glue_unpack_new_shadow_impl(glue_unpack(pos, msg, ext, void*), size)\n\n";

    file << "#ifndef LCD_ISOLATE\n";
    file << "#define glue_unpack_rpc_ptr(pos, msg, ext, name) \\\n"
        << "\tglue_peek(pos, msg, ext) ? (fptr_##name)glue_unpack_rpc_ptr_impl(glue_unpack(pos, msg, ext, void*), "
        << "LCD_DUP_TRAMPOLINE(trmp_##name), LCD_TRAMPOLINE_SIZE(trmp_##name)) : NULL\n\n";

    file << "#else\n";
    file << "#define glue_unpack_rpc_ptr(pos, msg, ext, name) NULL;"
        << " glue_user_panic(\"Trampolines cannot be used on LCD side\")\n";
    file << "#endif\n\n";

    file << "#define glue_peek(pos, msg, ext) glue_peek_impl(pos, msg, ext)\n";
    file << "#define glue_call_server(pos, msg, rpc_id) \\\n"
        << "\tmsg->regs[0] = *pos; *pos = 0; glue_user_call_server(msg, rpc_id);\n\n";

    file << "#define glue_call_client(pos, msg, rpc_id) \\\n"
        << "\tmsg->regs[0] = *pos; *pos = 0; glue_user_call_client(msg, rpc_id);\n\n";

    file << "void glue_user_init(void);\n";
    file << "void glue_user_panic(const char* msg);\n";
    file << "void glue_user_trace(const char* msg);\n";
    file << "void* glue_user_map_to_shadow(const void* obj);\n";
    file << "const void* glue_user_map_from_shadow(const void* shadow);\n";
    file << "void glue_user_add_shadow(const void* ptr, void* shadow);\n";
    file << "void* glue_user_alloc(size_t size);\n";
    file << "void glue_user_free(void* ptr);\n";
    file << "void glue_user_call_server(struct fipc_message* msg, size_t rpc_id);\n";
    file << "void glue_user_call_client(struct fipc_message* msg, size_t rpc_id);\n";
    file << "\n";
    file << "static inline void* glue_unpack_rpc_ptr_impl(void* target, "
        << "struct lcd_trampoline_handle* handle, size_t size)\n";
    
    file << "{\n";
    file << "\tif (!target)\n\t\tglue_user_panic(\"Target was NULL\");\n\n";
    file << "\tif (!handle)\n\t\tglue_user_panic(\"Trmp was NULL\");\n\n";
    file << "\tset_memory_x(((unsigned long)handle) & PAGE_MASK, ALIGN(size, PAGE_SIZE) >> PAGE_SHIFT);\n";
    file << "\thandle->hidden_args = target;\n";
    file << "\treturn LCD_HANDLE_TO_TRAMPOLINE(handle);\n";
    file << "}\n";
    file << "\n";
    file << "static inline void\n"
        << "glue_pack_impl(size_t* pos, struct fipc_message* msg, struct ext_registers* ext, uint64_t value)\n";

    file << "{\n";
    file << "\tif (*pos >= 128)\n";
    file << "\t\tglue_user_panic(\"Glue message was too large\");\n";
    file << "\tif (*pos < 6)\n";
    file << "\t\tmsg->regs[(*pos)++ + 1] = value;\n";
    file << "\telse\n";
    file << "\t\text->regs[(*pos)++ + 1] = value;\n";
    file << "}\n";
    file << "\n";
    file << "static inline uint64_t\n"
        << "glue_unpack_impl(size_t* pos, const struct fipc_message* msg, const struct ext_registers* ext)\n";

    file << "{\n";
    file << "\tif (*pos >= msg->regs[0])\n";
    file << "\t\tglue_user_panic(\"Unpacked past end of glue message\");\n";
    file << "\tif (*pos < 6)\n";
    file << "\t\treturn msg->regs[(*pos)++ + 1];\n";
    file << "\telse\n";
    file << "\t\treturn ext->regs[(*pos)++ + 1];\n";
    file << "}\n";
    file << "\n";
    file << "static inline uint64_t\n"
        << "glue_peek_impl(size_t* pos, const struct fipc_message* msg, const struct ext_registers* ext)\n";

    file << "{\n";
    file << "\tif (*pos >= msg->regs[0])\n";
    file << "\t\tglue_user_panic(\"Peeked past end of glue message\");\n";
    file << "\tif (*pos < 5)\n";
    file << "\t\treturn msg->regs[*pos + 2];\n";
    file << "\telse\n";
    file << "\t\treturn ext->regs[*pos + 2];\n";
    file << "}\n";
    file << "\n";
    file << "static inline void* glue_unpack_new_shadow_impl(const void* ptr, size_t size)\n";
    file << "{\n";
    file << "\tvoid* shadow = 0;";
    file << "\tif (!ptr)\n";
    file << "\t\treturn NULL;\n";
    file << "\n";
    file << "\tshadow = glue_user_alloc(size);\n";
    file << "\tglue_user_add_shadow(ptr, shadow);\n";
    file << "\treturn shadow;\n";
    file << "}\n";
    file << "\n";
    file << "static inline void* glue_unpack_shadow_impl(const void* ptr)\n";
    file << "{\n";
    file << "\treturn ptr ? glue_user_map_to_shadow(ptr) : NULL;\n";
    file << "}\n";
    file << "\n";
    file << "static inline void glue_pack_shadow_impl(size_t* pos, struct fipc_message* msg, struct ext_registers* ext, const void* ptr)\n";
    file << "{\n";
    file << "\tglue_pack(pos, msg, ext, ptr ? glue_user_map_from_shadow(ptr) : NULL);\n";
    file << "}\n";
}
