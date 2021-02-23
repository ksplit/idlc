#include <algorithm>
#include <fstream>
#include <tuple>
#include <vector>

#include <gsl/gsl>

#include "ast/ast.h"
#include "ast/pgraph.h"
#include "ast/pgraph_walk.h"
#include "frontend/analysis.h"
#include "utility.h"

// TODO: most of the marshal walks are identical, with differences in flag values
// TODO: similar situation with unmarshaling walks
// FIXME: do not generate pointer if-blocks if the value will be skipped (warning spam)
// FIXME: the writable-pointer cast hack is needed on both sides for unmarshaling, not just callee-side
// FIXME: don't ignore pointer annotations
// TODO: implement alloc support, which requires the concept of a value's "size expression"
// TODO: in general, lots of shared code and refactoring opportunities between the context-specific marshaling stuff
// (6!!)
// TODO: c_specifier_walk and company all belong in here, the generation module
// TODO: possibly split this module, it's by far the biggest one in the compiler

namespace idlc {
    namespace {
        using projection_vec = std::vector<gsl::not_null<projection*>>;
        using projection_vec_view = gsl::span<const gsl::not_null<projection*>>;

        class visitor_proto_walk : public pgraph_walk<visitor_proto_walk> {
        public:
            visitor_proto_walk(std::ostream& os) : m_stream {os} {}

            bool visit_projection(projection& node)
            {
                m_stream << "void " << node.caller_marshal_visitor
                    << "(\n\tstruct fipc_message*,\n\tstruct " << node.real_name << " const*);\n\n";

                m_stream << "void " << node.callee_unmarshal_visitor
                    << "(\n\tstruct fipc_message*,\n\tstruct " << node.real_name << "*);\n\n";

                m_stream << "void " << node.callee_marshal_visitor
                    << "(\n\tstruct fipc_message*,\n\tstruct " << node.real_name << " const*);\n\n";

                m_stream << "void " << node.caller_unmarshal_visitor
                    << "(\n\tstruct fipc_message*,\n\tstruct " << node.real_name << "*);\n\n";

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
            file << "#include <libfipc.h>\n";
            file << "#include <asm/lcd_domains/libvmfunc.h>\n";
            file << "\n";
            file << "#include \"glue_user.h\"\n";
            file << "\n";
            file << "#define glue_marshal(msg, value) (void)(value) // TODO\n";
            file << "#define glue_marshal_shadow(msg, value) (void)(value) // TODO\n";
            file << "#define glue_unmarshal(msg, type) (type)0xdeadbeef // TODO\n";
            file << "#define glue_unmarshal_shadow(msg, type) (type)0xdeadbeef // TODO\n";
            file << "#define glue_unmarshal_new_shadow(msg, type, size) (type)0xdeadbeef // TODO\n";
            file << "#define glue_unmarshal_rpc_ptr(msg, trmp_id) 0 // TODO\n";
            file << "#define glue_look_ahead(msg) 0 // TODO\n";
            file << "\n";
            file << "enum RPC_ID {\n";
            for (const auto& rpc : rpcs) {
                file << "\t" << rpc->enum_id << ",\n";
            }

            file << "};\n\n";

            generate_visitor_prototypes(file, projections);
            for (const auto& rpc : rpcs) {
                if (rpc->kind == rpc_def_kind::indirect) {
                    file << "typedef " << rpc->ret_string << " (*" << rpc->typedef_id << ")("
                        << rpc->args_string << ");\n";

                    file << "typedef " << rpc->ret_string << " (*" << rpc->impl_typedef_id << ")(" << rpc->typedef_id
                        << " target, " << rpc->args_string << ");\n\n";
                }
            }

            file << "\n#endif\n";
        }

        template<typename derived>
        class generation_walk : public pgraph_walk<derived> {
        public:
            generation_walk(std::ostream& os, absl::string_view holder, unsigned level) :
                m_stream {os},
                m_marshaled_ptr {holder},
                m_indent_level {level}
            {}

        protected:
            // identifier of the variable that points to what we're currently marshaling
            const auto& subject()
            {
                return m_marshaled_ptr;
            }

            template<typename node_type>
            bool marshal(std::string&& new_ptr, node_type& type)
            {
                const auto state = std::make_tuple(m_marshaled_ptr, m_indent_level);
                m_marshaled_ptr = new_ptr;
                ++m_indent_level;
                if (!this->traverse(this->self(), type))
                    return false;

                std::forward_as_tuple(m_marshaled_ptr, m_indent_level) = state;

                return true;
            }

            std::ostream& stream()
            {
                return m_stream;
            }

            std::ostream& new_line()
            {
                return indent(m_stream, m_indent_level);
            }

        private:
            std::ostream& m_stream;
            std::string m_marshaled_ptr {};
            unsigned m_indent_level {};
        };

        enum class rpc_side {
            caller,
            callee
        };

        template<rpc_side side>
        class marshaling_walk : public generation_walk<marshaling_walk<side>> {
        public:
            marshaling_walk(std::ostream& os, absl::string_view holder, unsigned level) :
                generation_walk<marshaling_walk<side>> {os, holder, level}
            {}

            bool visit_pointer(pointer& node)
            {
                if (should_bind(node.pointer_annots))
                    this->new_line() << "glue_marshal_shadow(msg, *" << this->subject() << ");\n";
                else
                    this->new_line() << "glue_marshal(msg, *" << this->subject() << ");\n";

                if (should_walk(node.pointer_annots)) {
                    this->new_line() << "if (*" << this->subject() << ") {\n";
                    this->marshal("*" + this->subject(), node);
                    this->new_line() << "}\n\n";
                }

                return true;
            }

            bool visit_primitive(primitive node)
            {
                this->new_line() << "glue_marshal(msg, *" << this->subject() << ");\n";
                return true;
            }

            bool visit_rpc_ptr(rpc_ptr& node)
            {
                this->new_line() << "glue_marshal(msg, *" << this->subject() << ");\n";
                return true;
            }

            bool visit_projection(projection& node)
            {
                this->new_line() << visitor(node) << "(msg, " << this->subject() << ");\n";
                return true;
            }

            bool visit_null_terminated_array(null_terminated_array& node)
            {
                const auto star = node.element->is_const ? " const*" : "*";
                const auto ptr_type = concat(node.element->c_specifier, star);

                this->new_line() << "size_t i, len;\n";
                this->new_line() << ptr_type << " array = "	<< this->subject() << ";\n";
                this->new_line() << "for (len = 0; array[len]; ++len);\n";
                this->new_line() << "glue_marshal(msg, len);\n";
                this->new_line() << "for (i = 0; i < len; ++i) {\n";
                this->new_line() << "\t" << ptr_type << " element = &array[i];\n";
                if (!this->marshal("element", node))
                    return false;

                this->new_line() << "}\n\n";

                return true;
            }

            bool visit_value(value& node)
            {
                if (should_walk(node.value_annots)) {
                    return this->traverse(*this, node);
                }

                return true;
            }

        private:
            static constexpr bool should_walk(annotation flags)
            {
                switch (side) {
                case rpc_side::caller:
                    return flags_set(flags, annotation::in);

                case rpc_side::callee:
                    return flags_set(flags, annotation::out);
                }

                std::terminate();
            }

            /*
                This confused me today; it will confuse you too
                Bind annotations indicate which side of the call has the shadow copy (statically)
                Whether or not they are bound on a side is irrespective of whether we are marshaling or unmarshaling
            */
            static constexpr bool should_bind(annotation flags)
            {
                switch (side) {
                case rpc_side::caller:
                    return flags_set(flags, annotation::bind_caller);

                case rpc_side::callee:
                    return flags_set(flags, annotation::bind_callee);
                }

                std::terminate();
            }

            static constexpr absl::string_view visitor(projection& node)
            {
                switch (side) {
                case rpc_side::caller:
                    return node.caller_marshal_visitor;

                case rpc_side::callee:
                    return node.callee_marshal_visitor;
                }

                std::terminate();
            }
        };

        auto get_size_expr(const value& node)
        {
            const auto visit = [&node](auto&& item) {
                using node_type = std::decay_t<decltype(item)>;
                if constexpr (std::is_same_v<node_type, null_terminated_array> || std::is_same_v<node_type, dyn_array>)
                    return concat("sizeof(", node.c_specifier, " * glue_look_ahead(msg))");
                else
                    return concat("sizeof(", node.c_specifier, ")");
            };

            return std::visit(visit, node.type);
        }

        template<rpc_side side>
        class unmarshaling_walk : public generation_walk<unmarshaling_walk<side>> {
        public:
            unmarshaling_walk(std::ostream& os, absl::string_view holder, unsigned level) :
                generation_walk<unmarshaling_walk<side>> {os, holder, level}
            {}

            bool visit_value(value& node)
            {
                if (should_walk(node.value_annots)) {
                    const auto old = m_c_specifier;
                    m_c_specifier = node.c_specifier;
                    if (!this->traverse(*this, node))
                        return false;

                    m_c_specifier = old;
                }

                return true;
            }

            bool visit_primitive(primitive node)
            {
                this->new_line() << "*" << this->subject() << " = glue_unmarshal(msg, " << m_c_specifier << ");\n";
                return true;
            }

            bool visit_pointer(pointer& node)
            {
                this->new_line() << "*" << this->subject() << " = ";
                if (should_bind(node.pointer_annots))
                    this->stream() << "glue_unmarshal_shadow(msg, " << m_c_specifier << ")";
                else if (should_alloc(node.pointer_annots)) {
                    this->stream() << "glue_unmarshal_new_shadow(msg, " << m_c_specifier << ", "
                        << get_size_expr(*node.referent) << ")";
                }
                else {
                    this->stream() << "glue_unmarshal(msg, " << m_c_specifier << ")";
                }

                this->stream() << ";\n";

                if (!should_walk(node.referent->value_annots))
                    return true;

                this->new_line() << "if (*" << this->subject() << ") {\n";
                if (node.referent->is_const) {
                    const auto type = concat(node.referent->c_specifier, "*");
                    this->new_line() << "\t" << type << " writable = "
                        << concat("(", type, ")*", this->subject()) << ";\n";

                    if (!this->marshal("writable", node))
                        return false;
                }
                else {
                    if (!this->marshal(concat("*", this->subject()), node))
                        return false;
                }

                this->new_line() << "}\n\n";

                return true;
            }

            bool visit_null_terminated_array(null_terminated_array& node)
            {
                this->new_line() << "size_t i, len;\n";
                this->new_line() << node.element->c_specifier << "* array = " << this->subject() << ";\n";
                this->new_line() << "len = glue_unmarshal(msg, size_t);\n";
                this->new_line() << "for (i = 0; i < len; ++i) {\n";
                this->new_line() << "\t" << node.element->c_specifier << "* element = &array[i];\n";
                if (!this->marshal("element", node))
                    return false;

                this->new_line() << "}\n\n";

                return true;
            }

            bool visit_dyn_array(dyn_array& node)
            {
                this->new_line() << "int i;\n";
                this->new_line() << node.element->c_specifier << "* array = " << this->subject() << ";\n";
                this->new_line() << "for (i = 0; i < " << node.size << "; ++i) {\n";
                this->new_line() << "\t" << node.element->c_specifier << "* element = &array[i];\n";
                if (!this->marshal("element", node))
                    return false;

                this->new_line() << "}\n\n";

                return true;
            }

            bool visit_rpc_ptr(rpc_ptr& node)
            {
                this->new_line() << "*" << this->subject() << " = glue_unmarshal_rpc_ptr(msg, "
                    << node.definition->trmp_id << ");\n";

                return true;
            }

            bool visit_projection(projection& node)
            {
                this->new_line() << visitor(node) << "(msg, " << this->subject() << ");\n";
                return true;
            }

        private:
            absl::string_view m_c_specifier {};

            static constexpr bool should_walk(annotation flags)
            {
                switch (side) {
                case rpc_side::callee:
                    return flags_set(flags, annotation::in);

                case rpc_side::caller:
                    return flags_set(flags, annotation::out);
                }

                std::terminate();
            }

            static constexpr bool should_bind(annotation flags)
            {
                switch (side) {
                case rpc_side::caller:
                    return flags_set(flags, annotation::bind_caller);

                case rpc_side::callee:
                    return flags_set(flags, annotation::bind_callee);
                }

                std::terminate();
            }

            static constexpr bool should_alloc(annotation flags)
            {
                switch (side) {
                case rpc_side::caller:
                    return flags_set(flags, annotation::alloc_caller);

                case rpc_side::callee:
                    return flags_set(flags, annotation::alloc_callee);
                }

                std::terminate();
            }

            static constexpr absl::string_view visitor(projection& node)
            {
                switch (side) {
                case rpc_side::callee:
                    return node.callee_unmarshal_visitor;

                case rpc_side::caller:
                    return node.caller_unmarshal_visitor;
                }

                std::terminate();
            }
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
        auto generate_root_ptrs(rpc_def& rpc, std::ostream& os)
        {
            const auto n_args = gsl::narrow<gsl::index>(rpc.arg_pgraphs.size());
            std::vector<std::tuple<std::string, value*>> roots;
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

            return std::make_tuple(roots, "ret_ptr");
        }

        // TODO: this is confusing, document it
        auto generate_root_ptrs(
            std::ostream& os,
            projection& projection,
            absl::string_view source_var,
            annotation context,
            bool is_const_context)
        {
            std::vector<std::tuple<std::string, value*>> roots;
            roots.reserve(projection.fields.size());
            for (const auto& [name, type] : projection.fields) {
                if (!flags_set(type->value_annots, context))
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

        void generate_caller_glue(rpc_def& rpc, std::ostream& os)
        {
            os << "\tstruct fipc_message msg_buf = {0};\n";
            os << "\tstruct fipc_message *msg = &msg_buf;\n\n";

            const auto n_args = rpc.arg_pgraphs.size();
            const auto [roots, ret_root] = generate_root_ptrs(rpc, os);

            if (rpc.kind == rpc_def_kind::indirect)
                os << "\tglue_marshal(msg, target);\n";

            for (const auto& [name, type] : roots) {
                marshaling_walk<rpc_side::caller> arg_marshal {os, name, 1};
                arg_marshal.visit_value(*type);
            }

            os << "\tvmfunc_wrapper(msg);\n\n";

            for (const auto& [name, type] : roots) {
                unmarshaling_walk<rpc_side::caller> arg_unremarshal {os, name, 1};
                arg_unremarshal.visit_value(*type);
            }

            if (rpc.ret_pgraph) {
                unmarshaling_walk<rpc_side::caller> ret_unmarshal {os, ret_root, 1};
                ret_unmarshal.visit_value(*rpc.ret_pgraph);
            }

            os << (rpc.ret_pgraph ? concat("\treturn ret;\n") : "");
        }

        void generate_callee_glue(rpc_def& rpc, std::ostream& os)
        {
            if (rpc.kind == rpc_def_kind::indirect)
                os << "\t" << rpc.typedef_id << " function_ptr = glue_unmarshal(msg, " << rpc.typedef_id << ");\n";

            const auto n_args = gsl::narrow<gsl::index>(rpc.arg_pgraphs.size());
            for (gsl::index i {}; i < n_args; ++i) {
                const auto& type = rpc.arg_pgraphs.at(i)->c_specifier;
                const auto name = rpc.arguments->at(i)->name;
                os << "\t" << type << " " << name << " = 0;\n";
            }

            const auto [roots, ret_root] = generate_root_ptrs(rpc, os);

            for (const auto& [name, type] : roots) {
                unmarshaling_walk<rpc_side::callee> arg_unmarshal {os, name, 1};
                arg_unmarshal.visit_value(*type);
            }

            os << "\t";
            if (rpc.ret_pgraph)
                os << "ret = ";

            const auto impl_name = (rpc.kind == rpc_def_kind::direct) ? rpc.name : "function_ptr";
            os << impl_name << "(" << rpc.params_string << ");\n\n";

            for (const auto& [name, type] : roots) {
                marshaling_walk<rpc_side::callee> arg_remarshal {os, name, 1};
                arg_remarshal.visit_value(*type);
            }

            if (rpc.ret_pgraph) {
                marshaling_walk<rpc_side::callee> ret_marshal {os, ret_root, 1};
                ret_marshal.visit_value(*rpc.ret_pgraph);
            }
        }

        void generate_indirect_rpc(rpc_def& rpc, std::ostream& os)
        {
            os << rpc.ret_string << " " << rpc.impl_id << "(" << rpc.typedef_id << " target, "
                << rpc.args_string << ")\n{\n";

            generate_caller_glue(rpc, os);
            os << "}\n\n";

            os << "void " << rpc.callee_id << "(struct fipc_message* msg)\n{\n";
            generate_callee_glue(rpc, os);
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

        void generate_caller(rpc_vec_view rpcs)
        {
            std::ofstream file {"caller.c"};
            file.exceptions(file.badbit | file.failbit);

            file << "#include <lcd_config/pre_hook.h>\n\n";
            file << "#include \"common.h\"\n\n";
            file << "#include <lcd_config/post_hook.h>\n\n";
            for (const auto& rpc : rpcs) {
                if (rpc->kind == rpc_def_kind::direct) {
                    file << rpc->ret_string << " " << rpc->name << "(" << rpc->args_string << ")\n{\n";
                    generate_caller_glue(*rpc, file);
                    file << "}\n\n";
                }
                else {
                    generate_indirect_rpc(*rpc, file);
                }
            }
        }

        void generate_callee(rpc_vec_view rpcs)
        {
            std::ofstream file {"callee.c"};
            file.exceptions(file.badbit | file.failbit);

            file << "#include <lcd_config/pre_hook.h>\n\n";
            file << "#include \"common.h\"\n\n";
            file << "#include <lcd_config/post_hook.h>\n\n";
            for (const auto& rpc : rpcs) {
                if (rpc->kind == rpc_def_kind::indirect) {
                    generate_indirect_rpc(*rpc, file);
                }
                else {
                    file << "void " << rpc->callee_id << "(struct fipc_message* msg)\n{\n";
                    generate_callee_glue(*rpc, file);
                    file << "}\n\n";
                }
            }
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

            file << "}\n";
            file << "INSERT AFTER .text\n";
        }

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
            file << "void " << node.caller_marshal_visitor << "(\n\tstruct fipc_message* msg,\n\tstruct "
                << node.real_name << " const* ptr)\n{\n";

            const auto roots = generate_root_ptrs(file, node, "ptr", annotation::in, true);
            const auto n_fields = node.fields.size();
            for (const auto& [name, type] : roots) {
                marshaling_walk<rpc_side::caller> walk {file, name, 1};
                walk.visit_value(*type);
            }

            file << "}\n\n";
        }

        void generate_callee_unmarshal_visitor(std::ostream& file, projection& node)
        {
            file << "void " << node.callee_unmarshal_visitor << "(\n\tstruct fipc_message* msg,\n\tstruct "
                << node.real_name << "* ptr)\n{\n";

            const auto roots = generate_root_ptrs(file, node, "ptr", annotation::in, false);
            const auto n_fields = node.fields.size();
            for (const auto& [name, type] : roots) {
                unmarshaling_walk<rpc_side::callee> walk {file, name, 1};
                walk.visit_value(*type);
            }

            file << "}\n\n";
        }

        void generate_callee_marshal_visitor(std::ostream& file, projection& node)
        {
            file << "void " << node.callee_marshal_visitor
                    << "(\n\tstruct fipc_message* msg,\n\tstruct " << node.real_name << " const* ptr)\n{\n";

            const auto roots = generate_root_ptrs(file, node, "ptr", annotation::out, true);
            const auto n_fields = node.fields.size();
            for (const auto& [name, type] : roots) {
                marshaling_walk<rpc_side::callee> walk {file, name, 1};
                walk.visit_value(*type);
            }

            file << "}\n\n";
        }

        void generate_caller_unmarshal_visitor(std::ostream& file, projection& node)
        {
            file << "void " << node.caller_unmarshal_visitor
                    << "(\n\tstruct fipc_message* msg,\n\tstruct " << node.real_name << "* ptr)\n{\n";

            const auto roots = generate_root_ptrs(file, node, "ptr", annotation::out, false);
            const auto n_fields = node.fields.size();
            for (const auto& [name, type] : roots) {
                unmarshaling_walk<rpc_side::caller> walk {file, name, 1};
                walk.visit_value(*type);
            }

            file << "}\n\n";
        }

        void generate_common_source(projection_vec_view projections)
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

        void populate_c_type_specifiers(value& node);

        class c_specifier_walk : public pgraph_walk<c_specifier_walk> {
        public:
            const auto& get() const
            {
                return m_specifier;
            }

            bool visit_value(value& node)
            {
                if (!traverse(*this, node))
                    return false;

                node.c_specifier = m_specifier;
                return true;
            }

            bool visit_projection(projection& node)
            {
                m_specifier = "struct ";
                m_specifier += node.real_name;
                for (const auto& [name, field] : node.fields)
                    populate_c_type_specifiers(*field);

                return true;
            }

            bool visit_pointer(pointer& node)
            {
                if (!traverse(*this, node))
                    return false;

                if (node.referent->is_const)
                    m_specifier += " const";

                m_specifier += "*";

                return true;
            }

            bool visit_rpc_ptr(rpc_ptr& node)
            {
                m_specifier += node.definition->typedef_id;
                return true;
            }

            bool visit_primitive(primitive node)
            {
                switch (node) {
                case primitive::ty_bool:
                    m_specifier = "bool";
                    break;

                case primitive::ty_char:
                    m_specifier = "char";
                    break;

                case primitive::ty_schar:
                    m_specifier = "signed char";
                    break;

                case primitive::ty_uchar:
                    m_specifier = "unsigned char";
                    break;

                case primitive::ty_short:
                    m_specifier = "short";
                    break;

                case primitive::ty_ushort:
                    m_specifier = "unsigned short";
                    break;

                case primitive::ty_int:
                    m_specifier = "int";
                    break;

                case primitive::ty_uint:
                    m_specifier = "unsigned int";
                    break;

                case primitive::ty_long:
                    m_specifier = "long";
                    break;

                case primitive::ty_ulong:
                    m_specifier = "unsigned long";
                    break;

                case primitive::ty_llong:
                    m_specifier = "long long";
                    break;

                case primitive::ty_ullong:
                    m_specifier = "unsigned long long";
                    break;

                default:
                    std::cout << "Debug: Unhandled primitive was " << static_cast<std::uintptr_t>(node) << "\n";
                    std::terminate();
                }

                return true;
            }

        private:
            std::string m_specifier {};
        };

        // NOTE: this *does not* walk into projections, and should only be applied to
        // variable types (fields or arguments) and return types, as done in the lowering pass
        void populate_c_type_specifiers(value& node)
        {
            c_specifier_walk type_walk {};
            const auto succeeded = type_walk.visit_value(node);
            assert(succeeded);
        }

        void populate_c_type_specifiers(rpc_vec_view rpcs)
        {
            for (const auto& node : rpcs) {
                if (node->ret_pgraph)
                    populate_c_type_specifiers(*node->ret_pgraph);
                
                for (const auto& arg : node->arg_pgraphs)
                    populate_c_type_specifiers(*arg);
            }
        }
    }

    void generate(rpc_vec_view rpcs)
    {
        const auto projections = idlc::get_projections(rpcs);
        populate_c_type_specifiers(rpcs);
        create_function_strings(rpcs);
        generate_common_header(rpcs, projections);
        generate_common_source(projections);
        generate_caller(rpcs);
        generate_callee(rpcs);
        generate_linker_script(rpcs);
    }
}