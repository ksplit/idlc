#include "name_binding.h"

#include <iostream>
#include <vector>

#include "../ast/ast.h"
#include "../ast/walk.h"
#include "scope.h"

namespace idlc::sema {
	namespace {
		// TODO: a possible performance improvement is in avoiding traversing "uninteresting" subtrees

		class symbol_walk : public ast::ast_walk<symbol_walk> {
		public:
			bool visit_rpc_def(ast::rpc_def& node);
			bool visit_module_def(ast::module_def& node);
			bool visit_proj_def(ast::proj_def& node);

		private:
			scope* scope_ {};
		};

		class bind_walk : public ast::ast_walk<bind_walk> {
		public:
			bool visit_module_def(ast::module_def& node);
			bool visit_rpc_def(ast::rpc_def& node);
			bool visit_type_proj(ast::type_proj& node);
			bool visit_type_rpc(ast::type_rpc& node);

		private:
			std::vector<scope*> scopes_ {};
		};
	}	
}

bool idlc::sema::bind_walk::visit_module_def(ast::module_def& node)
{
	scopes_.emplace_back(&node.scope);
	if (!ast::traverse(*this, node))
		return false;

	scopes_.pop_back();

	return true;
}

bool idlc::sema::bind_walk::visit_rpc_def(ast::rpc_def& node)
{
	scopes_.emplace_back(&node.scope);
	if (!ast::traverse(*this, node))
		return false;

	scopes_.pop_back();

	return true;
}

bool idlc::sema::bind_walk::visit_type_proj(ast::type_proj& node)
{
	auto iter = scopes_.crbegin();
	const auto last = scopes_.crend();
	for (; iter != last; ++iter) {
		const auto def = (*iter)->get<ast::proj_def>(node.name);
		if (def) {
			node.definition = def;
			//std::cout << "[scope_walk] Resolved projection \"" << node.name << "\"\n";
			return ast::traverse(*this, node);
		}
	}

	std::cout << "[error] Could not resolve \"" << node.name << "\"\n";

	return false;
}

bool idlc::sema::bind_walk::visit_type_rpc(ast::type_rpc& node)
{
	auto iter = scopes_.crbegin();
	const auto last = scopes_.crend();
	for (; iter != last; ++iter) {
		const auto def = (*iter)->get<ast::rpc_def>(node.name);
		if (def) {
			node.definition = def;
			//std::cout << "[scope_walk] Resolved RPC \"" << node.name << "\"\n";
			return ast::traverse(*this, node);
		}
	}

	std::cout << "[error] Could not resolve \"" << node.name << "\"\n";

	return false;
}

bool idlc::sema::symbol_walk::visit_rpc_def(ast::rpc_def& node)
{
	// Careful to insert into the outer scope
	scope_->insert(node.name, &node);
	//std::cout << "[scopes] Inserted RPC def \"" << node.name << "\"\n";

	const auto old = scope_;
	scope_ = &node.scope;
	if (!ast::traverse(*this, node))
		return false;

	scope_ = old;

	return true;
}

bool idlc::sema::symbol_walk::visit_module_def(ast::module_def& node)
{
	const auto old = scope_;
	scope_ = &node.scope;
	if (!ast::traverse(*this, node))
		return false;

	scope_ = old;

	return true;
}

bool idlc::sema::symbol_walk::visit_proj_def(ast::proj_def& node)
{
	scope_->insert(node.name, &node);
	//std::cout << "[scopes] Inserted proj def \"" << node.name << "\"\n";
	return true;
}

bool idlc::sema::bind_all(ast::file& root)
{
	idlc::sema::symbol_walk walk {};
	idlc::sema::bind_walk bind_walk {};
	if (!walk.visit_file(root)) {
		std::cout << "Error: scope construction failed\n";
		return false;
	}

	return bind_walk.visit_file(root);
}
