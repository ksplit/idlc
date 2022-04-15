#ifndef IDLC_AST_AST_H_
#define IDLC_AST_AST_H_

#include <memory>
#include <string>
#include <variant>
#include <vector>

#include <gsl/gsl>

#include "../string_heap.h"
#include "../utility.h"
#include "node_ptrs.h"
#include "pgraph.h"
#include "scope.h"
#include "tag_types.h"

namespace idlc {
    template<typename node_type>
    node_ref<node_type> clone(const node_ref<node_type>& node);

    template<typename node_type>
    ref_vec<node_type> clone(const ref_vec<node_type>& node);

    template<typename node_type>
    node_ptr<node_type> clone(const node_ptr<node_type>& node);

    using ident_vec = std::vector<ident>;

    struct module_def;
    struct driver_def;
    struct driver_file;
    struct type_none;
    struct type_rpc;
    struct type_proj;
    struct type_array;
    struct type_string;
    struct type_casted;
    struct type_spec;

    struct naked_proj_decl;
    struct var_decl;

    struct rpc_def;
    struct global_def;
    struct lock_def;
    struct lock_scope;

    struct tok_kw_null {
    }; // Doesn't exist in parse rules, used as marker (represents tok_kw_null)

    inline tok_kw_null clone(const tok_kw_null&) { return {}; }
    inline unsigned clone(const unsigned& node) { return node; }
    inline ident clone(const ident& node) { return node; }

    using file = std::variant<node_ref<driver_file>, node_ref<ref_vec<module_def>>>;
    // using field_ref = std::variant<node_ref<field_abs_ref>, node_ref<field_rel_ref>>;
    using array_size = std::variant<unsigned, tok_kw_null, ident>;
    using proj_field = std::tuple<
        std::variant<node_ref<var_decl>, node_ref<naked_proj_decl>, node_ref<lock_scope>, node_ref<lock_def>>,
        std::uint8_t
    >;

    inline auto clone(const proj_field& node)
    {
        const auto& [subnode, width] = node;
        const auto visitor = [](auto&& item) -> std::decay_t<decltype(subnode)> { return clone(item); };
        return proj_field {std::visit(visitor, subnode), width};
    }

    using type_stem = std::variant<type_primitive, type_string, type_none, node_ref<type_rpc>, node_ref<type_proj>,
        node_ref<type_array>, node_ref<type_casted>>;

    inline auto clone(const type_primitive& node) { return node; }

    // Another marker
    // NOTE: since these don't actually exist in any meaningful sense, their own parse rules don't produce them
    struct type_string {
    };
    
    inline type_string clone(const type_string&) { return {}; }

    struct type_none {
    };

    inline type_none clone(const type_none&) { return {}; }

    inline auto clone(const array_size& node)
    {
        const auto visitor = [](auto&& item) -> array_size { return clone(item); };
        return std::visit(visitor, node);
    }

    struct driver_def {
        ident name;
        node_ptr<ident_vec> imports;

        driver_def(ident name, node_ptr<ident_vec> imports)
            : name {name}
            , imports {imports}
        {
        }
    };

    struct driver_file {
        node_ptr<ident_vec> former;
        node_ref<driver_def> driver;
        node_ptr<ident_vec> latter;

        driver_file(node_ptr<ident_vec> former, node_ref<driver_def> driver, node_ptr<ident_vec> latter)
            : former {former}
            , driver {driver}
            , latter {latter}
        {
        }
    };

    struct type_proj {
        ident name;

        proj_def* definition;

        type_proj(ident name)
            : name {name}
            , definition {}
        {
        }
    };

    inline auto clone(const type_proj& node)
    {
        return node;
    }

    struct type_rpc {
        ident name;

        rpc_def* definition;

        type_rpc(ident name)
            : name {name}
            , definition {}
        {
        }
    };

    inline auto clone(const type_rpc& node)
    {
        return node;
    }

    struct type_array {
        node_ref<type_spec> element;
        node_ref<array_size> size;

        type_array(node_ref<type_spec> element, node_ref<array_size> size)
            : element {element}
            , size {size}
        {
        }
    };

    inline auto clone(const type_array& node)
    {
        return type_array {clone(node.element), clone(node.size)};
    }

    struct type_casted {
        // NOTE: this type is only here for the benefit of making a c-specifier
        node_ref<type_spec> declared_type;
        node_ref<type_spec> true_type;

        type_casted(node_ref<type_spec> declared_type, node_ref<type_spec> true_type)
            : declared_type {declared_type}
            , true_type {true_type}
        {
        }
    };

    inline auto clone(const type_casted& node)
    {
        return type_casted {clone(node.declared_type), clone(node.true_type)};
    }

    struct indirection {
        node_ref<annotation> attrs; // Contextually, both ptr and value attrs
        bool is_const;

        indirection(node_ref<annotation> attrs, bool is_const)
            : attrs {attrs}
            , is_const {is_const}
        {
        }
    };

    inline auto clone(const annotation& node)
    {
        return node;
    }

    inline auto clone(const indirection& node)
    {
        return indirection {clone(node.attrs), node.is_const};
    }

    struct type_spec {
        node_ref<type_stem> stem;
        ref_vec<indirection> indirs;
        annotation_kind attrs; // Will only ever have value attrs in it
        bool is_const;
        bool is_volatile;

        type_spec(node_ref<type_stem> stem, ref_vec<indirection> indirs, annotation_kind attrs, bool is_const,
            bool is_volatile)
            : stem {stem}
            , indirs {indirs}
            , attrs {attrs}
            , is_const {is_const}
            , is_volatile {is_volatile}
        {
        }
    };

    inline auto clone(const type_spec& node)
    {
        return type_spec {clone(node.stem), clone(node.indirs), node.attrs, node.is_const, node.is_volatile};
    }

    struct var_decl {
        node_ref<type_spec> type;
        ident name;

        var_decl(node_ref<type_spec> type, ident name)
            : type {type}
            , name {name}
        {
        }
    };

    inline auto clone(const var_decl& node)
    {
        return var_decl {clone(node.type), node.name};
    }

    struct naked_proj_decl {
        node_ptr<ref_vec<proj_field>> fields;
        ident name;

        naked_proj_decl(node_ptr<ref_vec<proj_field>> fields, ident name)
            : fields {fields}
            , name {name}
        {
        }
    };

    inline auto clone(const naked_proj_decl& node)
    {
        return naked_proj_decl {clone(node.fields), node.name};
    }

    enum proj_def_kind { struct_kind, union_kind };

    struct proj_def {
        ident name;
        ident type;
        node_ptr<ref_vec<proj_field>> fields;
        proj_def_kind kind;

        std::string scoped_name;
        std::shared_ptr<projection> in_proj;
        std::shared_ptr<projection> out_proj;
        std::shared_ptr<projection> in_out_proj;

        const rpc_def* parent;
        names_scope scope;

        proj_def(ident name, ident type, node_ptr<ref_vec<proj_field>> fields, proj_def_kind kind)
            : name {name}
            , type {type}
            , fields {fields}
            , kind {kind}
            , in_proj {}
            , out_proj {}
            , in_out_proj {}
            , parent {}
            , scope {}
        {
        }
    };

    inline auto clone(const proj_def& node)
    {
        return proj_def {node.name, node.type, clone(node.fields), node.kind};
    }

    using rpc_item = std::variant<node_ref<proj_def>, node_ref<rpc_def>>;

    enum rpc_def_kind {
        direct,
        indirect,
        export_sym,
    };

    enum class lock_state {
        lock,
        unlock
    };

    struct lock_info {
        const lock_def* lock;
        lock_state state;

        lock_info(const lock_def* lock, lock_state state) : lock {lock}, state {state} {}
    };

    struct rpc_def {
        node_ref<type_spec> ret_type;
        ident name;
        node_ptr<ref_vec<var_decl>> arguments;
        node_ptr<ref_vec<rpc_item>> items;
        rpc_def_kind kind;

        node_ptr<lock_info> lock;

        names_scope scope;
        node_ptr<value> ret_pgraph;
        ptr_vec<value> arg_pgraphs;

        std::string enum_id;
        std::string callee_id;
        std::string ctx_id;

        std::string typedef_id;
        std::string impl_typedef_id;
        std::string trmp_id;
        std::string impl_id;

        std::string ret_string;
        std::string args_string;
        std::string params_string;

        rpc_def(node_ptr<type_spec> ret_type, ident name, node_ptr<ref_vec<var_decl>> arguments,
            node_ptr<ref_vec<rpc_item>> items, rpc_def_kind kind, node_ptr<lock_info> lock = nullptr)
            : ret_type {std::move(ret_type)}
            , name {std::move(name)}
            , arguments {std::move(arguments)}
            , items {std::move(items)}
            , kind {std::move(kind)}
            , lock {std::move(lock)}
            , scope {}
            , ret_pgraph {}
            , arg_pgraphs {}
            , enum_id {}
            , callee_id {}
            , typedef_id {}
            , impl_typedef_id {}
            , trmp_id {}
            , impl_id {}
            , ret_string {}
            , args_string {}
            , params_string {}
        {
            append(enum_id, "RPC_ID_", name);
            append(callee_id, name, "_callee");
            append(ctx_id, name, "_call_ctx");
            if (kind == rpc_def_kind::indirect) {
                append(trmp_id, "trmp_", name);
                append(impl_id, "trmp_impl_", name);
                append(typedef_id, "fptr_", name);
                append(impl_typedef_id, "fptr_impl_", name);
            }
        }
    };

    // TODO: impl
    // Headers named in these nodes will get included into the generated "common" header
    struct header_stmt {
    };

    using module_item
        = std::variant<header_stmt, node_ref<proj_def>, node_ref<rpc_def>, node_ref<global_def>, node_ref<lock_def>>;

    struct module_def {
        ident name;
        node_ptr<ref_vec<module_item>> items;

        names_scope scope;

        module_def(ident name, node_ptr<ref_vec<module_item>> items)
            : name {name}
            , items {items}
            , scope {}
        {
        }
    };

    struct global_def {
        ident name;
        node_ref<type_spec> type;
        node_ptr<ref_vec<rpc_item>> items;

        names_scope scope;
        node_ptr<value> pgraph;

        global_def(ident name, node_ref<type_spec> type, node_ptr<ref_vec<rpc_item>> items)
            : name {name}
            , type {type}
            , items {items}
            , scope {}
            , pgraph {}
        {
        }
    };

    enum class lock_type { spinlock };

    struct lock_def {
        ident name;
        lock_type type;

        proj_def* parent;

        lock_def(ident name, lock_type type)
            : name {name}
            , type {type}
            , parent {}
        {
        }
    };

    inline auto clone(const lock_def& node)
    {
        return node;
    }

    struct lock_scope {
        ident name;
        ident_vec fields;

        const lock_def* definition;
        proj_def* parent;

        lock_scope(ident name, ident_vec&& fields)
            : name {name}
            , fields {fields}
        {
        }
    };

    inline auto clone(const lock_scope& node)
    {
        return node;
    }

    /*
        Nodes that have scopes: module_defs, rpc_defs, proj_defs, global_defs
    */

    inline auto clone(const type_stem& node)
    {
        const auto visitor = [](auto&& item) -> type_stem { return clone(item); };
        return std::visit(visitor, node);
    }

    template<typename node_type>
    node_ref<node_type> clone(const node_ref<node_type>& node)
    {
        return std::make_shared<node_type>(clone(*node));
    }

    template<typename node_type>
    node_ptr<node_type> clone(const node_ptr<node_type>& node)
    {
        return node ? std::make_shared<node_type>(clone(*node)) : node_ptr<node_type> {};
    }

    template<typename node_type>
    ref_vec<node_type> clone(const ref_vec<node_type>& node)
    {
        auto copy = node;
        for (auto& item : copy)
            item = clone(item);
        
        return copy;
    }
}

#endif
