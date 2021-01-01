#ifndef _LCDS_IDL_RESOLUTION_H_
#define _LCDS_IDL_RESOLUTION_H_

#include "../ast/walk.h"

#include <map>
#include <list>

// TODO: create a walk that other walks can subclass that provides the immediately enclosing scope
/*
	Essentially, we want to create a standard mechanism via which any walk can have access to scope-chain information
	for a node on demand

	A lot of things can be done quite conveniently with parent pointers
	Since walks walk the whole tree or stop at some point along the walk, it's best to compute parents for all nodes
	at once

	An even more useful thing might be a node database that allows us to associate any node with its parent, type,
	and data from a particular node ID.

	Rustc inspires me with query-based compilation.

	scope_chain(node) would return a list of scopes that enclose a particular node
	If it has not been computed, it does so by walking up the parent pointers and querying scope(node)
	on each. scope(node) returns nothing if the node has no associated scope, but fills the scope with a subtree
	walk if needed.

	NOTE: it is trivial to do computation over a subtree, but parent pointers can not be computed for a subtree only
	in any useful sense, so we compute all the node db data in one pass.
*/

namespace idlc::sema {
	struct _values_rib {
		std::map<gsl::czstring<>, const ast::naked_proj_decl*> naked {};
		std::map<gsl::czstring<>, const ast::var_decl*> vars {};
	};

	using _trib_map = std::map<const void*, types_rib*>;
	using _vrib_map = std::map<const void*, _values_rib*>;

	// NOTE: unused
	class _scopes_pass : public ast::ast_walk<_scopes_pass> {
	public:
		_trib_map type_scopes_;
		_vrib_map val_scopes_;
		
		bool visit_module_def(const ast::module_def& node)
		{
			types_.push_back({}); // TODO: inefficient?
			// TODO: If we ever do node memoization, using their address won't work
			type_scopes_[&node] = &types_.back();
			std::cout << "[Scopes] Creating type scope for module def \"" << node.name << "\"" << std::endl;
			return ast::traverse_module_def(*this, node);
		}

		bool visit_rpc_def(const ast::rpc_def& node)
		{
			types_.push_back({}); // TODO: inefficient?
			type_scopes_[&node] = &types_.back();
			std::cout << "[Scopes] Creating type scope for rpc def \"" << node.name << "\"" << std::endl;

			values_.push_back({});
			val_scopes_[&node] = &values_.back();
			std::cout << "[Scopes] Creating value scope for rpc def \"" << node.name << "\"" << std::endl;
			
			return ast::traverse_rpc_def(*this, node);
		}

		bool visit_rpc_ptr_def(const ast::rpc_ptr_def& node)
		{
			types_.push_back({}); // TODO: inefficient?
			type_scopes_[&node] = &types_.back();
			std::cout << "[Scopes] Creating type scope for rpc pointer def \"" << node.name << "\"" << std::endl;

			values_.push_back({});
			val_scopes_[&node] = &values_.back();
			std::cout << "[Scopes] Creating value scope for rpc pointer def \"" << node.name << "\"" << std::endl;

			return ast::traverse_rpc_ptr_def(*this, node);
		}

		bool visit_naked_proj_decl(const ast::naked_proj_decl& node)
		{
			values_.push_back({});
			val_scopes_[&node] = &values_.back();
			std::cout << "[Scopes] Creating value scope for naked decl \"" << node.name << "\"" << std::endl;
			return ast::traverse_naked_proj_decl(*this, node);
		}

		bool visit_struct_proj_def(const ast::struct_proj_def& node)
		{
			values_.push_back({});
			val_scopes_[&node] = &values_.back();
			std::cout << "[Scopes] Creating value scope for struct def \"" << node.name << "\"" << std::endl;
			return ast::traverse_struct_proj_def(*this, node);
		}

		bool visit_union_proj_def(const ast::union_proj_def& node)
		{
			values_.push_back({});
			val_scopes_[&node] = &values_.back();
			std::cout << "[Scopes] Creating value scope for union def \"" << node.name << "\"" << std::endl;
			return ast::traverse_union_proj_def(*this, node);
		}

	private:
		// To avoid pointer invalidation
		// The primary output of this (admttedly inefficient) pass is a map of node pointers to corresponding scopes
		std::list<types_rib> types_;
		std::list<_values_rib> values_;
	};

	// NOTE: unused
	class _names_pass : public ast::ast_walk<_names_pass> {
	public:
		_names_pass(std::map<const void*, types_rib*>& tscopes, std::map<const void*, _values_rib*>& vscopes) :
			tscopes_ {tscopes},
			vscopes_ {vscopes},
			types_ {},
			values_ {}
		{
		}

		bool visit_module_def(const ast::module_def& node)
		{
			const auto old = types_;
			types_ = tscopes_[&node];
			if (!traverse_module_def(*this, node))
				return false;

			types_ = old;
			return true;
		}

		bool visit_rpc_def(const ast::rpc_def& node)
		{
			const auto t_old = types_;
			types_ = tscopes_[&node];
			const auto v_old = values_;
			values_ = vscopes_[&node];

			if (!traverse_rpc_def(*this, node))
				return false;

			types_ = t_old;
			values_ = v_old;

			return true;
		}

		bool visit_struct_proj_def(const ast::struct_proj_def& node)
		{
			types_->structs[node.name] = &node;

			const auto v_old = values_;
			values_ = vscopes_[&node];

			std::cout << "[Scopes] Inserted struct projection \"" << node.name << "\"" << std::endl;

			if (!traverse_struct_proj_def(*this, node))
				return false;

			values_ = v_old;

			return true;
		}

		bool visit_union_proj_def(const ast::union_proj_def& node)
		{
			types_->unions[node.name] = &node;

			const auto v_old = values_;
			values_ = vscopes_[&node];

			std::cout << "[Scopes] Inserted union projection \"" << node.name << "\"" << std::endl;

			if (!traverse_union_proj_def(*this, node))
				return false;

			values_ = v_old;

			return true;
		}

		bool visit_rpc_ptr_def(const ast::rpc_ptr_def& node)
		{
			types_->rpcs[node.name] = &node;
			std::cout << "[Scopes] Inserted rpc pointer \"" << node.name << "\"" << std::endl;

			const auto t_old = types_;
			types_ = tscopes_[&node];
			const auto v_old = values_;
			values_ = vscopes_[&node];
			
			if (!traverse_rpc_ptr_def(*this, node))
				return false;

			types_ = t_old;
			values_ = v_old;

			return true;
		}

		bool visit_naked_proj_decl(const ast::naked_proj_decl& node)
		{
			values_->naked[node.name] = &node;
			std::cout << "[Scopes] Inserted naked projection declaration \"" << node.name << "\"" << std::endl;
			
			const auto v_old = values_;
			values_ = vscopes_[&node];

			if (!traverse_naked_proj_decl(*this, node))
				return false;

			values_ = v_old;

			return true;
		}

		bool visit_var_decl(const ast::var_decl& node)
		{
			values_->vars[node.name] = &node;
			std::cout << "[Scopes] Inserted var declaration \"" << node.name << "\"" << std::endl;
			return traverse_var_decl(*this, node);
		}

	private:
		std::map<const void*, types_rib*>& tscopes_;
		std::map<const void*, _values_rib*>& vscopes_;
		types_rib* types_ {};
		_values_rib* values_ {};
	};
}

#endif
