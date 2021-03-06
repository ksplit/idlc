#include "name_binding.h"

#include <iostream>
#include <vector>

#include "../ast/ast.h"
#include "../ast/ast_walk.h"
#include "../ast/scope.h"

namespace idlc {
    namespace {
        class symbol_walk : public ast_walk<symbol_walk> {
        public:
            bool visit_rpc_def(rpc_def& node);
            bool visit_module_def(module_def& node);
            bool visit_proj_def(proj_def& node);
            bool visit_lock_def(lock_def& node);

        private:
            names_scope* m_scope {};
        };

        class bind_walk : public ast_walk<bind_walk> {
        public:
            bool visit_module_def(module_def& node);
            bool visit_proj_def(proj_def& node);
            bool visit_rpc_def(rpc_def& node);
            bool visit_type_proj(type_proj& node);
            bool visit_type_rpc(type_rpc& node);
            bool visit_lock_scope(lock_scope& node);

        private:
            std::vector<names_scope*> scopes_ {};

            template <typename node_type>
            node_type* try_resolve(ident name)
            {
                auto iter = scopes_.crbegin();
                const auto last = scopes_.crend();
                for (; iter != last; ++iter) {
                    const auto def = (*iter)->get<node_type>(name);
                    if (def)
                        return def;
                }

                return nullptr;
            }
        };

        class scoped_name_walk : public ast_walk<scoped_name_walk> {
        public:
            bool visit_module_def(module_def& node);
            bool visit_rpc_def(rpc_def& node);
            bool visit_proj_def(proj_def& node);

        private:
            std::vector<ident> path_ {};
        };
    }
}

bool idlc::bind_walk::visit_module_def(module_def& node)
{
    scopes_.emplace_back(&node.scope);
    if (!traverse(*this, node))
        return false;

    scopes_.pop_back();

    return true;
}

bool idlc::bind_walk::visit_proj_def(proj_def& node)
{
    scopes_.emplace_back(&node.scope);
    if (!traverse(*this, node))
        return false;

    scopes_.pop_back();

    return true;
}

bool idlc::bind_walk::visit_rpc_def(rpc_def& node)
{
    scopes_.emplace_back(&node.scope);
    if (!traverse(*this, node))
        return false;

    scopes_.pop_back();

    return true;
}

bool idlc::bind_walk::visit_type_proj(type_proj& node)
{
    const auto def = try_resolve<proj_def>(node.name);
    if (def) {
        node.definition = def;
        return traverse(*this, node);
    }

    std::cout << "Error: Could not resolve \"" << node.name << "\"\n";
    std::cout << "In: " << scopes_.back()->get_path() << "\n";

    return false;
}

bool idlc::bind_walk::visit_type_rpc(type_rpc& node)
{
    const auto def = try_resolve<rpc_def>(node.name);
    if (def) {
        node.definition = def;
        return traverse(*this, node);
    }

    std::cout << "Error: Could not resolve \"" << node.name << "\"\n";
    std::cout << "In: " << scopes_.back()->get_path() << "\n";

    return false;
}

bool idlc::bind_walk::visit_lock_scope(lock_scope& node)
{
    const auto def = try_resolve<lock_def>(node.name);
    if (def) {
        node.definition = def;
        return traverse(*this, node);
    }

    std::cout << "Error: Could not resolve \"" << node.name << "\"\n";
    std::cout << "In: " << scopes_.back()->get_path() << "\n";

    return false;
}

bool idlc::symbol_walk::visit_rpc_def(rpc_def& node)
{
    // Careful to insert into the outer scope, *not* the inner one
    m_scope->insert(node.name, &node);
    const auto old = m_scope;
    m_scope = &node.scope;
    if (!traverse(*this, node))
        return false;

    m_scope = old;

    return true;
}

bool idlc::symbol_walk::visit_module_def(module_def& node)
{
    const auto old = m_scope;
    m_scope = &node.scope;
    if (!traverse(*this, node))
        return false;

    m_scope = old;

    return true;
}

bool idlc::symbol_walk::visit_proj_def(proj_def& node)
{
    m_scope->insert(node.name, &node);
    const auto old = m_scope;
    m_scope = &node.scope;
    if (!traverse(*this, node))
        return false;

    m_scope = old;

    return true;
}

bool idlc::symbol_walk::visit_lock_def(lock_def& node)
{
    m_scope->insert(node.name, &node);
    return true;
}

bool idlc::scoped_name_walk::visit_proj_def(proj_def& node)
{
    for (const auto& item : path_) {
        append(node.scoped_name, item, "__"); // double underscore used to reduce odds of collisions
        // FIXME: scope collisions are still possible!
    }

    node.scoped_name += node.name;

    path_.emplace_back(node.name);
    if (!traverse(*this, node))
        return false;

    path_.pop_back();

    return true;
}

bool idlc::scoped_name_walk::visit_rpc_def(rpc_def& node)
{
    path_.emplace_back(node.name);
    node.scope.set_path(path_);
    if (!traverse(*this, node))
        return false;

    path_.pop_back();

    return true;
}

bool idlc::scoped_name_walk::visit_module_def(module_def& node)
{
    path_.emplace_back(node.name);
    node.scope.set_path(path_);
    if (!traverse(*this, node))
        return false;

    path_.pop_back();

    return true;
}

bool idlc::bind_all_names(file& root)
{
    idlc::scoped_name_walk names_walk {};
    idlc::symbol_walk walk {};
    idlc::bind_walk bind_walk {};
    if (!walk.visit_file(root)) {
        std::cout << "Error: scope construction failed\n";
        return false;
    }

    if (!names_walk.visit_file(root)) {
        std::cout << "Error: scoped naming of some items failed\n";
        return false;
    }

    return bind_walk.visit_file(root);
}
