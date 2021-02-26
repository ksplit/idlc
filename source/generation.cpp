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

// TODO: possibly split this module, it's by far the biggest one in the compiler
// TODO: there is a certain illegal range of ints that cannot be reversibly casted
// TODO: bug where we try and overwrite an existing, const value, but this should never happen in sane code?
// FIXME: annotations are only valid at the leaves!

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
                    << "(\n\tstruct glue_message*,\n\tstruct " << node.real_name << " const*);\n\n";

                m_stream << "void " << node.callee_unmarshal_visitor
                    << "(\n\tstruct glue_message*,\n\tstruct " << node.real_name << "*);\n\n";

                m_stream << "void " << node.callee_marshal_visitor
                    << "(\n\tstruct glue_message*,\n\tstruct " << node.real_name << " const*);\n\n";

                m_stream << "void " << node.caller_unmarshal_visitor
                    << "(\n\tstruct glue_message*,\n\tstruct " << node.real_name << "*);\n\n";

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
            file << "\n";
            file << "#include \"glue_user.h\"\n";
            file << "\n";
            file << "#define GLUE_MAX_SLOTS 128\n";
            file << "#define glue_pack(msg, value) glue_pack_impl((msg), (uint64_t)(value))\n";
            file << "#define glue_pack_shadow(msg, value) glue_pack_shadow_impl((msg), (value))\n";
            file << "#define glue_unpack(msg, type) (type)glue_unpack_impl((msg))\n";
            file << "#define glue_unpack_shadow(msg, type) (type)glue_unpack_shadow_impl(glue_unpack(msg, void*));\n";
            file << "#define glue_unpack_new_shadow(msg, type, size) \\\n"
                << "\t(type)glue_unpack_new_shadow_impl(glue_unpack(msg, void*), size)\n\n";

            file << "#define glue_unpack_rpc_ptr(msg, trmp_id) 0 // TODO\n";
            file << "#define glue_peek(msg) glue_peek_impl(msg)\n";
            file << "#define glue_call_server(msg, rpc_id) \\\n"
                << "\tmsg->slots[0] = msg->position; msg->position = 0; glue_user_call_server(msg->slots, rpc_id);\n\n";

            file << "#define glue_call_client(msg, rpc_id) \\\n"
                << "\tmsg->slots[0] = msg->position; msg->position = 0; glue_user_call_client(msg->slots, rpc_id);\n\n";

            file << "\n";
            file << "void glue_user_panic(const char* msg);\n";
            file << "void glue_user_trace(const char* msg);\n";
            file << "void* glue_user_map_to_shadow(const void* obj);\n";
            file << "const void* glue_user_map_from_shadow(const void* shadow);\n";
            file << "void glue_user_add_shadow(const void* ptr, void* shadow);\n";
            file << "void* glue_user_alloc(size_t size);\n";
            file << "void glue_user_free(void* ptr);\n";
            file << "void glue_user_call_server(const uint64_t* data, size_t rpc_id);\n";
            file << "void glue_user_call_client(const uint64_t* data, size_t rpc_id);\n";
            file << "\n";
            file << "struct glue_message {\n";
            file << "\tuint64_t slots[GLUE_MAX_SLOTS];\n";
            file << "\tuint64_t position;\n";
            file << "};\n";
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
            file << "\n";
            file << "enum RPC_ID {\n";
            for (const auto& rpc : rpcs)
                file << "\t" << rpc->enum_id << ",\n";

            file << "};\n\n";

            file << "int try_dispatch(enum RPC_ID id, struct glue_message* msg);\n\n";

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

        enum class marshal_side {
            caller,
            callee
        };

        // TODO: move me to the pgraph headers
        bool is_nonterminal(const value& node)
        {
            const auto visit = [](auto&& item) {
                using type = std::decay_t<decltype(item)>;
                return std::is_same_v<type, node_ptr<projection>>
                    || std::is_same_v<type, node_ptr<pointer>>;
            };

            return std::visit(visit, node.type);
        }

        template<marshal_side side>
        class marshaling_walk : public generation_walk<marshaling_walk<side>> {
        public:
            marshaling_walk(std::ostream& os, absl::string_view holder, unsigned level) :
                generation_walk<marshaling_walk<side>> {os, holder, level}
            {}

            bool visit_pointer(pointer& node)
            {

                if (should_bind(node.pointer_annots))
                    this->new_line() << "glue_pack_shadow(msg, *" << this->subject() << ");\n";
                else
                    this->new_line() << "glue_pack(msg, *" << this->subject() << ");\n";
                
                if (!should_walk(*node.referent))
                    return true;

                this->new_line() << "if (*" << this->subject() << ") {\n";
                this->marshal("*" + this->subject(), node);
                this->new_line() << "}\n\n";

                return true;
            }

            bool visit_primitive(primitive node)
            {
                this->new_line() << "glue_pack(msg, *" << this->subject() << ");\n";
                return true;
            }

            bool visit_rpc_ptr(rpc_ptr& node)
            {
                this->new_line() << "glue_pack(msg, *" << this->subject() << ");\n";
                return true;
            }

            bool visit_projection(projection& node)
            {
                this->new_line() << get_visitor_name(node) << "(msg, " << this->subject() << ");\n";
                return true;
            }

            bool visit_null_terminated_array(null_terminated_array& node)
            {
                const auto star = node.element->is_const ? " const*" : "*";
                const auto ptr_type = concat(node.element->c_specifier, star);

                this->new_line() << "size_t i, len;\n";
                this->new_line() << ptr_type << " array = "	<< this->subject() << ";\n";
                this->new_line() << "for (len = 0; array[len]; ++len);\n";
                this->new_line() << "glue_pack(msg, len);\n";
                this->new_line() << "for (i = 0; i < len; ++i) {\n";
                this->new_line() << "\t" << ptr_type << " element = &array[i];\n";
                if (!this->marshal("element", node))
                    return false;

                this->new_line() << "}\n\n";

                return true;
            }

            bool visit_value(value& node)
            {
                if (should_walk(node)) {
                    return this->traverse(*this, node);
                }

                return true;
            }

        private:
            static constexpr bool should_walk(const value& node)
            {
                if (is_nonterminal(node))
                    return true;

                const auto flags = node.value_annots;
                switch (side) {
                case marshal_side::caller:
                    return flags_set(flags, annotation::in);

                case marshal_side::callee:
                    return flags_set(flags, annotation::out);
                }

                std::terminate();
            }

            /*
                NOTE: This confused me today; it will confuse you too
                Bind annotations indicate which side of the call has the shadow copy (statically)
                Whether or not they are bound on a side is irrespective of whether we are marshaling or unmarshaling
            */
            static constexpr bool should_bind(annotation flags)
            {
                switch (side) {
                case marshal_side::caller:
                    return flags_set(flags, annotation::bind_caller);

                case marshal_side::callee:
                    return flags_set(flags, annotation::bind_callee);
                }

                std::terminate();
            }

            static constexpr absl::string_view get_visitor_name(projection& node)
            {
                switch (side) {
                case marshal_side::caller:
                    return node.caller_marshal_visitor;

                case marshal_side::callee:
                    return node.callee_marshal_visitor;
                }

                std::terminate();
            }
        };

        auto get_size_expr(const value& node)
        {
            const auto visit = [&node](auto&& item) {
                using node_type = std::decay_t<decltype(item)>;
                if constexpr (std::is_same_v<node_type, node_ptr<null_terminated_array>> || std::is_same_v<node_type, node_ptr<dyn_array>>)
                    return concat("sizeof(", node.c_specifier, ") * glue_peek(msg)");
                else
                    return concat("sizeof(", node.c_specifier, ")");
            };

            return std::visit(visit, node.type);
        }

        template<marshal_side side>
        class unmarshaling_walk : public generation_walk<unmarshaling_walk<side>> {
        public:
            unmarshaling_walk(std::ostream& os, absl::string_view holder, unsigned level) :
                generation_walk<unmarshaling_walk<side>> {os, holder, level}
            {}

            bool visit_value(value& node)
            {
                if (should_walk(node)) {
                    auto old = std::move(m_c_specifier);
                    m_c_specifier = node.c_specifier;
                    if (!this->traverse(*this, node))
                        return false;

                    m_c_specifier = std::move(old);
                }

                return true;
            }

            bool visit_primitive(primitive node)
            {
                this->new_line() << "*" << this->subject() << " = glue_unpack(msg, " << m_c_specifier << ");\n";
                return true;
            }

            // TODO: long an complex, could use some splitting
            // TODO: need to automatically free shadows as they're replaced?
            bool visit_pointer(pointer& node)
            {
                this->new_line() << "*" << this->subject() << " = ";
                if (should_bind(node.pointer_annots)) {
                    this->stream() << "glue_unpack_shadow(msg, " << m_c_specifier << ")";
                }
                else if (should_alloc(node.pointer_annots)) {
                    this->stream() << "glue_unpack_new_shadow(msg, " << m_c_specifier << ", "
                        << get_size_expr(*node.referent) << ")";
                }
                else {
                    this->stream() << "glue_unpack(msg, " << m_c_specifier << ")";
                }

                this->stream() << ";\n";
                if (!should_walk(*node.referent))
                    return true;

                this->new_line() << "if (*" << this->subject() << ") {\n";
                if (node.referent->is_const) {
                    // Since the type itself must include const, but we need to write to it for unmarshaling,
                    // We create a writeable version of the pointer and use it instead
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

            // FIXME: consider the fact that these can change size
            bool visit_null_terminated_array(null_terminated_array& node)
            {
                this->new_line() << "size_t i, len;\n";
                this->new_line() << node.element->c_specifier << "* array = " << this->subject() << ";\n";
                this->new_line() << "len = glue_unpack(msg, size_t);\n";
                this->new_line() << "array[len] = '\\0';\n";
                this->new_line() << "for (i = 0; i < len; ++i) {\n";
                this->new_line() << "\t" << node.element->c_specifier << "* element = &array[i];\n";
                if (!this->marshal("element", node))
                    return false;

                this->new_line() << "}\n\n";

                return true;
            }

            // FIXME: we pack the array size before the array elements, but this doesn't try and use it
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
                this->new_line() << "*" << this->subject() << " = glue_unpack_rpc_ptr(msg, "
                    << node.definition->trmp_id << ");\n";

                return true;
            }

            bool visit_projection(projection& node)
            {
                this->new_line() << get_visitor_name(node) << "(msg, " << this->subject() << ");\n";
                return true;
            }

        private:
            absl::string_view m_c_specifier {};

            static constexpr bool should_walk(const value& node)
            {
                if (is_nonterminal(node))
                    return true;

                const auto flags = node.value_annots;
                switch (side) {
                case marshal_side::callee:
                    return flags_set(flags, annotation::in);

                case marshal_side::caller:
                    return flags_set(flags, annotation::out);
                }

                std::terminate();
            }

            static constexpr bool should_bind(annotation flags)
            {
                switch (side) {
                case marshal_side::caller:
                    return flags_set(flags, annotation::bind_caller);

                case marshal_side::callee:
                    return flags_set(flags, annotation::bind_callee);
                }

                std::terminate();
            }

            static constexpr bool should_alloc(annotation flags)
            {
                switch (side) {
                case marshal_side::caller:
                    return flags_set(flags, annotation::alloc_caller);

                case marshal_side::callee:
                    return flags_set(flags, annotation::alloc_callee);
                }

                std::terminate();
            }

            static constexpr absl::string_view get_visitor_name(projection& node)
            {
                switch (side) {
                case marshal_side::callee:
                    return node.callee_unmarshal_visitor;

                case marshal_side::caller:
                    return node.caller_unmarshal_visitor;
                }

                std::terminate();
            }
        };

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

        enum class rpc_side {
            server,
            client
        };

        auto generate_caller_glue_prologue(std::ostream& os, rpc_def& rpc)
        {
            os << "\tstruct glue_message *msg = glue_user_alloc(sizeof(struct glue_message));\n\n";

            const auto n_args = rpc.arg_pgraphs.size();
            const auto roots = generate_root_ptrs(rpc, os);

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
            for (const auto& [name, type] : roots.arguments) {
                unmarshaling_walk<marshal_side::caller> arg_unremarshal {os, name, 1};
                arg_unremarshal.visit_value(*type);
            }

            if (rpc.ret_pgraph) {
                unmarshaling_walk<marshal_side::caller> ret_unmarshal {os, roots.return_value, 1};
                ret_unmarshal.visit_value(*rpc.ret_pgraph);
            }

            os << "\tglue_user_free(msg);\n";
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
        }

        // Portions of indirect RPC glue that are identical in both client and server
        void generate_indirect_rpc_common_code(std::ostream& os, rpc_def& rpc)
        {
            os << "void " << rpc.callee_id << "(struct glue_message* msg)\n{\n";
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

        template<rpc_side side>
        void generate_indirect_rpc(rpc_def& rpc, std::ostream& os)
        {
            os << rpc.ret_string << " " << rpc.impl_id << "(" << rpc.typedef_id << " target, "
                << rpc.args_string << ")\n{\n";

            generate_caller_glue<side>(rpc, os);
            os << "}\n\n";

            generate_indirect_rpc_common_code(os, rpc);
        }

        template<rpc_side side>
        void generate_dispatch_fn(std::ostream& os, rpc_vec_view rpcs)
        {
            os << "int try_dispatch(enum RPC_ID id, struct glue_message* msg)\n";
            os << "{\n";
            os << "\tswitch(id) {\n";
            for (const auto& rpc : rpcs) {
                if (rpc->kind == rpc_def_kind::direct && side == rpc_side::client)
                    continue;
                
                os << "\tcase " << rpc->enum_id << ":\n";
                os << "\t\tglue_user_trace(\"" << rpc->name << "\\n\");\n";
                os << "\t\t" << rpc->callee_id << "(msg);\n";
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
                if (rpc->kind == rpc_def_kind::direct) {
                    file << rpc->ret_string << " " << rpc->name << "(" << rpc->args_string << ")\n{\n";
                    generate_caller_glue<rpc_side::client>(*rpc, file);
                    file << "}\n\n";
                }
                else {
                    generate_indirect_rpc<rpc_side::client>(*rpc, file);
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
                if (rpc->kind == rpc_def_kind::indirect) {
                    generate_indirect_rpc<rpc_side::server>(*rpc, file);
                }
                else {
                    file << "void " << rpc->callee_id << "(struct glue_message* msg)\n{\n";
                    generate_callee_glue(*rpc, file);
                    file << "}\n\n";
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
            file << "void " << node.caller_marshal_visitor << "(\n\tstruct glue_message* msg,\n\tstruct "
                << node.real_name << " const* ptr)\n{\n";

            const auto roots = generate_root_ptrs(file, node, "ptr", annotation::in, true);
            const auto n_fields = node.fields.size();
            for (const auto& [name, type] : roots) {
                marshaling_walk<marshal_side::caller> walk {file, name, 1};
                walk.visit_value(*type);
            }

            file << "}\n\n";
        }

        void generate_callee_unmarshal_visitor(std::ostream& file, projection& node)
        {
            file << "void " << node.callee_unmarshal_visitor << "(\n\tstruct glue_message* msg,\n\tstruct "
                << node.real_name << "* ptr)\n{\n";

            const auto roots = generate_root_ptrs(file, node, "ptr", annotation::in, false);
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
                    << "(\n\tstruct glue_message* msg,\n\tstruct " << node.real_name << " const* ptr)\n{\n";

            const auto roots = generate_root_ptrs(file, node, "ptr", annotation::out, true);
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
                    << "(\n\tstruct glue_message* msg,\n\tstruct " << node.real_name << "* ptr)\n{\n";

            const auto roots = generate_root_ptrs(file, node, "ptr", annotation::out, false);
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

        void populate_c_type_specifiers(rpc_vec_view rpcs, projection_vec_view projections)
        {
            for (const auto& node : rpcs) {
                if (node->ret_pgraph)
                    populate_c_type_specifiers(*node->ret_pgraph);
                
                for (const auto& arg : node->arg_pgraphs)
                    populate_c_type_specifiers(*arg);
            }

            for (const auto& projection : projections) {
                for (const auto& [name, field] : projection->fields)
                    populate_c_type_specifiers(*field);
            }
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