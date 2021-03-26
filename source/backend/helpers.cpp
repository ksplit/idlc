#include "helpers.h"

#include <iostream>

void idlc::generate_helpers(std::ostream& file)
{
    file << "#define verbose_debug 0\n";
    file << "#define GLUE_MAX_SLOTS 128\n";
    file << "#define glue_pack(msg, value) glue_pack_impl((msg), (uint64_t)(value))\n";
    file << "#define glue_pack_shadow(msg, value) glue_pack_shadow_impl((msg), (value))\n";
    file << "#define glue_unpack(msg, type) (type)glue_unpack_impl((msg))\n";
    file << "#define glue_unpack_shadow(msg, type) (type)glue_unpack_shadow_impl(glue_unpack(msg, void*));\n";
    file << "#define glue_unpack_new_shadow(msg, type, size) \\\n"
        << "\t(type)glue_unpack_new_shadow_impl(glue_unpack(msg, void*), size)\n\n";

    file << "#ifndef LCD_ISOLATE\n";
    file << "#define glue_unpack_rpc_ptr(msg, name) \\\n"
        << "\tglue_peek(msg) ? (fptr_##name)glue_unpack_rpc_ptr_impl(glue_unpack(msg, void*), "
        << "LCD_DUP_TRAMPOLINE(trmp_##name), LCD_TRAMPOLINE_SIZE(trmp_##name)) : NULL\n\n";

    file << "#else\n";
    file << "#define glue_unpack_rpc_ptr(msg, name) NULL;"
        << " glue_user_panic(\"Trampolines cannot be used on LCD side\")\n";
    file << "#endif\n\n";

    file << "#define glue_peek(msg) glue_peek_impl(msg)\n";
    file << "#define glue_call_server(msg, rpc_id) \\\n"
        << "\tmsg->slots[0] = msg->position; msg->position = 0; glue_user_call_server(msg->slots, rpc_id);\n\n";

    file << "#define glue_call_client(msg, rpc_id) \\\n"
        << "\tmsg->slots[0] = msg->position; msg->position = 0; glue_user_call_client(msg->slots, rpc_id);\n\n";

    file << "\n";
    file << "void glue_user_init(void);\n";
    file << "void glue_user_panic(const char* msg);\n";
    file << "void glue_user_trace(const char* msg);\n";
    file << "void* glue_user_map_to_shadow(const void* obj);\n";
    file << "const void* glue_user_map_from_shadow(const void* shadow);\n";
    file << "void glue_user_add_shadow(const void* ptr, void* shadow);\n";
    file << "void* glue_user_alloc(size_t size);\n";
    file << "void glue_user_free(void* ptr);\n";
    file << "void glue_user_call_server(uint64_t* data, size_t rpc_id);\n";
    file << "void glue_user_call_client(uint64_t* data, size_t rpc_id);\n";
    file << "\n";
    file << "struct glue_message {\n";
    file << "\tuint64_t slots[GLUE_MAX_SLOTS];\n";
    file << "\tuint64_t position;\n";
    file << "};\n\n";
    file << "extern struct glue_message shared_buffer;\n";
    file << "\n";
    file << "static inline struct glue_message* glue_init_msg(void)\n{\n";
    file << "\tshared_buffer.position = 0;\n";
    file << "\treturn &shared_buffer;\n";
    file << "}\n";
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
    file << "static inline void glue_pack_impl(struct glue_message* msg, uint64_t value)\n";
    file << "{\n";
    file << "\tif (msg->position >= GLUE_MAX_SLOTS)\n";
    file << "\t\tglue_user_panic(\"Glue message was too large\");\n";
    file << "\tmsg->slots[msg->position++ + 1] = value;\n";
    file << "}\n";
    file << "\n";
    file << "static inline uint64_t glue_unpack_impl(struct glue_message* msg)\n";
    file << "{\n";
    file << "\tif (msg->position >= msg->slots[0])\n";
    file << "\t\tglue_user_panic(\"Unpacked past end of glue message\");\n";
    file << "\treturn msg->slots[msg->position++ + 1];\n";
    file << "}\n";
    file << "\n";
    file << "static inline uint64_t glue_peek_impl(struct glue_message* msg)\n";
    file << "{\n";
    file << "\tif (msg->position >= msg->slots[0])\n";
    file << "\t\tglue_user_panic(\"Peeked past end of glue message\");\n";
    file << "\treturn msg->slots[msg->position + 2];\n";
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
    file << "static inline void glue_pack_shadow_impl(struct glue_message* msg, const void* ptr)\n";
    file << "{\n";
    file << "\tglue_pack(msg, ptr ? glue_user_map_from_shadow(ptr) : NULL);\n";
    file << "}\n";
}
