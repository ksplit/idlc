#ifndef _LCDS_IDL_MARSHAL_TREE_BUILD_H_
#define _LCDS_IDL_MARSHAL_TREE_BUILD_H_

#include "../ast/walk.h"
#include "../sema/resolution.h"
#include "../pgraph/tree.h"

#include <gsl/gsl>

namespace idlc::marshal {
	// NOTE: this is shallow
	// FIXME: why is it shallow?
	// NOTE: shallowness allows us to assign to each layout output exactly once
	class type_layout_pass : public ast::ast_walk<type_layout_pass> {
	public:
		type_layout_pass(pgraph::layout& out) : out_ {out}
		{
		}

		bool visit_tyname_rpc(const ast::tyname_rpc& node)
		{
			std::cout << "[Passgraph] Adding layout for rpc pointer\n";
			out_ = std::make_unique<pgraph::rpc_ptr_layout>(pgraph::rpc_ptr_layout {node.name});
			return true;
		}

		// Handles both string markers and primitives
		bool visit_tyname_stem(const ast::tyname_stem& node)
		{
			const auto visit = [this, &node](auto&& inner)
			{
				using type = std::decay_t<decltype(inner)>;
				if constexpr (std::is_same_v<type, ast::tyname_string>) {
					std::cout << "[Passgraph] Adding layout for string value (NOT A POINTER)\n";
					// NOTE: strings are just null-terminated arrays of char
					// passgraph makes no distinction
					out_ = std::make_unique<pgraph::null_array_layout>(pgraph::null_array_layout {
						std::make_unique<pgraph::layout>(pgraph::prim::ty_char)
					});
				}
				else if constexpr (std::is_same_v<type, ast::tyname_arith>) {
					std::cout << "[Passgraph] Adding layout for arithmetic type\n";
					out_ = inner;
				}
				else {
					// Delegate
					ast::traverse_tyname_stem(*this, node);
				}
			};

			std::visit(visit, node);

			return true;
		}

		bool visit_tyname_array(const ast::tyname_array& node)
		{
			pgraph::layout elem {};
			type_layout_pass lpass {elem};
			ast::traverse_tyname(lpass, *node.element);
			const auto visit = [this, &elem](auto&& inner)
			{
				using type = std::decay_t<decltype(inner)>;
				if constexpr (std::is_same_v<type, unsigned>) {
					std::cout << "[Passgraph] Adding const-sized array layout\n";
					out_ = std::make_unique<pgraph::array_layout>(pgraph::array_layout {
						inner,
						std::make_unique<pgraph::layout>(std::move(elem))
					});
				}
				else if constexpr (std::is_same_v<type, ast::tok_kw_null>) {
					std::cout << "[Passgraph] Adding null-terminated array layout\n";
					out_ = std::make_unique<pgraph::null_array_layout>(pgraph::null_array_layout {
						std::make_unique<pgraph::layout>(std::move(elem))
					});
				}
				else if constexpr (std::is_same_v<type, gsl::czstring<>>) {
					std::cout << "[Passgraph] Adding variable-size array layout\n";
					out_ = std::make_unique<pgraph::dyn_array_layout>(pgraph::dyn_array_layout {
						inner,
						std::make_unique<pgraph::layout>(std::move(elem))
					});
				}
			};

			std::visit(visit, *node.size);

			return true;
		}

		bool visit_tyname(const ast::tyname& node)
		{
			// FIXME: const-ness information lost in-tree

			pgraph::layout root_layout {};
			ast::traverse_tyname(type_layout_pass {root_layout}, node);
			for (const auto& star : node.indirs) {
				std::cout << "[Passgraph] Applying indirection\n";
				root_layout = std::make_unique<pgraph::ptr>(pgraph::ptr {
					star->attrs,
					std::make_unique<pgraph::layout>(std::move(root_layout))
				});
			}

			out_ = std::move(root_layout);

			return true;
		}

	private:
		pgraph::layout& out_;
	};

	class passgraph_pass : public ast::ast_walk<passgraph_pass> {
	public:
		passgraph_pass(const sema::trib_map& types) : types_ {types}, ribs_ {}
		{
		}

		bool visit_module_def(const ast::module_def& node)
		{
			ribs_.emplace_back(types_.at(&node));
			if (!ast::traverse_module_def(*this, node))
				return false;

			ribs_.pop_back();

			return true;
		}

		bool visit_rpc_def(const ast::rpc_def& node)
		{
			if (node.arguments) {
				for (const auto& arg : *node.arguments) {
					pgraph::layout arg_layout {};
					ast::traverse_var_decl<type_layout_pass>(arg_layout, *arg);
					std::cout << "[Passgraph] Building for argument '" << arg->name << "' of '" << node.name << "'\n";
				}
			}

			ribs_.emplace_back(types_.at(&node));
			if (!ast::traverse_rpc_def(*this, node))
				return false;

			ribs_.pop_back();

			return true;
		}

		bool visit_rpc_ptr_def(const ast::rpc_ptr_def& node)
		{
			if (node.arguments) {
				for (const auto& arg : *node.arguments) {
					pgraph::layout arg_layout {};
					ast::traverse_var_decl<type_layout_pass>(arg_layout, *arg);
					std::cout << "[Passgraph] Building for argument '" << arg->name << "' of '" << node.name << "'\n";
				}
			}

			ribs_.emplace_back(types_.at(&node));
			if (!ast::traverse_rpc_ptr_def(*this, node))
				return false;

			ribs_.pop_back();

			return true;
		}

	private:
		const sema::trib_map& types_;
		std::vector<const sema::types_rib*> ribs_ {};
	};
}

#endif
