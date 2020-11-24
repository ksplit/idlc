#ifndef _LCDS_IDL_RESOLUTION_H_
#define _LCDS_IDL_RESOLUTION_H_

#include "parser/walk.h"

#include <map>
#include <list>

namespace idlc {
	struct types_rib {
		std::map<gsl::czstring<>, const parser::rpc_ptr_def*> rpcs {};
		std::map<gsl::czstring<>, const parser::struct_proj_def*> structs {};
		std::map<gsl::czstring<>, const parser::union_proj_def*> unions {};
	};

	struct values_rib {
		std::map<gsl::czstring<>, const parser::naked_proj_decl*> naked {};
		std::map<gsl::czstring<>, const parser::var_decl*> vars {};
	};

	class scopes_pass : public parser::ast_walk<scopes_pass> {
	public:
		std::map<const void*, types_rib*> type_scopes_;
		std::map<const void*, values_rib*> val_scopes_;
		
		bool visit_module_def(const parser::module_def& node)
		{
			types_.push_back({}); // TODO: inefficient?
			// TODO: If we ever do node memoization, using their address won't work
			type_scopes_[&node] = &types_.back();
			std::cout << "Creating type scope for module def \"" << node.name << "\"" << std::endl;
			return parser::traverse_module_def(*this, node);
		}

		bool visit_rpc_def(const parser::rpc_def& node)
		{
			types_.push_back({}); // TODO: inefficient?
			type_scopes_[&node] = &types_.back();
			std::cout << "Creating type scope for rpc def \"" << node.name << "\"" << std::endl;

			values_.push_back({});
			val_scopes_[&node] = &values_.back();
			std::cout << "Creating value scope for rpc def \"" << node.name << "\"" << std::endl;
			
			return parser::traverse_rpc_def(*this, node);
		}

		bool visit_rpc_ptr_def(const parser::rpc_ptr_def& node)
		{
			types_.push_back({}); // TODO: inefficient?
			type_scopes_[&node] = &types_.back();
			std::cout << "Creating type scope for rpc pointer def \"" << node.name << "\"" << std::endl;

			values_.push_back({});
			val_scopes_[&node] = &values_.back();
			std::cout << "Creating value scope for rpc pointer def \"" << node.name << "\"" << std::endl;

			return parser::traverse_rpc_ptr_def(*this, node);
		}

		bool visit_naked_proj_decl(const parser::naked_proj_decl& node)
		{
			values_.push_back({});
			val_scopes_[&node] = &values_.back();
			std::cout << "Creating value scope for naked decl \"" << node.name << "\"" << std::endl;
			return parser::traverse_naked_proj_decl(*this, node);
		}

		bool visit_struct_proj_def(const parser::struct_proj_def& node)
		{
			values_.push_back({});
			val_scopes_[&node] = &values_.back();
			std::cout << "Creating value scope for struct def \"" << node.name << "\"" << std::endl;
			return parser::traverse_struct_proj_def(*this, node);
		}

		bool visit_union_proj_def(const parser::union_proj_def& node)
		{
			values_.push_back({});
			val_scopes_[&node] = &values_.back();
			std::cout << "Creating value scope for union def \"" << node.name << "\"" << std::endl;
			return parser::traverse_union_proj_def(*this, node);
		}

	private:
		// To avoid pointer invalidation
		// The primary output of this (admttedly inefficient) pass is a map of node pointers to corresponding scopes
		std::list<types_rib> types_;
		std::list<values_rib> values_;
	};

	class names_pass : public parser::ast_walk<names_pass> {
	public:
		names_pass(std::map<const void*, types_rib*>& tscopes, std::map<const void*, values_rib*>& vscopes) :
			tscopes_ {tscopes},
			vscopes_ {vscopes},
			types_ {},
			values_ {}
		{
		}

		bool visit_module_def(const parser::module_def& node)
		{
			const auto old = types_;
			types_ = tscopes_[&node];
			if (!traverse_module_def(*this, node))
				return false;

			types_ = old;
			return true;
		}

		bool visit_rpc_def(const parser::rpc_def& node)
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

		bool visit_struct_proj_def(const parser::struct_proj_def& node)
		{
			types_->structs[node.name] = &node;

			const auto v_old = values_;
			values_ = vscopes_[&node];

			std::cout << "Inserted struct projection \"" << node.name << "\"" << std::endl;

			if (!traverse_struct_proj_def(*this, node))
				return false;

			values_ = v_old;

			return true;
		}

		bool visit_union_proj_def(const parser::union_proj_def& node)
		{
			types_->unions[node.name] = &node;

			const auto v_old = values_;
			values_ = vscopes_[&node];

			std::cout << "Inserted union projection \"" << node.name << "\"" << std::endl;

			if (!traverse_union_proj_def(*this, node))
				return false;

			values_ = v_old;

			return true;
		}

		bool visit_rpc_ptr_def(const parser::rpc_ptr_def& node)
		{
			types_->rpcs[node.name] = &node;
			std::cout << "Inserted rpc pointer \"" << node.name << "\"" << std::endl;

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

		bool visit_naked_proj_decl(const parser::naked_proj_decl& node)
		{
			values_->naked[node.name] = &node;
			std::cout << "Inserted naked projection declaration \"" << node.name << "\"" << std::endl;
			
			const auto v_old = values_;
			values_ = vscopes_[&node];

			if (!traverse_naked_proj_decl(*this, node))
				return false;

			values_ = v_old;

			return true;
		}

		bool visit_var_decl(const parser::var_decl& node)
		{
			values_->vars[node.name] = &node;
			std::cout << "Inserted var declaration \"" << node.name << "\"" << std::endl;
			return traverse_var_decl(*this, node);
		}

	private:
		std::map<const void*, types_rib*>& tscopes_;
		std::map<const void*, values_rib*>& vscopes_;
		types_rib* types_ {};
		values_rib* values_ {};
	};
}

#endif
