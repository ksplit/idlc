#include "construction.h"

#include <cassert>

#include <gsl/gsl>

#include "tree.h"
#include "../ast/ast.h"
#include "../ast/walk.h"
#include "../sema/nodedb.h"

using namespace idlc;
using namespace idlc::pgraph;

// The AST walk logic for field construction is much nicer to express explicitly than as a walk class

namespace idlc::pgraph {
	namespace {
		// NOTE: to what extent the type_scope_db argument is actually needed here is unclear

		field build_field(
			const ast::tyname& node,
			const sema::type_scope_db& types,
			const sema::type_scope_chain& scope_chain
		);

		layout build_array_layout(
			const ast::tyname_array& node,
			const sema::type_scope_db& types,
			const sema::type_scope_chain& scope_chain
		)
		{
			// TODO: Finish me
			std::cout << "[debug] Noted, but did not construct an array layout\n";
			// array_layout layout {0, std::make_unique<field>(build_field(*node.element, types, scope_chain))};
			return {};
		}

		layout build_arith_layout(
			ast::tyname_arith item,
			const sema::type_scope_db& types,
			const sema::type_scope_chain& scope_chain
		)
		{
			std::cout << "[build_layout] Built a primitive type layout\n";
			return item;
		}

		layout build_string_layout(
			const sema::type_scope_db& types,
			const sema::type_scope_chain& scope_chain
		)
		{
			std::cout << "[build_layout] Built a char string layout\n";
			return std::make_unique<null_array_layout>(
				std::make_unique<field>(
					std::make_unique<layout>(prim::ty_char),
					tags::use_default
				)
			);
		}

		// What is needed here is a state object in which to look up the pgraphs of each projection by node ID
		// I immediately think of folding it into the same logical object as the type_scope_db
		layout build_proj_layout(
			const ast::tyname_proj& item,
			const sema::type_scope_db& types,
			const sema::type_scope_chain& scope_chain
		)
		{
			// NOTE: To be able to share work, reduce the graph size in memory, and just in general preserve the
			// semantics of how projections are lazy-defined, not "templates," we should employ memoization here 
			const auto proj_def = sema::find_type(scope_chain, item.name);
			std::cout << "[debug] Noted and located, but did not construct a projection layout\n";
			// TODO: finish me
			
			return {};
		}

		// TODO: will not be needed
		layout report_unknown_layout()
		{
			std::cout << "[debug] Didn't know how to build this layout\n";
			return {};
		}

		layout build_layout(
			const ast::tyname_stem& node,
			const sema::type_scope_db& types,
			const sema::type_scope_chain& scope_chain
		)
		{
			const auto visit = [&types, &scope_chain](auto&& item) -> layout
			{
				using type = std::decay_t<decltype(item)>;
				if constexpr (std::is_same_v<type, ast::tyname_arith>)
					return build_arith_layout(item, types, scope_chain);
				else if constexpr (std::is_same_v<type, ast::tyname_string>)
					return build_string_layout(types, scope_chain);
				else if constexpr (std::is_same_v<type, ast::node_ref<ast::tyname_proj>>)
					return build_proj_layout(*item, types, scope_chain);
				else if constexpr (std::is_same_v<type, ast::node_ref<ast::tyname_array>>)
					return build_array_layout(*item, types, scope_chain);
				else
					return report_unknown_layout();
			};

			return std::visit(visit, node);
		}

		// TODO: me
		field build_field(
			const ast::tyname& node,
			const sema::type_scope_db& types,
			const sema::type_scope_chain& scope_chain
		)
		{
			Expects((node.attrs & tags::is_ptr) == node.attrs);
			auto lo = build_layout(*node.stem, types, scope_chain);
			for (const auto& star : node.indirs) {
				lo = std::make_unique<ptr>(
					star->attrs & tags::is_ptr,
					std::make_unique<field>(
						std::make_unique<layout>(std::move(lo)),
						star->attrs & tags::is_val
					)
				);
			}

			return field {std::make_unique<layout>(std::move(lo)), node.attrs};
		}

		using rpc_args = std::vector<std::pair<gsl::czstring<>, field>>;

		rpc_args build_rpc_args(
			const std::vector<ast::node_ref<ast::var_decl>>& args,
			const sema::type_scope_db& types,
			const sema::type_scope_chain& scope_chain
		)
		{
			rpc_args new_args {};
			for (const auto& decl : args) {
				const auto& node = *decl;
				new_args.emplace_back(node.name, build_field(*node.type, types, scope_chain));
			}

			return new_args;
		}

		// Not a constructor, rpc_node has no invariants
		rpc_node build_rpc_node(
			rpc_kind kind,
			
			// The parameter group here is do to the need to capture the "in-common" information of the RPC nodes,
			// both direct and indirect. Note that I've left notes elsewhere about merging these node types
			// to avoid the code duplication and allow these and the previous parameter to just be folded together
			// For now, though, nothing else in pgraph construction cares about it, so it's in the backlog
			// TODO: decided on the above considerations
			gsl::czstring<> name,
			const ast::tyname* ret_type,
			// A pointer, since "no arguments" is encoded as this vector being absent
			const std::vector<ast::node_ref<ast::var_decl>>* args,
			
			const sema::type_scope_db& types,
			const sema::type_scope_chain& scope_chain
		)
		{
			return {
				kind,
				name,
				ret_type ? std::make_optional(build_field(*ret_type, types, scope_chain)) : std::nullopt,
				args ? build_rpc_args(*args, types, scope_chain) : rpc_args {}
			};
		}

		// We have no interest in walking into rpc_defs directly
		class rpc_walk : public ast::ast_walk<rpc_walk> {
		public:
			rpc_walk(std::vector<rpc_node>& table, const sema::type_scope_db& types) :
				table_ {table},
				types_ {types}
			{}

			bool visit_rpc_def(const ast::rpc_def& node)
			{
				const auto scope_chain = types_.scope_chains.at(&node);

				std::cout << "[construction] Building direct RPC node " << node.name << "\n";
				table_.emplace_back(build_rpc_node(
					rpc_kind::direct,
					node.name,
					node.ret_type.get(),
					node.arguments.get(),
					types_,
					scope_chain
				));

				return true;
			}

		private:
			std::vector<rpc_node>& table_;
			const sema::type_scope_db& types_;
		};
	}
}

// TODO: use type db
rpc_tables pgraph::build_rpc_table(const ast::file& file, const sema::type_scope_db& types)
{
	rpc_tables tables {};
	rpc_walk walk {tables.direct, types};
	const auto dir_success = walk.visit_file(file);
	assert(dir_success);
	return tables;
}
