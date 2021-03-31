#ifndef IDLC_BACKEND_WALKS_H
#define IDLC_BACKEND_WALKS_H

namespace idlc {
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

    enum class marshal_role {
        marshaling,
        unmarshaling
    };

    template<marshal_side side>
    constexpr bool should_marshal(const value& node)
    {
        const auto flags = node.value_annots;
        switch (side) {
        case marshal_side::caller:
            return flags_set(flags, annotation_kind::in);

        case marshal_side::callee:
            return flags_set(flags, annotation_kind::out);
        }

        std::terminate();
    }

    template<marshal_side side>
    constexpr bool should_unmarshal(const value& node)
    {
        const auto flags = node.value_annots;
        switch (side) {
        case marshal_side::callee:
            return flags_set(flags, annotation_kind::in);

        case marshal_side::caller:
            return flags_set(flags, annotation_kind::out);
        }

        std::terminate();
    }

    template<marshal_role role, marshal_side side>
    constexpr bool should_walk(const value& node)
    {
        switch (role) {
        case marshal_role::marshaling:
            return is_nonterminal(node) || should_marshal<side>(node);

        case marshal_role::unmarshaling:
            return is_nonterminal(node) || should_unmarshal<side>(node);
        }

        std::terminate();
    }

    template<marshal_side side>
    class marshaling_walk : public generation_walk<marshaling_walk<side>> {
    public:
        marshaling_walk(std::ostream& os, absl::string_view holder, unsigned level) :
            generation_walk<marshaling_walk<side>> {os, holder, level}
        {}

        bool visit_pointer(pointer& node)
        {
            if (m_should_marshal) {
                if (should_bind(node.pointer_annots)) {
                    this->new_line() << "glue_pack_shadow(pos, msg, ext, *" << this->subject() << ");\n";
                }
                else if (flags_set(node.pointer_annots.kind, annotation_kind::shared)) {
                    // TODO
                    this->new_line() << "glue_pack(pos, msg, ext, (char*)*" << this->subject() << " - NULL);\n";
                    //    << node.pointer_annots.share_global << ");\n";
                    return true; // No need to walk these (yet)
                }
                else {
                    this->new_line() << "glue_pack(pos, msg, ext, *" << this->subject() << ");\n";
                }
            }
            
            // This is done to absorb any unused variables
            if (!should_walk<marshal_role::marshaling, side>(*node.referent)) {
                this->new_line() << "(void)" << this->subject() << ";\n";
                return true;
            }

            this->new_line() << "if (*" << this->subject() << ") {\n";
            this->marshal("*" + this->subject(), node);
            this->new_line() << "}\n\n";

            return true;
        }

        bool visit_primitive(primitive node)
        {
            this->new_line() << "glue_pack(pos, msg, ext, *" << this->subject() << ");\n";
            return true;
        }

        bool visit_rpc_ptr(rpc_ptr& node)
        {
            this->new_line() << "glue_pack(pos, msg, ext, *" << this->subject() << ");\n";
            return true;
        }

        bool visit_projection(projection& node)
        {
            this->new_line() << get_visitor_name(node) << "(pos, msg, ext, " << this->subject() << ");\n";
            return true;
        }

        bool visit_null_terminated_array(null_terminated_array& node)
        {
            this->new_line() << "size_t i, len;\n";
            this->new_line() << node.element->c_specifier << " const* array = "	<< this->subject() << ";\n";
            this->new_line() << "for (len = 0; array[len]; ++len);\n";
            // The size slot is used for allocation, so we have a +1 for the null terminator
            this->new_line() << "glue_pack(pos, msg, ext, len + 1);\n";
            this->new_line() << "for (i = 0; i < len; ++i) {\n";
            this->new_line() << "\t" << node.element->c_specifier << " const* element = &array[i];\n";
            if (!this->marshal("element", node))
                return false;

            this->new_line() << "}\n\n";

            return true;
        }

        bool visit_static_array(static_array& node)
        {
            this->new_line() << "size_t i, len = " << node.size << ";\n";
            this->new_line() << node.element->c_specifier << " const* array = "	<< this->subject() << ";\n";
            this->new_line() << "glue_pack(pos, msg, ext, len);\n";
            this->new_line() << "// Warning: see David if this breaks\n";
            this->new_line() << "glue_user_trace(\"Warning: see David if this breaks\");\n";
            this->new_line() << "for (i = 0; i < len; ++i) {\n";
            this->new_line() << "\t" << node.element->c_specifier << " const* element = &array[i];\n";
            if (!this->marshal("element", node))
                return false;

            this->new_line() << "}\n\n";

            return true;
        }

        bool visit_value(value& node)
        {
            if (should_walk<marshal_role::marshaling, side>(node)) {
                const auto old = m_should_marshal;
                m_should_marshal = should_marshal<side>(node);
                if (!this->traverse(*this, node))
                    return false;

                m_should_marshal = old;
            }

            return true;
        }

    private:
        bool m_should_marshal {};

        /*
            NOTE: This confused me today; it will confuse you too
            Bind annotations indicate which side of the call has the shadow copy (statically)
            Whether or not they are bound on a side is irrespective of whether we are marshaling or unmarshaling
        */
        static constexpr bool should_bind(const annotation& ann)
        {
            switch (side) {
            case marshal_side::caller:
                return flags_set(ann.kind, annotation_kind::bind_caller);

            case marshal_side::callee:
                return flags_set(ann.kind, annotation_kind::bind_callee);
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
            if constexpr (
                std::is_same_v<node_type, node_ref<null_terminated_array>>
                || std::is_same_v<node_type, node_ref<dyn_array>>
                || std::is_same_v<node_type, node_ref<static_array>>)
                return concat("sizeof(", node.c_specifier, ") * glue_peek(pos, msg, ext)");
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
            if (should_walk<marshal_role::unmarshaling, side>(node)) {
                auto old = std::forward_as_tuple(std::move(m_c_specifier), m_should_marshal);
                m_c_specifier = node.c_specifier;
                m_should_marshal = should_unmarshal<side>(node);
                if (!this->traverse(*this, node))
                    return false;

                std::make_tuple(m_c_specifier, m_should_marshal) = std::move(old);
            }

            return true;
        }

        bool visit_primitive(primitive node)
        {
            this->new_line() << "*" << this->subject() << " = glue_unpack(pos, msg, ext, " << m_c_specifier << ");\n";
            return true;
        }

        // TODO: long an complex, could use some splitting
        // TODO: need to automatically free shadows as they're replaced?
        bool visit_pointer(pointer& node)
        {
            if (m_should_marshal)
                marshal_pointer_value(node);

            if (!should_walk<marshal_role::unmarshaling, side>(*node.referent)) {
                this->new_line() << "(void)" << this->subject() << ";\n";
                return true;
            }
            else {
                return marshal_pointer_child(node);
            }                

            return true;
        }

        // FIXME: consider the fact that these can change size
        bool visit_null_terminated_array(null_terminated_array& node)
        {
            this->new_line() << "size_t i, len;\n";
            this->new_line() << node.element->c_specifier << "* array = " << this->subject() << ";\n";
            // The size slot is used for allocation, so we have a +1 for the null terminator
            this->new_line() << "len = glue_unpack(pos, msg, ext, size_t) - 1;\n";
            this->new_line() << "array[len] = '\\0';\n";
            this->new_line() << "for (i = 0; i < len; ++i) {\n";
            this->new_line() << "\t" << node.element->c_specifier << "* element = &array[i];\n";
            if (!this->marshal("element", node))
                return false;

            this->new_line() << "}\n\n";

            return true;
        }

        // FIXME: we pack the array size before the array elements, but this is redundant
        bool visit_static_array(static_array& node)
        {
            this->new_line() << "int i;\n";
            this->new_line() << node.element->c_specifier << "* array = " << this->subject() << ";\n";
            this->new_line() << "size_t len = glue_unpack(pos, msg, ext, size_t);\n";
            this->new_line() << "// Warning: see David if this breaks\n";
            this->new_line() << "glue_user_trace(\"Warning: see David if this breaks\");\n";
            this->new_line() << "for (i = 0; i < len; ++i) {\n";
            this->new_line() << "\t" << node.element->c_specifier << "* element = &array[i];\n";
            if (!this->marshal("element", node))
                return false;

            this->new_line() << "}\n\n";

            return true;
        }

        bool visit_rpc_ptr(rpc_ptr& node)
        {
            this->new_line() << "*" << this->subject() << " = glue_unpack_rpc_ptr(pos, msg, ext, "
                << node.definition->name << ");\n";

            return true;
        }

        bool visit_projection(projection& node)
        {
            this->new_line() << get_visitor_name(node) << "(pos, msg, ext, " << this->subject() << ");\n";
            return true;
        }

    private:
        absl::string_view m_c_specifier {};
        bool m_should_marshal {};

        static constexpr bool should_bind(const annotation& ann)
        {
            switch (side) {
            case marshal_side::caller:
                return flags_set(ann.kind, annotation_kind::bind_caller);

            case marshal_side::callee:
                return flags_set(ann.kind, annotation_kind::bind_callee);
            }

            std::terminate();
        }

        static constexpr bool should_alloc(const annotation& ann)
        {
            switch (side) {
            case marshal_side::caller:
                return flags_set(ann.kind, annotation_kind::alloc_caller);

            case marshal_side::callee:
                return flags_set(ann.kind, annotation_kind::alloc_callee);
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

        void marshal_pointer_value(pointer& node)
        {
            this->new_line() << "*" << this->subject() << " = ";
            if (should_bind(node.pointer_annots)) {
                this->stream() << "glue_unpack_shadow(pos, msg, ext, " << m_c_specifier << ")";
            }
            else if (should_alloc(node.pointer_annots)) {
                this->stream() << "glue_unpack_new_shadow(pos, msg, ext, " << m_c_specifier << ", "
                    << get_size_expr(*node.referent) << ")";
            }
            else {
                this->stream() << "glue_unpack(pos, msg, ext, " << m_c_specifier << ")";
            }

            this->stream() << ";\n";
        }

        bool marshal_pointer_child(pointer& node)
        {
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
    };
}

#endif
