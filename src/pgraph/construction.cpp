#include "construction.h"

#include <cassert>

#include <gsl/gsl>

#include "tree.h"
#include "../ast/ast.h"
#include "../ast/walk.h"
#include "../sema/nodedb.h"

using namespace idlc;
using namespace idlc::pgraph;

namespace idlc::pgraph {
	namespace {
		field build_field(const ast::tyname& node)
		{
			return {};
		}

		using rpc_args = std::vector<std::pair<gsl::czstring<>, field>>;

		rpc_args build_rpc_args(const std::vector<ast::node_ref<ast::var_decl>>& args)
		{
			rpc_args new_args {};
			for (const auto& decl : args) {
				const auto& node = *decl;
				new_args.emplace_back(node.name, build_field(*node.type));
			}

			return new_args;
		}

		// Not a constructor, rpc_node has no invariants
		rpc_node build_rpc_node(
			rpc_kind kind,
			gsl::czstring<> name,
			const ast::tyname& ret_type,
			// Since no arguments is encoded as this vector being absent
			const std::vector<ast::node_ref<ast::var_decl>>* args
		)
		{
			return {kind, name, build_field(ret_type), args ? build_rpc_args(*args) : rpc_args {}};
		}

		// We have no interest in walking into rpc_defs directly
		class direct_rpc_walk : public ast::ast_walk<direct_rpc_walk> {
		public:
			direct_rpc_walk(std::vector<rpc_node>& table) : table_ {table} {}

			bool visit_rpc_def(const ast::rpc_def& node)
			{
				table_.emplace_back(build_rpc_node(rpc_kind::direct, node.name, *node.ret_type, node.arguments.get()));
				return true;
			}

		private:
			std::vector<rpc_node>& table_;
		};

		class indirect_rpc_walk : public ast::ast_walk<indirect_rpc_walk> {
		public:
			indirect_rpc_walk(std::vector<rpc_node>& table) : table_ {table} {}

			bool visit_rpc_ptr_def(const ast::rpc_ptr_def& node)
			{
				table_.emplace_back(build_rpc_node(rpc_kind::direct, node.name, *node.ret_type, node.arguments.get()));
				return true;
			}

		private:
			std::vector<rpc_node>& table_;
		};
	}
}

rpc_tables pgraph::build_rpc_table(const ast::file& file, const sema::type_scope_db& types)
{
	rpc_tables tables {};
	direct_rpc_walk dir_walk {tables.direct};
	const auto dir_success = dir_walk.visit_file(file);
	assert(dir_success);
	
	indirect_rpc_walk indir_walk {tables.direct};
	const auto indir_success = indir_walk.visit_file(file);
	assert(indir_success);

	return tables;
}
