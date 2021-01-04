#ifndef IDLC_SEMA_SCOPE_WALK_H
#define IDLC_SEMA_SCOPE_WALK_H

#include <iostream>

#include "../ast/walk.h"
#include "../ast/ast.h"
#include "scope.h"

namespace idlc::sema {
	class symbol_walk : public ast::ast_walk<symbol_walk> {
	public:
		bool visit_rpc_def(ast::rpc_def& node)
		{
			// Careful to insert into the outer scope
			scope_->insert(node.name, &node);
			std::cout << "[scopes] Inserted RPC def \"" << node.name << "\"\n";

			const auto old = scope_;
			scope_ = &node.scope;
			if (!ast::traverse(*this, node))
				return false;

			scope_ = old;

			return true;
		}

		bool visit_module_def(ast::module_def& node)
		{
			const auto old = scope_;
			scope_ = &node.scope;
			if (!ast::traverse(*this, node))
				return false;

			scope_ = old;

			return true;
		}

		bool visit_proj_def(ast::proj_def& node)
		{
			scope_->insert(node.name, &node);
			std::cout << "[scopes] Inserted proj def \"" << node.name << "\"\n";
			return true;
		}

	private:
		scope* scope_ {};
	};
}

#endif
