#include <algorithm>
#include <fstream>
#include <tuple>
#include <vector>

#include <gsl/gsl>

#include "../ast/ast.h"
#include "../ast/pgraph.h"
#include "../ast/pgraph_walk.h"
#include "../frontend/analysis.h"
#include "../utility.h"
#include "c_specifiers.h"
#include "walks.h"
#include "helpers.h"

// TODO: possibly split this module, it's by far the biggest one in the compiler
// TODO: there is a certain illegal range of ints that cannot be reversibly casted
// TODO: bug where we try and overwrite an existing, const value, but this should never happen in sane code?
// FIXME: annotations are only valid at the leaves!

namespace idlc {
    namespace {
        using projection_vec = std::vector<gsl::not_null<projection*>>;

        class visitor_proto_walk : public pgraph_walk<visitor_proto_walk> {
        public:
            visitor_proto_walk(std::ostream& os) : m_stream {os} {}

            bool visit_projection(projection& node)
            {
                m_stream << "void " << node.caller_marshal_visitor << "(\n"
                    << "\tsize_t* __pos,\n"
                    << "\tstruct fipc_message* __msg,\n"
                    << "\tstruct ext_registers* __ext,\n";
                
                if (node.def->parent)
                    m_stream << "\tstruct " << node.def->parent->ctx_id << " const* call_ctx,\n";
                    
                m_stream << "\tstruct " << node.real_name
                    << " const* ptr);\n\n";

                m_stream << "void " << node.callee_unmarshal_visitor << "(\n"
                    << "\tsize_t* __pos,\n"
                    << "\tconst struct fipc_message* __msg,\n"
                    << "\tconst struct ext_registers* __ext,\n";
                    
                if (node.def->parent)
                    m_stream << "\tstruct " << node.def->parent->ctx_id << " const* call_ctx,\n";
                    
                m_stream << "\tstruct " << node.real_name
                    << "* ptr);\n\n";

                m_stream << "void " << node.callee_marshal_visitor << "(\n"
                    << "\tsize_t* __pos,\n"
                    << "\tstruct fipc_message* __msg,\n"
                    << "\tstruct ext_registers* __ext,\n";
                    
                if (node.def->parent)
                    m_stream << "\tstruct " << node.def->parent->ctx_id << " const* call_ctx,\n";
                    
                m_stream << "\tstruct " << node.real_name
                    << " const* ptr);\n\n";

                m_stream << "void " << node.caller_unmarshal_visitor << "(\n"
                    << "\tsize_t* __pos,\n"
                    << "\tconst struct fipc_message* __msg,\n"
                    << "\tconst struct ext_registers* __ext,\n";
                    
                if (node.def->parent)
                    m_stream << "\tstruct " << node.def->parent->ctx_id << " const* call_ctx,\n";
                    
                m_stream << "\tstruct " << node.real_name
                    << "* ptr);\n\n";

                return true;
            }

        private:
            std::ostream& m_stream;
        };

        void generate_visitor_prototypes(std::ostream& file, projection_vec_view projections)
        {
            visitor_proto_walk visit_walk {file};
            for (const auto& proj : projections)
                visit_walk.visit_projection(*proj);
        }

        void generate_contexts(std::ostream& file, rpc_vec_view rpcs)
        {
            for (const auto& rpc : rpcs) {
                file << "struct " << rpc->ctx_id << " {\n";
                for (gsl::index i {}; i < rpc->arg_pgraphs.size(); ++i)
                    file << "\t" << rpc->arg_pgraphs.at(i)->c_specifier << " " << rpc->arguments->at(i)->name << ";\n";

                file << "};\n\n";
            }
        }

        void generate_common_header(rpc_vec_view rpcs, projection_vec_view projections)
        {
            std::ofstream file {"common.h"};
            file.exceptions(file.badbit | file.failbit);

            file << "#ifndef COMMON_H\n#define COMMON_H\n\n";
            file << "#include <liblcd/trampoline.h>\n";
            file << "#include <libfipc.h>\n";
            file << "#include <liblcd/boot_info.h>\n";
            file << "#include <asm/cacheflush.h>\n";
            file << "#include <lcd_domains/microkernel.h>\n";
            file << "#include <liblcd/liblcd.h>\n";
            file << "\n";
            file << "#include \"glue_user.h\"\n";
            file << "\n";
            generate_helpers(file);
            file << "\n";
            file << "enum RPC_ID {\n";

            // Add MODULE_{INIT,EXIT} enums that are used to load and teardown
            // the module
            file << "\tMODULE_INIT,\n";
            file << "\tMODULE_EXIT,\n";
            file << "\tRPC_ID_shared_mem_init,\n";

            for (const auto& rpc : rpcs)
                file << "\t" << rpc->enum_id << ",\n";

            file << "};\n\n";

            file << "int try_dispatch(enum RPC_ID id, struct fipc_message* __msg, struct ext_registers* __ext);\n\n";

            for (const auto& rpc : rpcs) {
                if (rpc->kind == rpc_def_kind::indirect) {
                    file << "typedef " << rpc->ret_string << " (*" << rpc->typedef_id << ")("
                        << rpc->args_string << ");\n";

                    file << "typedef " << rpc->ret_string << " (*" << rpc->impl_typedef_id << ")(" << rpc->typedef_id
                        << " target, " << rpc->args_string << ");\n\n";

                    file << "LCD_TRAMPOLINE_DATA(" << rpc->trmp_id << ")\n";
                    file << rpc->ret_string << " "
                        << "LCD_TRAMPOLINE_LINKAGE(" << rpc->trmp_id << ") "
                        << rpc->trmp_id << "(" << rpc->args_string << ");\n\n";
                }
            }

            generate_contexts(file, rpcs);
            generate_visitor_prototypes(file, projections);
            file << "\n#endif\n";
        }

        using root_vec = std::vector<std::tuple<std::string, value*>>;

        struct marshal_roots {
            ident return_value;
            root_vec arguments;
        };

        bool is_return(const value& node)
        {
            return !std::get_if<none>(&node.type);
        }

        annotation_kind get_ptr_annotation(const value& node)
        {
            auto p = std::get_if<node_ref<pointer>>(&node.type);
            if (p)
                return p->get()->pointer_annots.kind;
            else
                return annotation_kind::use_default;
        }
        /*
            Some explanation: C usually has two different ways of accessing variables, depnding on if they're a value
            or a pointer, i.e. foo_ptr->a_field vs. foo_ptr.a_field. To avoid unnecessary copying, and also because in
            come cases (arrays) copying the variable would be more complex than an assignment, we'd rather work with
            pointers in marshaling. But, arguments to functions start as values. I didn't want to maintain code paths
            to correctly track what "kind" of variable we're marshaling to access it correctly, so this function
            creates pointers to all the arguments of the RPC. That way the marshaling system can be written to only
            deal with pointers.
        */
        template<marshal_side side>
        marshal_roots generate_root_ptrs(rpc_def& rpc, std::ostream& os)
        {
            const auto n_args = gsl::narrow<gsl::index>(rpc.arg_pgraphs.size());
            root_vec roots;
            roots.reserve(n_args);
            for (gsl::index i {}; i < n_args; ++i) {
                const auto& name = rpc.arguments->at(i)->name;
                const auto& pgraph = rpc.arg_pgraphs.at(i);
                if (flags_set(pgraph->value_annots, annotation_kind::unused))
                    continue;

                const auto& specifier = pgraph->c_specifier;
                auto ptr_name = concat(name, "_ptr");

                // TODO: Doesn't really work for multi-dimensional arrays
                auto p = std::get_if<node_ref<pointer>>(&pgraph->type);

                if (p && (side == marshal_side::callee)) {
                    auto p_array = std::get_if<node_ref<static_array>>(&p->get()->referent->type);
                    auto raw_specifier = p->get()->referent->c_specifier;

                    if (flags_set(p->get()->pointer_annots.kind, annotation_kind::alloc_stack_callee)) {
                        if (p_array) {
                            os << "\t" << raw_specifier << " __"  << name << "[" << p_array->get()->size << "];\n";
                            os << "\t" << specifier << " "  << name << " = __" << name << ";\n";
                        } else {
                            os << "\t" << raw_specifier << " __"  << name << ";\n";
                            os << "\t" << specifier << " "  << name << " = &__" << name << ";\n";
                        }
                        os << "\t" << specifier << "* " << ptr_name << " = &" << name << ";\n";
                    } else {
                        os << "\t" << specifier << "* " << ptr_name << " = &" << name << ";\n";
                    }
                } else {
                    os << "\t" << specifier << "* " << ptr_name << " = &" << name << ";\n";
                }
                roots.emplace_back(std::move(ptr_name), pgraph.get());
            }

            if (is_return(*rpc.ret_pgraph)) {
                if (flags_set(get_ptr_annotation(*rpc.ret_pgraph), annotation_kind::ioremap_caller)) {
                    switch (side) {
                    case marshal_side::caller:
                        os << "\tcptr_t ioremap_cptr;\n";
                        os << "\tgpa_t ioremap_gpa;\n";
                        os << "\tuint64_t ioremap_len;\n";
                        break;
                    case marshal_side::callee:
                        os << "\tcptr_t resource_cptr;\n";
                        os << "\t__maybe_unused unsigned int resource_len;\n";
                        os << "\tcptr_t lcd_resource_cptr;\n";
                        break;
                    }
                }
                os << "\t" << rpc.ret_pgraph->c_specifier << " ret = 0;\n";
                os << "\t" << rpc.ret_pgraph->c_specifier << "* ret_ptr = &ret;\n";
            }

            os << "\t\n";

            return {"ret_ptr", roots};
        }

        // TODO: this is confusing, document it
        template<marshal_role role, marshal_side side>
        auto generate_root_ptrs(
            std::ostream& os,
            projection& projection,
            absl::string_view source_var)
        {
            constexpr auto is_const_context = role == marshal_role::marshaling;
            std::vector<std::tuple<std::string, value*>> roots;
            roots.reserve(projection.fields.size());
            const ref_vec<proj_field>* field_vec;

            if (projection.def && projection.def->fields)
                field_vec = projection.def->fields.get();

            gsl::index i {0};
            for (const auto& [name, type] : projection.fields) {
                // This is an identical check to that conducted in the marshaling walks
                if (!should_walk<role, side>(*type))
                    continue;

                const auto& [ast_type, width] = *(*field_vec)[i++];
                // cannot take address of bitfields. Handle it with plain variables
                if (width > 0) {
                    const auto bitfield_name = concat("__", name);
                    const auto bitfield_ptr_name = concat(bitfield_name, "_ptr");
                    os << "\t" << type->c_specifier << " " << bitfield_name << " = " << source_var << "->" << name
                        << ";\n";

                    const auto specifier = concat(type->c_specifier, is_const_context ? " const*" : "*");
                    const auto assign = std::get_if<node_ref<static_array>>(&type->type) ? " = " : " = &";
                    os << "\t" << specifier << " " << bitfield_ptr_name << assign << bitfield_name << ";\n";
                    roots.emplace_back(std::move(bitfield_ptr_name), type.get());
                } else {
                    const auto specifier = concat(type->c_specifier, is_const_context ? " const*" : "*");
                    const auto ptr_name = concat(name, "_ptr");
                    const auto assign = std::get_if<node_ref<static_array>>(&type->type) ? " = " : " = &";
                    os << "\t" << specifier << " " << ptr_name << assign << source_var << "->" << name << ";\n";
                    roots.emplace_back(std::move(ptr_name), type.get());
                }
            }

            os << "\n";

            return roots;
        }

        enum class rpc_side {
            server,
            client
        };

        auto generate_caller_glue_prologue(std::ostream& os, rpc_def& rpc)
        {
            os << "\tstruct fipc_message __buffer = {0};\n";
            os << "\tstruct fipc_message *__msg = &__buffer;\n";
            os << "\tstruct ext_registers* __ext = get_register_page(smp_processor_id());\n";
            os << "\tsize_t n_pos = 0;\n";
            os << "\tsize_t* __pos = &n_pos;\n\n";

            const auto n_args = rpc.arg_pgraphs.size();
            const auto roots = generate_root_ptrs<marshal_side::caller>(rpc, os);

            os << "\t__maybe_unused const struct " << rpc.ctx_id << " call_ctx = {" << rpc.params_string << "};\n";
            os << "\t__maybe_unused const struct " << rpc.ctx_id << " *ctx = &call_ctx;\n\n";

            os << "\t(void)__ext;\n\n";

            // Add verbose printk's while entering
            os << "\tif (verbose_debug) {\n";
            os << "\t\tprintk(\"%s:%d, entered!\\n\", __func__, __LINE__);\n" << "\t}\n\n";

            if (rpc.kind == rpc_def_kind::indirect) {
                // make sure we marshal the target pointer before everything
                os << "\tglue_pack(__pos, __msg, __ext, target);\n";
            }

            if (is_return(*rpc.ret_pgraph)) {
                if (flags_set(get_ptr_annotation(*rpc.ret_pgraph), annotation_kind::ioremap_caller)) {
                    os << "\t{\n";
                    os << "\t\tlcd_cptr_alloc(&ioremap_cptr);\n";
                    os << "\t\tglue_pack(__pos, __msg, __ext, cptr_val(ioremap_cptr));\n";
                    os << "\t}\n\n";
                }
            }

            for (const auto& [name, type] : roots.arguments) {
                os << "\t{\n";
                marshaling_walk<marshal_side::caller> arg_marshal {os, name, 2};
                arg_marshal.visit_value(*type);
                os << "\t}\n\n";
            }

            return roots;
        }

        void generate_caller_glue_epilogue(std::ostream& os, rpc_def& rpc, marshal_roots roots)
        {
            os << "\t*__pos = 0;\n";

            if (is_return(*rpc.ret_pgraph)) {
                if (flags_set(get_ptr_annotation(*rpc.ret_pgraph), annotation_kind::ioremap_caller)) {
                        os << "\t{\n";
                        os << "\t\tint __ret;\n";
                        os << "\t\tioremap_len = glue_unpack(__pos, __msg, __ext, uint64_t);\n";
                        os << "\t\t__ret = lcd_ioremap_phys(ioremap_cptr, ioremap_len, &ioremap_gpa);\n";
                        os << "\t\tif (__ret) {\n";
                        os << "\t\t\tLIBLCD_ERR(\"failed to remap bar region\");\n";
                        os << "\t\t}\n";
                        os << "\t\t*ret_ptr = lcd_ioremap(gpa_val(ioremap_gpa), ioremap_len);\n";
                        os << "\t\tif (!*ret_ptr) {\n";
                        os << "\t\t\tLIBLCD_ERR(\"failed to ioremap virt\");\n";
                        os << "\t\t}\n";
                        os << "\t}\n\n";
                    }
            }

            for (const auto& [name, type] : roots.arguments) {
                os << "\t{\n";
                unmarshaling_walk<marshal_side::caller> arg_unremarshal {os, name, 2};
                arg_unremarshal.visit_value(*type);
                os << "\t}\n\n";
            }

            if (is_return(*rpc.ret_pgraph)) {
                // Unmarshal return pointer only if it's NOT an ioremap annotation
                if (!flags_set(get_ptr_annotation(*rpc.ret_pgraph), annotation_kind::ioremap_caller)) {
                    os << "\t{\n";
                    unmarshaling_walk<marshal_side::caller> ret_unmarshal {os, roots.return_value, 2};
                    ret_unmarshal.visit_value(*rpc.ret_pgraph);
                    os << "\t}\n\n";
                }
            }

            // Add verbose printk's while returning
            os << "\tif (verbose_debug) {\n";
            os << "\t\tprintk(\"%s:%d, returned!\\n\", __func__, __LINE__);\n" << "\t}\n";
            os << (is_return(*rpc.ret_pgraph) ? concat("\treturn ret;\n") : "");
        }

        template<rpc_side side>
        void generate_caller_glue(rpc_def& rpc, std::ostream& os)
        {
            const auto roots = generate_caller_glue_prologue(os, rpc);
            switch (side) {
            case rpc_side::client:
                os << "\tglue_call_server(__pos, __msg, " << rpc.enum_id << ");\n\n";
                break;

            case rpc_side::server:
                os << "\tglue_call_client(__pos, __msg, " << rpc.enum_id << ");\n\n";
                break;
            }

            generate_caller_glue_epilogue(os, rpc, roots);
        }

        // TODO: large and complex, needs splitting
        void generate_callee_glue(rpc_def& rpc, std::ostream& os)
        {
            os << "\tsize_t n_pos = 0;\n";
            os << "\tsize_t* __pos = &n_pos;\n\n";

            if (rpc.kind == rpc_def_kind::indirect) {
                os << "\t" << rpc.typedef_id << " function_ptr = glue_unpack(__pos, __msg, __ext, " << rpc.typedef_id
                    << ");\n";
            }

            const auto n_args = gsl::narrow<gsl::index>(rpc.arg_pgraphs.size());
            for (gsl::index i {}; i < n_args; ++i) {
                const auto& pgraph = rpc.arg_pgraphs.at(i);
                const auto& type = pgraph->c_specifier;
                const auto name = rpc.arguments->at(i)->name;

                auto p = std::get_if<node_ref<pointer>>(&pgraph->type);
                if (p && flags_set(p->get()->pointer_annots.kind, annotation_kind::alloc_stack_callee))
                    continue;
                os << "\t" << type << " " << name << " = 0;\n";
            }

            const auto roots = generate_root_ptrs<marshal_side::callee>(rpc, os);

            os << "\t__maybe_unused struct " << rpc.ctx_id << " call_ctx = {" << rpc.params_string << "};\n";
            os << "\t__maybe_unused struct " << rpc.ctx_id << " *ctx = &call_ctx;\n\n";

            // Add verbose printk's while entering
            os << "\tif (verbose_debug) {\n";
            os << "\t\tprintk(\"%s:%d, entered!\\n\", __func__, __LINE__);\n" << "\t}\n\n";

            // Unmarshal cptr from the driver domain
            if (is_return(*rpc.ret_pgraph)) {
                if (flags_set(get_ptr_annotation(*rpc.ret_pgraph), annotation_kind::ioremap_caller)) {
                    os << "\t{\n";
                    os << "\t\tlcd_resource_cptr.cptr = glue_unpack(__pos, __msg, __ext, uint64_t);\n";
                    os << "\t}\n\n";
                }
            }

            for (const auto& [name, type] : roots.arguments) {
                os << "\t{\n";
                unmarshaling_walk<marshal_side::callee> arg_unmarshal {os, name, 2};
                arg_unmarshal.visit_value(*type);
                os << "\t}\n\n";
            }

            os << "\t";
            if (is_return(*rpc.ret_pgraph))
                os << "ret = ";

            if (flags_set(get_ptr_annotation(*rpc.ret_pgraph), annotation_kind::ioremap_caller)) {
                std::string phys_addr;
                if ((rpc.name == std::string("pci_iomap")) || (rpc.name == std::string("pci_ioremap_bar"))) {
                    phys_addr = "pci_resource_start(pdev, bar)";
                } else if (rpc.name == std::string("ioremap_nocache")) {
                    phys_addr = "phys_addr";
                }
                os << "(void *) " << phys_addr <<";\n\n";
            } else {

                const auto impl_name = (rpc.kind == rpc_def_kind::direct) ? rpc.name : "function_ptr";
                os << impl_name << "(" << rpc.params_string << ");\n\n";
            }

            os << "\t*__pos = 0;\n";


            // Marshal the resource len back to the caller domain for ioremapping the region
            if (is_return(*rpc.ret_pgraph)) {
                if (flags_set(get_ptr_annotation(*rpc.ret_pgraph), annotation_kind::ioremap_caller)) {
                    ident resource_len = "unknown";

                    if ((rpc.name == std::string("pci_iomap")) || (rpc.name == std::string("pci_ioremap_bar"))) {
                        resource_len = "pci_resource_len(pdev, bar)";
                    } else if (rpc.name == std::string("ioremap_nocache")) {
                        resource_len = "size";
                    }

                    os << "\n\t{\n";
                    os << "\t\tlcd_volunteer_dev_mem(__gpa((uint64_t)*ret_ptr), get_order(" << resource_len
                        << "), &resource_cptr);\n";

                    os << "\t\tcopy_msg_cap_vmfunc(current->lcd, current->vmfunc_lcd, resource_cptr, lcd_resource_cptr)"
                        << ";\n";
                        
                    os << "\t\tglue_pack(__pos, __msg, __ext, " << resource_len << ");\n";
                    os << "\t}\n\n";
                }
            }

            for (const auto& [name, type] : roots.arguments) {
                os << "\t{\n";
                marshaling_walk<marshal_side::callee> arg_remarshal {os, name, 2};
                arg_remarshal.visit_value(*type);
                os << "\t}\n\n";
            }
            
            if (is_return(*rpc.ret_pgraph)) {
                // Marshal return pointer only if it's NOT an ioremap annotation
                if (!flags_set(get_ptr_annotation(*rpc.ret_pgraph), annotation_kind::ioremap_caller)) {
                    os << "\t{\n";
                    marshaling_walk<marshal_side::callee> ret_marshal {os, roots.return_value, 2};
                    ret_marshal.visit_value(*rpc.ret_pgraph);
                    os << "\t}\n\n";
                }
            }
        
            os << "\t__msg->regs[0] = *__pos;\n";

            // Add verbose printk's while returning
            os << "\tif (verbose_debug) {\n";
            os << "\t\tprintk(\"%s:%d, returned!\\n\", __func__, __LINE__);\n" << "\t}\n";
        }

        void generate_indirect_rpc_callee(std::ostream& os, rpc_def& rpc)
        {
            os << "void " << rpc.callee_id << "(struct fipc_message* __msg, struct ext_registers* __ext)\n{\n";
            generate_callee_glue(rpc, os);
            os << "}\n\n";            
        }

        void generate_indirect_rpc_caller(std::ostream& os, rpc_def& rpc)
        {
            os << rpc.ret_string << " " << rpc.impl_id << "(" << rpc.typedef_id << " target, "
                << rpc.args_string << ")\n{\n";

            generate_caller_glue<rpc_side::server>(rpc, os);
            os << "}\n\n";

            os << "LCD_TRAMPOLINE_DATA(" << rpc.trmp_id << ")\n";
            os << rpc.ret_string;
            os << " LCD_TRAMPOLINE_LINKAGE(" << rpc.trmp_id << ") ";
            os << rpc.trmp_id << "(" << rpc.args_string << ")\n{\n";

            os << "\tvolatile " << rpc.impl_typedef_id << " impl;\n";
            os << "\t" << rpc.typedef_id << " target;\n";
            os << "\tLCD_TRAMPOLINE_PROLOGUE(target, " << rpc.trmp_id << ");\n";
            os << "\timpl = " << rpc.impl_id << ";\n";
            os << "\treturn impl(target, " << rpc.params_string << ");\n";

            os << "}\n\n";
        }

        template<rpc_side side>
        void generate_dispatch_fn(std::ostream& os, rpc_vec_view rpcs)
        {
            os << "int try_dispatch(enum RPC_ID id, struct fipc_message* __msg, struct ext_registers* __ext)\n";
            os << "{\n";
            os << "\tswitch(id) {\n";
            if constexpr (side == rpc_side::client) {
                os << "\tcase MODULE_INIT:\n";
                os << "\t\tglue_user_trace(\"MODULE_INIT\");\n";
                os << "\t\t__module_lcd_init();\n";
                os << "\t\tshared_mem_init();\n";
                os << "\t\tbreak;\n\n";

                os << "\tcase MODULE_EXIT:\n";
                os << "\t\tglue_user_trace(\"MODULE_EXIT\");\n";
                os << "\t\t__module_lcd_exit();\n";
                os << "\t\tbreak;\n\n";
            }

            if constexpr (side == rpc_side::server) {
                os << "\tcase RPC_ID_shared_mem_init:\n";
                os << "\t\tglue_user_trace(\"shared_mem_init\\n\");\n";
                os << "\t\tshared_mem_init_callee(__msg, __ext);\n";
                os << "\t\tbreak;\n\n";
            }

            for (const auto& rpc : rpcs) {
                if (rpc->kind == rpc_def_kind::direct && side == rpc_side::client)
                    continue;
                
                if (rpc->kind == rpc_def_kind::indirect && side == rpc_side::server)
                    continue;
                
                os << "\tcase " << rpc->enum_id << ":\n";
                os << "\t\tglue_user_trace(\"" << rpc->name << "\\n\");\n";
                os << "\t\t" << rpc->callee_id << "(__msg, __ext);\n";
                os << "\t\tbreak;\n\n";
            }

            os << "\tdefault:\n\t\treturn 0;\n";
            os << "\t}\n\n\treturn 1;\n}\n\n";
        }

        void generate_client(rpc_vec_view rpcs)
        {
            std::ofstream file {"client.c"};
            file.exceptions(file.badbit | file.failbit);

            file << "#include <lcd_config/pre_hook.h>\n\n";
            file << "#include \"common.h\"\n\n";
            file << "#include <lcd_config/post_hook.h>\n\n";
            for (const auto& rpc : rpcs) {
                switch (rpc->kind) {
                case rpc_def_kind::direct:
                    file << rpc->ret_string << " " << rpc->name << "(" << rpc->args_string << ")\n{\n";
                    generate_caller_glue<rpc_side::client>(*rpc, file);
                    file << "}\n\n";
                    break;

                case rpc_def_kind::indirect:
                    generate_indirect_rpc_callee(file, *rpc);
                    break;
                }
            }

            generate_dispatch_fn<rpc_side::client>(file, rpcs);
        }

        void generate_server(rpc_vec_view rpcs)
        {
            std::ofstream file {"server.c"};
            file.exceptions(file.badbit | file.failbit);

            file << "#include <lcd_config/pre_hook.h>\n\n";
            file << "#include \"common.h\"\n\n";
            file << "#include <lcd_config/post_hook.h>\n\n";
            for (const auto& rpc : rpcs) {
                switch (rpc->kind) {
                case rpc_def_kind::indirect:
                    generate_indirect_rpc_caller(file, *rpc);
                    break;

                case rpc_def_kind::direct:
                    file << "void " << rpc->callee_id << "(struct fipc_message* __msg, struct ext_registers* __ext)\n{\n";
                    generate_callee_glue(*rpc, file);
                    file << "}\n\n";
                    break;                    
                }
            }

            generate_dispatch_fn<rpc_side::server>(file, rpcs);
        }

        void generate_linker_script(rpc_vec_view rpcs)
        {
            std::ofstream file {"trampolines.lds.S"};
            file.exceptions(file.badbit | file.failbit);

            file << "#include <liblcd/trampoline_link.h>\n\n";
            file << "SECTIONS{\n";

            for (const auto& rpc : rpcs) {
                if (rpc->kind == rpc_def_kind::indirect)
                    file << "\tLCD_TRAMPOLINE_LINKER_SECTION(" << rpc->trmp_id << ")\n";
            }

            file << "}\n\n";
            file << "INSERT AFTER .text\n";
        }

        // Pre-compute these for convenience when generating function calls or prototypes
        void create_function_strings(rpc_vec_view rpcs)
        {
            for (auto& rpc : rpcs) {
                rpc->ret_string = rpc->ret_pgraph->c_specifier;
                if (rpc->arguments) {
                    bool is_first {true};
                    auto& args_str = rpc->args_string;
                    auto& param_str = rpc->params_string;
                    const auto size = gsl::narrow<gsl::index>(rpc->arguments->size());
                    for (gsl::index i {}; i < size; ++i) {
                        const auto& arg = rpc->arg_pgraphs.at(i);
                        const auto name = rpc->arguments->at(i)->name;
                        if (!is_first) {
                            args_str += ", ";
                            param_str += ", ";
                        }

                        append(args_str, arg->c_specifier, " ", name);
                        param_str += name;
                        is_first = false;
                    }
                }
                else {
                    rpc->args_string = "void"; // GCC doesn't like it if we just leave the arguments list empty
                }
            }
        }

        void generate_caller_marshal_visitor(std::ostream& file, projection& node)
        {
            file << "void " << node.caller_marshal_visitor << "(\n"
                << "\tsize_t* __pos,\n"
                << "\tstruct fipc_message* __msg,\n"
                << "\tstruct ext_registers* __ext,\n";
                
            if (node.def->parent)
                file << "\tstruct " << node.def->parent->ctx_id << " const* ctx,\n";
                
            file << "\tstruct " << node.real_name
                << " const* ptr)\n{\n";

            const auto roots = generate_root_ptrs<marshal_role::marshaling, marshal_side::caller>(file, node, "ptr");
            const auto n_fields = node.fields.size();
            for (const auto& [name, type] : roots) {
                file << "\t{\n";
                marshaling_walk<marshal_side::caller> walk {file, name, 2};
                walk.visit_value(*type);
                file << "\t}\n\n";
            }

            file << "}\n\n";
        }

        template <marshal_side side>
        void fixup_bitfields(std::ostream& file, projection& node, absl::string_view source_var)
        {
            ref_vec<proj_field> field_vec;
            gsl::index i {0};

            if (node.def && node.def->fields) {
                field_vec = *node.def->fields;
            }

            file << "\t{\n";
            for (const auto& [name, type] : node.fields) {
                const auto & [_, width] = *field_vec[i++];
                auto bitfield_ptr_name = std::string("__") + name + std::string("_ptr");
                if (!should_walk<marshal_role::unmarshaling, side>(*type))
                    continue;
                if (width > 0)
                    file << "\t\t" << "ptr" << "->" << name << " = *" << bitfield_ptr_name << ";\n";
            }
            file << "\t}\n";
        }

        void generate_callee_unmarshal_visitor(std::ostream& file, projection& node)
        {
            file << "void " << node.callee_unmarshal_visitor << "(\n"
                << "\tsize_t* __pos,\n"
                << "\tconst struct fipc_message* __msg,\n"
                << "\tconst struct ext_registers* __ext,\n";
                
            if (node.def->parent)
                file << "\tstruct " << node.def->parent->ctx_id << " const* ctx,\n";
                
            file << "\tstruct " << node.real_name
                << "* ptr)\n{\n";

            const auto roots = generate_root_ptrs<marshal_role::unmarshaling, marshal_side::callee>(file, node, "ptr");
            const auto n_fields = node.fields.size();
            for (const auto& [name, type] : roots) {
                file << "\t{\n";
                unmarshaling_walk<marshal_side::callee> walk {file, name, 2};
                walk.visit_value(*type);
                file << "\t}\n\n";
            }

            fixup_bitfields<marshal_side::callee>(file, node, "ptr");

            file << "}\n\n";
        }

        void generate_callee_marshal_visitor(std::ostream& file, projection& node)
        {
            file << "void " << node.callee_marshal_visitor << "(\n"
                << "\tsize_t* __pos,\n"
                << "\tstruct fipc_message* __msg,\n"
                << "\tstruct ext_registers* __ext,\n";
                
            if (node.def->parent)
                file << "\tstruct " << node.def->parent->ctx_id << " const* ctx,\n";
                
            file << "\tstruct " << node.real_name
                << " const* ptr)\n{\n";

            const auto roots = generate_root_ptrs<marshal_role::marshaling, marshal_side::callee>(file, node, "ptr");
            const auto n_fields = node.fields.size();
            for (const auto& [name, type] : roots) {
                file << "\t{\n";
                marshaling_walk<marshal_side::callee> walk {file, name, 2};
                walk.visit_value(*type);
                file << "\t}\n\n";
            }

            file << "}\n\n";
        }

        void generate_caller_unmarshal_visitor(std::ostream& file, projection& node)
        {
            file << "void " << node.caller_unmarshal_visitor << "(\n"
                << "\tsize_t* __pos,\n"
                << "\tconst struct fipc_message* __msg,\n"
                << "\tconst struct ext_registers* __ext,\n";
                
            if (node.def->parent)
                file << "\tstruct " << node.def->parent->ctx_id << " const* ctx,\n";
                
            file << "\tstruct " << node.real_name
                << "* ptr)\n{\n";

            const auto roots = generate_root_ptrs<marshal_role::unmarshaling, marshal_side::caller>(file, node, "ptr");
            const auto n_fields = node.fields.size();
            for (const auto& [name, type] : roots) {
                file << "\t{\n";
                unmarshaling_walk<marshal_side::caller> walk {file, name, 2};
                walk.visit_value(*type);
                file << "\t}\n\n";
            }

            fixup_bitfields<marshal_side::caller>(file, node, "ptr");

            file << "}\n\n";
        }

        void generate_common_source(rpc_vec_view rpcs, projection_vec_view projections)
        {
            std::ofstream file {"common.c"};
            file.exceptions(file.badbit | file.failbit);
            file << "#include <lcd_config/pre_hook.h>\n\n";
            file << "#include \"common.h\"\n\n";
            file << "#include <lcd_config/post_hook.h>\n\n";
            for (const auto& projection : projections) {
                // TODO: make these optional
                generate_caller_marshal_visitor(file, *projection);
                generate_callee_unmarshal_visitor(file, *projection);
                generate_callee_marshal_visitor(file, *projection);
                generate_caller_unmarshal_visitor(file, *projection);
            }

            file << "\n#ifdef LCD_ISOLATE\n";
            file << "__attribute__((weak)) void shared_mem_init(void) {\n";
            file << "\tLIBLCD_MSG(\"Weak shared_mem_init does nothing! Override if you want!\");\n";
            file << "}\n";
            file << "#else\n";
            file << "__attribute__((weak)) void shared_mem_init_callee(struct fipc_message *__msg, struct ext_registers* __ext) {\n";
            file << "\tLIBLCD_MSG(\"Weak shared_mem_init_callee does nothing! Override if you want!\");\n";
            file << "}\n";
            file << "#endif\t/* LCD_ISOLATE */\n\n";
        }

        class projection_collection_walk : public pgraph_walk<projection_collection_walk> {
        public:
            projection_collection_walk(projection_vec& projections) :
                m_projections {projections}
            {}

            bool visit_projection(projection& node)
            {
                const auto last = m_projections.end();
                // Traversal code protects us from traversing a projection multiple times, but not from visiting it
                if (std::find(m_projections.begin(), last, &node) == last)
                    m_projections.emplace_back(&node);

                return traverse(*this, node);
            }

        private:
            projection_vec& m_projections;
        };

        projection_vec get_projections(rpc_vec_view rpcs)
        {
            projection_vec projs {};
            projection_collection_walk walk {projs};
            for (const auto& rpc : rpcs) {
                walk.visit_value(*rpc->ret_pgraph);
                for (const auto& arg : rpc->arg_pgraphs)
                    walk.visit_value(*arg);
            }

            return projs;
        }
    }

    void generate(rpc_vec_view rpcs)
    {
        const auto projections = idlc::get_projections(rpcs);
        populate_c_type_specifiers(rpcs, projections);
        create_function_strings(rpcs);
        generate_common_header(rpcs, projections);
        generate_common_source(rpcs, projections);
        generate_client(rpcs);
        generate_server(rpcs);
        generate_linker_script(rpcs);
    }
}
