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
        if (flags_set(node.value_annots, annotation_kind::unused))
            return false;

        switch (role) {
        case marshal_role::marshaling:
            return is_nonterminal(node) || should_marshal<side>(node);

        case marshal_role::unmarshaling:
            return is_nonterminal(node) || should_unmarshal<side>(node);
        }

        std::terminate();
    }

    /*
        NOTE: This confused me today; it will confuse you too
        Bind annotations indicate which side of the call has the shadow copy (statically)
        Whether or not they are bound on a side is irrespective of whether we are marshaling or unmarshaling
    */
    template<marshal_side side>
    static constexpr bool should_bind(const annotation& ann)
    {
        switch (side) {
        case marshal_side::caller:
            return flags_set(ann.kind, annotation_kind::bind_caller)
                | flags_set(ann.kind, annotation_kind::bind_memberof_caller);

        case marshal_side::callee:
            return flags_set(ann.kind, annotation_kind::bind_callee)
                | flags_set(ann.kind, annotation_kind::bind_memberof_callee);
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
                // Should_bind takes care of the details, so we only check if either of these flags are set
                // NOTE: adjustment is mandatory! Currently undefined outside of bind, but still
                if (!is_clear(node.pointer_annots.kind & annotation_kind::is_bind_memberof)) {
                    const auto& offset = node.pointer_annots.member;
                    // Use container_of as it abstracts away the arithmetic and also typecasts the pointer
                    this->new_line() << "struct " << offset.struct_type << "* __adjusted = container_of(*" << this->subject()
                        << ", " << "struct " << offset.struct_type << ", " << offset.field << ");\n";
                }
                else {
                    this->new_line() << "const void* __adjusted = *" << this->subject() << ";\n";
                }

                if (should_bind<side>(node.pointer_annots)) {
                    this->new_line() << "glue_pack_shadow(__pos, msg, ext, __adjusted);\n";
                }
                else if (flags_set(node.pointer_annots.kind, annotation_kind::shared)) {
                    this->new_line() << "glue_pack(__pos, msg, ext, (void*)__adjusted - "
                        << node.pointer_annots.share_global << ");\n";

                    return true; // No need to walk these (yet)
                }
                else {
                    this->new_line() << "glue_pack(__pos, msg, ext, __adjusted);\n";
                }
            }
            
            // This is done to absorb any unused variables
            if (!should_walk<marshal_role::marshaling, side>(*node.referent)) {
                this->new_line() << "(void)" << this->subject() << ";\n";
            }
            else {
                this->new_line() << "if (*" << this->subject() << ") {\n";
                this->marshal("*" + this->subject(), node);
                this->new_line() << "}\n\n";
            }

            if (should_dealloc(node.pointer_annots))
                this->new_line() << "glue_remove_shadow(*" << this->subject() << ");\n";

            return true;
        }

        bool visit_primitive(primitive node)
        {
            this->new_line() << "glue_pack(__pos, msg, ext, *" << this->subject() << ");\n";
            return true;
        }

        bool visit_rpc_ptr(rpc_ptr& node)
        {
            this->new_line() << "glue_pack(__pos, msg, ext, *" << this->subject() << ");\n";
            return true;
        }

        bool visit_projection(projection& node)
        {
            this->new_line() << get_visitor_name(node) << "(__pos, msg, ext, "
                << ((node.def->parent) ? "ctx, " : "") << this->subject() << ");\n";

            return true;
        }

        bool visit_null_terminated_array(null_terminated_array& node)
        {
            auto p = std::get_if<node_ref<projection>>(&node.element->type);
            this->new_line() << "size_t i, len;\n";

            // If it's a projection type, define sentinel
            if (p)
                this->new_line() << node.element->c_specifier << " sentinel = { 0 };\n";

            this->new_line() << node.element->c_specifier << " const* array = "	<< this->subject() << ";\n";

            // XXX: Caution! This assumes the sentinels end with a zero-filled element!
            if (p)
                this->new_line() << "for (len = 0; memcmp(&array[len], &sentinel, sizeof(array[0])); ++len) ;\n";
            else
                this->new_line() << "for (len = 0; array[len]; ++len);\n";

            // The size slot is used for allocation, so we have a +1 for the null terminator
            this->new_line() << "glue_pack(__pos, msg, ext, len + 1);\n";
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
            this->new_line() << "glue_pack(__pos, msg, ext, len);\n";
            this->new_line() << "// Warning: see David if this breaks\n";
            this->new_line() << "glue_user_trace(\"Warning: see David if this breaks\");\n";
            this->new_line() << "for (i = 0; i < len; ++i) {\n";
            this->new_line() << "\t" << node.element->c_specifier << " const* element = &array[i];\n";
            if (!this->marshal("element", node))
                return false;

            this->new_line() << "}\n\n";

            return true;
        }

        bool visit_dyn_array(dyn_array& node)
        {
            const auto star = node.element->is_const ? " const*" : "*";
            const auto ptr_type = concat(node.element->c_specifier, star);

            // HACK: this depends on the root pointer naming convention
            this->new_line() << "size_t i, len = (" << node.size_expr << ");\n";
            this->new_line() << ptr_type << " array = "	<< this->subject() << ";\n";
            this->new_line() << "glue_pack(__pos, msg, ext, len);\n";
            this->new_line() << "// Warning: see David if this breaks\n";
            this->new_line() << "glue_user_trace(\"Warning: see David if this breaks\");\n";
            this->new_line() << "for (i = 0; i < len; ++i) {\n";
            this->new_line() << "\t" << ptr_type << " element = &array[i];\n";
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

        static constexpr bool should_dealloc(const annotation& ann)
        {
            if (side == marshal_side::callee)
                return flags_set(ann.kind, annotation_kind::dealloc_callee);
            return false;
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
                return concat("sizeof(", node.c_specifier, ") * glue_peek(__pos, msg, ext)");
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

        static constexpr bool should_dealloc(const annotation& ann)
        {
            if (side == marshal_side::caller)
                return flags_set(ann.kind, annotation_kind::dealloc_caller);
            return false;
        }


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
            this->new_line() << "*" << this->subject() << " = glue_unpack(__pos, msg, ext, " << m_c_specifier << ");\n";
            return true;
        }

        // TODO: need to automatically free shadows as they're replaced?
        bool visit_pointer(pointer& node)
        {

            if (should_dealloc(node.pointer_annots))
                this->new_line() << "glue_remove_shadow(*" << this->subject() << ");\n";

            if (m_should_marshal) {

                // NOTE: will return false if no need to walk further (i.e., the pointer is "opaque")
                if (!marshal_pointer_value(node))
                    return true;

            }

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
            auto p = std::get_if<node_ref<projection>>(&node.element->type);

            this->new_line() << "size_t i, len;\n";
            this->new_line() << node.element->c_specifier << "* array = " << this->subject() << ";\n";

            // The size slot is used for allocation, so we have a +1 for the null terminator
            this->new_line() << "len = glue_unpack(__pos, msg, ext, size_t) - 1;\n";

            // zero-fill the last element of the shadow-ed projection to make it a sentinel
            if (p)
                this->new_line() << "memset(&array[len], 0x0, sizeof(array[len]));\n";
            else
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
            this->new_line() << "size_t len = glue_unpack(__pos, msg, ext, size_t);\n";
            this->new_line() << "// Warning: see David if this breaks\n";
            this->new_line() << "glue_user_trace(\"Warning: see David if this breaks\");\n";
            this->new_line() << "for (i = 0; i < len; ++i) {\n";
            this->new_line() << "\t" << node.element->c_specifier << "* element = &array[i];\n";
            if (!this->marshal("element", node))
                return false;

            this->new_line() << "}\n\n";

            return true;
        }

        // FIXME: we pack the array size before the array elements, but this is redundant
        bool visit_dyn_array(dyn_array& node)
        {
            this->new_line() << "int i;\n";
            this->new_line() << node.element->c_specifier << "* array = " << this->subject() << ";\n";
            this->new_line() << "size_t len = glue_unpack(__pos, msg, ext, size_t);\n";
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
            this->new_line() << "*" << this->subject() << " = glue_unpack_rpc_ptr(__pos, msg, ext, "
                << node.definition->name << ");\n";

            return true;
        }

        bool visit_projection(projection& node)
        {
            this->new_line() << get_visitor_name(node) << "(__pos, msg, ext, "
                << ((node.def->parent) ? "ctx, " : "") << this->subject() << ");\n";
            
            return true;
        }

    private:
        absl::string_view m_c_specifier {};
        bool m_should_marshal {};

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

        static constexpr bool should_alloc_once(const annotation& ann)
        {
            switch (side) {
            case marshal_side::caller:
                return flags_set(ann.kind, annotation_kind::alloc_once_caller);

            case marshal_side::callee:
                return flags_set(ann.kind, annotation_kind::alloc_once_callee);
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

        [[nodiscard]] bool marshal_pointer_value(pointer& node)
        {
            if (!is_clear(node.pointer_annots.kind & annotation_kind::is_bind_memberof)) {
                // special case! early return
                marshal_bind_memberof_pointer(node);
                return true;
            }

            this->new_line() << "*" << this->subject() << " = ";

            if (should_bind<side>(node.pointer_annots)) {
                // Should_bind takes care of the details, so we only check if either of these flags are set
                this->stream() << "glue_unpack_shadow(__pos, msg, ext, " << m_c_specifier << ");\n";
            }
            else if (should_alloc(node.pointer_annots)) {
                auto alloc_flags = node.pointer_annots.flags_verbatim ? node.pointer_annots.flags_verbatim : "DEFAULT_GFP_FLAGS";
                std::string alloc_size = node.pointer_annots.size_verbatim ? node.pointer_annots.size_verbatim : "";

                // if the size field is empty (i.e., `{{}}`), use referent size
                if (alloc_size.empty())
                    alloc_size = get_size_expr(*node.referent);

                this->stream() << "glue_unpack_new_shadow(__pos, msg, ext, " << m_c_specifier << ", ("
                    << alloc_size << "), (" << alloc_flags << "));\n";
            }
            else if (should_alloc_once(node.pointer_annots)) {
                this->stream() << "glue_unpack_bind_or_new_shadow(__pos, msg, ext, " << m_c_specifier << ", "
                    << get_size_expr(*node.referent) << ");\n";
            }
            else if (flags_set(node.pointer_annots.kind, annotation_kind::shared)) {
                this->stream() << "(" << m_c_specifier << ")(glue_unpack(__pos, msg, ext, size_t) + "
                    << node.pointer_annots.share_global << ");\n";

                return false;
            }
            else {
                this->stream() << "glue_unpack(__pos, msg, ext, " << m_c_specifier << ");\n";
            }

            return true;
        }

        void marshal_bind_memberof_pointer(pointer& node)
        {
            const auto& offset = node.pointer_annots.member;
            this->new_line() << "struct " << offset.struct_type << "* __" << offset.struct_type << " = ";

            if (flags_set(node.pointer_annots.kind, annotation_kind::bind_memberof_callee))
                this->stream() << "glue_unpack_shadow(__pos, msg, ext, struct " << offset.struct_type << "*);\n";
            else if (flags_set(node.pointer_annots.kind, annotation_kind::bind_memberof_caller))
                this->stream() << "glue_unpack(__pos, msg, ext, struct " << offset.struct_type << "*);\n";
            else
                std::terminate();

            this->new_line() << "*" << this->subject() << " = &__" << offset.struct_type
                << "->" << offset.field << ";\n";
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
