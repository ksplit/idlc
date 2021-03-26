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
                m_stream << "void " << node.caller_marshal_visitor
                    << "(\n\tstruct fipc_message*, struct ext_registers*,\n\tstruct " << node.real_name << " const*);\n\n";

                m_stream << "void " << node.callee_unmarshal_visitor
                    << "(\n\tstruct fipc_message*, struct ext_registers*,\n\tstruct " << node.real_name << "*);\n\n";

                m_stream << "void " << node.callee_marshal_visitor
                    << "(\n\tstruct fipc_message*, struct ext_registers*,\n\tstruct " << node.real_name << " const*);\n\n";

                m_stream << "void " << node.caller_unmarshal_visitor
                    << "(\n\tstruct fipc_message*, struct ext_registers*,\n\tstruct " << node.real_name << "*);\n\n";

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

        void generate_common_header(rpc_vec_view rpcs, projection_vec_view projections)
        {
            std::ofstream file {"common.h"};
            file.exceptions(file.badbit | file.failbit);

            file << "#ifndef COMMON_H\n#define COMMON_H\n\n";
            file << "#include <liblcd/trampoline.h>\n";
            file << "#include <asm/cacheflush.h>\n";
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

            for (const auto& rpc : rpcs)
                file << "\t" << rpc->enum_id << ",\n";

            file << "};\n\n";

            file << "int try_dispatch(enum RPC_ID id, struct fipc_message* msg);\n\n";

            generate_visitor_prototypes(file, projections);
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

            file << "\n#endif\n";
        }

        using root_vec = std::vector<std::tuple<std::string, value*>>;

        struct marshal_roots {
            ident return_value;
            root_vec arguments;
        };

        /*
            Some explanation: C usually has two different ways of accessing variables, depnding on if they're a value
            or a pointer, i.e. foo_ptr->a_field vs. foo_ptr.a_field. To avoid unnecessary copying, and also because in
            come cases (arrays) copying the variable would be more complex than an assignment, we'd rather work with
            pointers in marshaling. But, arguments to functions start as values. I didn't want to maintain code paths
            to correctly track what "kind" of variable we're marshaling to access it correctly, so this function
            creates pointers to all the arguments of the RPC. That way the marshaling system can be written to only
            deal with pointers.
        */
        marshal_roots generate_root_ptrs(rpc_def& rpc, std::ostream& os)
        {
            const auto n_args = gsl::narrow<gsl::index>(rpc.arg_pgraphs.size());
            root_vec roots;
            roots.reserve(n_args);
            for (gsl::index i {}; i < n_args; ++i) {
                const auto& name = rpc.arguments->at(i)->name;
                const auto& pgraph = rpc.arg_pgraphs.at(i);
                const auto& specifier = pgraph->c_specifier;
                auto ptr_name = concat(name, "_ptr");
                os << "\t" << specifier << "* " << ptr_name << " = &" << name << ";\n";
                roots.emplace_back(std::move(ptr_name), pgraph.get());
            }

            if (rpc.ret_pgraph) {
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
            std::vector<std::tuple<std::string, value*>> roots;
            roots.reserve(projection.fields.size());
            const auto is_const_context {role == marshal_role::marshaling};
            for (const auto& [name, type] : projection.fields) {
                // This is an identical check to that conducted in the marshaling walks
                if (!should_walk<role, side>(*type))
                    continue;

                const auto specifier = concat(type->c_specifier, is_const_context ? " const*" : "*");
                auto ptr_name = concat(name, "_ptr");
                os << "\t" << specifier << " " << ptr_name << " = &" << source_var << "->"
                    << name << ";\n";

                roots.emplace_back(std::move(ptr_name), type.get());
            }

            os << "\t\n";

            return roots;
        }

        enum class rpc_side {
            server,
            client
        };

        auto generate_caller_glue_prologue(std::ostream& os, rpc_def& rpc)
        {
            os << "\tstruct fipc_message buffer = {0};";
            os << "\tstruct fipc_message *msg = &buffer;\n\n";

            const auto n_args = rpc.arg_pgraphs.size();
            const auto roots = generate_root_ptrs(rpc, os);

            // Add verbose printk's while entering
            os << "\tif (verbose_debug) {\n";
            os << "\t\tprintk(\"%s:%d, entered!\\n\", __func__, __LINE__);\n" << "\t}\n\n";

            if (rpc.kind == rpc_def_kind::indirect)
                os << "\tglue_pack(msg, target);\n"; // make sure we marshal the target pointer before everything

            for (const auto& [name, type] : roots.arguments) {
                marshaling_walk<marshal_side::caller> arg_marshal {os, name, 1};
                arg_marshal.visit_value(*type);
            }

            return roots;
        }

        void generate_caller_glue_epilogue(std::ostream& os, rpc_def& rpc, marshal_roots roots)
        {
            os << "\tmsg->position = 0;\n";

            for (const auto& [name, type] : roots.arguments) {
                unmarshaling_walk<marshal_side::caller> arg_unremarshal {os, name, 1};
                arg_unremarshal.visit_value(*type);
            }

            if (rpc.ret_pgraph) {
                unmarshaling_walk<marshal_side::caller> ret_unmarshal {os, roots.return_value, 1};
                ret_unmarshal.visit_value(*rpc.ret_pgraph);
            }

            // Add verbose printk's while returning
            os << "\tif (verbose_debug) {\n";
            os << "\t\tprintk(\"%s:%d, returned!\\n\", __func__, __LINE__);\n" << "\t}\n";

            os << (rpc.ret_pgraph ? concat("\treturn ret;\n") : "");
        }

        template<rpc_side side>
        void generate_caller_glue(rpc_def& rpc, std::ostream& os)
        {
            const auto roots = generate_caller_glue_prologue(os, rpc);
            switch (side) {
            case rpc_side::client:
                os << "\tglue_call_server(msg, " << rpc.enum_id << ");\n\n";
                break;

            case rpc_side::server:
                os << "\tglue_call_client(msg, " << rpc.enum_id << ");\n\n";
                break;
            }

            generate_caller_glue_epilogue(os, rpc, roots);
        }

        // TODO: large and complex, needs splitting
        void generate_callee_glue(rpc_def& rpc, std::ostream& os)
        {
            if (rpc.kind == rpc_def_kind::indirect)
                os << "\t" << rpc.typedef_id << " function_ptr = glue_unpack(msg, " << rpc.typedef_id << ");\n";

            const auto n_args = gsl::narrow<gsl::index>(rpc.arg_pgraphs.size());
            for (gsl::index i {}; i < n_args; ++i) {
                const auto& type = rpc.arg_pgraphs.at(i)->c_specifier;
                const auto name = rpc.arguments->at(i)->name;
                os << "\t" << type << " " << name << " = 0;\n";
            }

            const auto roots = generate_root_ptrs(rpc, os);

            // Add verbose printk's while entering
            os << "\tif (verbose_debug) {\n";
            os << "\t\tprintk(\"%s:%d, entered!\\n\", __func__, __LINE__);\n" << "\t}\n\n";

            for (const auto& [name, type] : roots.arguments) {
                unmarshaling_walk<marshal_side::callee> arg_unmarshal {os, name, 1};
                arg_unmarshal.visit_value(*type);
            }

            os << "\t";
            if (rpc.ret_pgraph)
                os << "ret = ";

            const auto impl_name = (rpc.kind == rpc_def_kind::direct) ? rpc.name : "function_ptr";
            os << impl_name << "(" << rpc.params_string << ");\n\n";
            os << "\tmsg->position = 0;\n";

            for (const auto& [name, type] : roots.arguments) {
                marshaling_walk<marshal_side::callee> arg_remarshal {os, name, 1};
                arg_remarshal.visit_value(*type);
            }

            if (rpc.ret_pgraph) {
                marshaling_walk<marshal_side::callee> ret_marshal {os, roots.return_value, 1};
                ret_marshal.visit_value(*rpc.ret_pgraph);
            }

            os << "\tmsg->slots[0] = msg->position;\n";

            // Add verbose printk's while returning
            os << "\tif (verbose_debug) {\n";
            os << "\t\tprintk(\"%s:%d, returned!\\n\", __func__, __LINE__);\n" << "\t}\n";
        }

        void generate_indirect_rpc_callee(std::ostream& os, rpc_def& rpc)
        {
            os << "void " << rpc.callee_id << "(struct fipc_message* msg, struct ext_registers* ext)\n{\n";
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
            os << "int try_dispatch(enum RPC_ID id, struct fipc_message* msg, struct ext_registers* ext)\n";
            os << "{\n";
            os << "\tswitch(id) {\n";
            if constexpr (side == rpc_side::client) {
                os << "\tcase MODULE_INIT:\n";
                os << "\t\tglue_user_trace(\"MODULE_INIT\");\n";
                os << "\t\t__module_lcd_init();\n";
                os << "\t\tbreak;\n\n";

                 os << "\tcase MODULE_EXIT:\n";
                os << "\t\tglue_user_trace(\"MODULE_EXIT\");\n";
                os << "\t\t__module_lcd_exit();\n";
                os << "\t\tbreak;\n\n";
            }

            for (const auto& rpc : rpcs) {
                if (rpc->kind == rpc_def_kind::direct && side == rpc_side::client)
                    continue;
                
                if (rpc->kind == rpc_def_kind::indirect && side == rpc_side::server)
                    continue;
                
                os << "\tcase " << rpc->enum_id << ":\n";
                os << "\t\tglue_user_trace(\"" << rpc->name << "\\n\");\n";
                os << "\t\t" << rpc->callee_id << "(msg, ext);\n";
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
                    file << "void " << rpc->callee_id << "(struct fipc_message*, struct ext_registers*)\n{\n";
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
                if (rpc->ret_type)
                    rpc->ret_string = rpc->ret_pgraph->c_specifier;
                else
                    rpc->ret_string = "void";

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
            file << "void " << node.caller_marshal_visitor
                << "(\n\tstruct fipc_message* msg,\n\tstruct ext_registers* ext,\n\tstruct " << node.real_name
                << " const* ptr)\n{\n";

            const auto roots = generate_root_ptrs<marshal_role::marshaling, marshal_side::caller>(file, node, "ptr");
            const auto n_fields = node.fields.size();
            for (const auto& [name, type] : roots) {
                marshaling_walk<marshal_side::caller> walk {file, name, 1};
                walk.visit_value(*type);
            }

            file << "}\n\n";
        }

        void generate_callee_unmarshal_visitor(std::ostream& file, projection& node)
        {
            file << "void " << node.callee_unmarshal_visitor
                << "(\n\tconst struct fipc_message* msg,\n\tconst struct ext_registers* ext,\n\tstruct "
                << node.real_name << "* ptr)\n{\n";

            const auto roots = generate_root_ptrs<marshal_role::unmarshaling, marshal_side::callee>(file, node, "ptr");
            const auto n_fields = node.fields.size();
            for (const auto& [name, type] : roots) {
                unmarshaling_walk<marshal_side::callee> walk {file, name, 1};
                walk.visit_value(*type);
            }

            file << "}\n\n";
        }

        void generate_callee_marshal_visitor(std::ostream& file, projection& node)
        {
            file << "void " << node.callee_marshal_visitor
                    << "(\n\tstruct fipc_message* msg,\n\tstruct ext_registers* ext,\n\tstruct "
                    << node.real_name << " const* ptr)\n{\n";

            const auto roots = generate_root_ptrs<marshal_role::marshaling, marshal_side::callee>(file, node, "ptr");
            const auto n_fields = node.fields.size();
            for (const auto& [name, type] : roots) {
                marshaling_walk<marshal_side::callee> walk {file, name, 1};
                walk.visit_value(*type);
            }

            file << "}\n\n";
        }

        void generate_caller_unmarshal_visitor(std::ostream& file, projection& node)
        {
            file << "void " << node.caller_unmarshal_visitor
                << "(\n\tconst struct fipc_message* msg,\n\tconst struct ext_registers* ext,\n\tstruct "
                << node.real_name << "* ptr)\n{\n";

            const auto roots = generate_root_ptrs<marshal_role::unmarshaling, marshal_side::caller>(file, node, "ptr");
            const auto n_fields = node.fields.size();
            for (const auto& [name, type] : roots) {
                unmarshaling_walk<marshal_side::caller> walk {file, name, 1};
                walk.visit_value(*type);
            }

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
                if (rpc->ret_pgraph)
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
