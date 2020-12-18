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
		class build_field_pass : public ast::ast_walk<build_field_pass> {
		public:


		private:
		};

		layout build_layout(
			const ast::tyname_array& node,
			const sema::type_scope_db& types,
			const sema::type_scope_chain& scope_chain
		)
		{
			// TODO: Finish me
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
				if constexpr (std::is_same_v<type, ast::tyname_arith>) {
					std::cout << "[build_layout] Built a primitive type layout\n";
					return item;
				}
				else if constexpr (std::is_same_v<type, ast::tyname_string>) {
					std::cout << "[build_layout] Built a char string layout\n";
					return std::make_unique<null_array_layout>(
						std::make_unique<field>(
							std::make_unique<layout>(prim::ty_char),
							tags::use_default
						)
					);
				}
				else if constexpr (std::is_same_v<type, ast::node_ref<ast::tyname_proj>>) {
					const auto proj_def = sema::find_type(scope_chain, item->name);
					// TODO: finish this
					return {};
				}
				else if constexpr (std::is_same_v<type, ast::node_ref<ast::tyname_array>>) {
					return build_layout(*item, types, scope_chain);
				}
				else {
					std::cout << "[warning] Didn't know how to build this layout\n";
					return {};
				}
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
			gsl::czstring<> name,
			const ast::tyname* ret_type,
			// Since "no arguments" is encoded as this vector being absent
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
		class direct_rpc_walk : public ast::ast_walk<direct_rpc_walk> {
		public:
			direct_rpc_walk(std::vector<rpc_node>& table, const sema::type_scope_db& types) :
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

		class indirect_rpc_walk : public ast::ast_walk<indirect_rpc_walk> {
		public:
			indirect_rpc_walk(std::vector<rpc_node>& table, const sema::type_scope_db& types) :
				table_ {table},
				types_ {types}
			{}

			bool visit_rpc_ptr_def(const ast::rpc_ptr_def& node)
			{
				const auto scope_chain = types_.scope_chains.at(&node);

				std::cout << "[construction] Building indirect RPC node " << node.name << "\n";
				table_.emplace_back(build_rpc_node(
					rpc_kind::indirect,
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
	direct_rpc_walk dir_walk {tables.direct, types};
	const auto dir_success = dir_walk.visit_file(file);
	assert(dir_success);
	
	indirect_rpc_walk indir_walk {tables.direct, types};
	const auto indir_success = indir_walk.visit_file(file);
	assert(indir_success);

	return tables;
}
