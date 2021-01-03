#include "nodedb.h"

#include <iostream>

#include "../ast/ast.h"
#include "../ast/walk.h"

using namespace idlc;
using namespace idlc::sema;

namespace idlc::sema {
	namespace {
		class type_scope_walk : public ast::ast_walk<type_scope_walk> {
		public:
			type_scope_walk(type_scope_db& dest_db) :
				db_ {dest_db},
				cur_scope_ {},
				cur_chain_ {}
			{}

			bool visit_rpc_def(const ast::rpc_def& node)
			{

				auto& new_scope = *db_.scopes.emplace_back(std::make_unique<types_rib>());
				cur_scope_ = &new_scope;
				cur_chain_.push_back(&new_scope);

				// Need to ensure the produced scope chain includes the RPC's attached scope
				db_.scope_chains[&node] = cur_chain_;
				if (!ast::traverse_rpc_def(*this, node))
					return false;

				cur_chain_.pop_back();

				return true;
			}

			bool visit_module_def(const ast::module_def& node)
			{
				auto& new_scope = *db_.scopes.emplace_back(std::make_unique<types_rib>());
				cur_scope_ = &new_scope;
				cur_chain_.push_back(&new_scope);
				if (!ast::traverse_module_def(*this, node))
					return false;

				cur_chain_.pop_back();

				return true;
			}

			bool visit_struct_proj_def(const ast::struct_proj_def& node)
			{
				db_.scope_chains[&node] = cur_chain_;
				cur_scope_->structs[node.name] = &node;
				return ast::traverse_struct_proj_def(*this, node);
			}

			bool visit_union_proj_def(const ast::union_proj_def& node)
			{
				db_.scope_chains[&node] = cur_chain_;
				cur_scope_->unions[node.name] = &node;
				return ast::traverse_union_proj_def(*this, node);
			}

		private:
			type_scope_db& db_;
			types_rib* cur_scope_ {};
			type_scope_chain cur_chain_ {};
		};

		void dump(const types_rib& rib)
		{
			std::cout << "[type_scope_db]\tScope " << &rib << "\n";
			for (const auto& [name, node] : rib.rpcs)
				std::cout << "[type_scope_db]\t\tRPC ptr (" << name << ", " << node << ")\n";

			for (const auto& [name, node] : rib.structs)
				std::cout << "[type_scope_db]\t\tStruct proj (" << name << ", " << node << ")\n";

			for (const auto& [name, node] : rib.unions)
				std::cout << "[type_scope_db]\t\tUnion proj (" << name << ", " << node << ")\n";
		}
	}
}

type_scope_db idlc::sema::build_types_db(const ast::file& file)
{
	type_scope_db new_db {};
	type_scope_walk walk {new_db};
	const auto success = walk.visit_file(file);
	assert(success);
	return new_db;
}

proj_ref idlc::sema::find_type(const type_scope_chain& scope_chain, gsl::czstring<> name)
{	
	using namespace std;
	auto iter = rbegin(scope_chain);
	const auto end = rend(scope_chain);
	for (; iter != end; ++iter) {
		const auto& scope = *iter;
		std::cout << "[find_type] Searching scope " << scope << "\n";
		const auto str_iter = scope->structs.find(name);
		if (str_iter != scope->structs.end()) {
			std::cout << "[find_type] Found struct def " << str_iter->second << "\n";
			return str_iter->second;
		}

		const auto uni_iter = scope->unions.find(name);
		if (uni_iter != scope->unions.end()) {
			std::cout << "[find_type] Found union def " << uni_iter->second << "\n";
			return uni_iter->second;
		}
	}

	return nullptr;
}

void idlc::sema::dump(const type_scope_db& db)
{
	for (const auto& scope : db.scopes)
		dump(*scope);
	
	for (const auto& [node, chain] : db.scope_chains) {
		std::cout << "[type_scope_db]\tNode " << node << " has chain:\n";
		for (const auto& scope : chain)
			std::cout << "[type_scope_db]\t\t" << &scope << "\n";
	}
}

// TODO: am I needed?
meta_table idlc::sema::build_meta_table(const ast::file& file)
{
	return {};
}
