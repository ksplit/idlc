#ifndef _LCDS_IDL_MARSHAL_TREE_BUILD_H_
#define _LCDS_IDL_MARSHAL_TREE_BUILD_H_

#include "../ast/walk.h"
#include "../sema/resolution.h"
#include "../pgraph/tree.h"

#include <gsl/gsl>

namespace idlc::marshal {
	class field_pass;

	// NOTE: this is shallow
	// FIXME: why is it shallow?
	// NOTE: shallowness allows us to assign to each layout output exactly once
	class type_layout_pass : public ast::ast_walk<type_layout_pass> {
	public:
		type_layout_pass(pgraph::layout& out) : out_ {out}
		{
		}

		bool visit_tyname_rpc(const ast::tyname_rpc& node);

		// Handles both string markers and primitives
		bool visit_tyname_stem(const ast::tyname_stem& node);
		bool visit_tyname_array(const ast::tyname_array& node);

	private:
		pgraph::layout& out_;
	};

	class field_pass : public ast::ast_walk<field_pass> {
	public:
		field_pass(pgraph::field& out) : out_ {out}
		{
		}

		bool visit_tyname(const ast::tyname& node)
		{
			// FIXME: const-ness information lost in-tree

			pgraph::layout root_layout {};
			ast::traverse_tyname(type_layout_pass {root_layout}, node);
			for (const auto& star : node.indirs) {
				std::cout << "[Passgraph] Applying indirection\n";
				root_layout = std::make_unique<pgraph::ptr>(pgraph::ptr {
					star->attrs & pgraph::tags::is_ptr,
					std::make_unique<pgraph::field>(pgraph::field {
						std::make_unique<pgraph::layout>(std::move(root_layout)),
						star->attrs & pgraph::tags::is_val
					})
				});
			}

			out_ = pgraph::field {
				std::make_unique<pgraph::layout>(std::move(root_layout)),
				node.attrs & pgraph::tags::is_val // TODO: this is a no-op in sane trees
			};

			return true;
		}

	private:
		pgraph::field& out_;
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
					pgraph::field arg_layout {};
					ast::traverse_var_decl<field_pass>(arg_layout, *arg);
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
					pgraph::field arg_layout {};
					ast::traverse_var_decl<field_pass>(arg_layout, *arg);
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

	// Build a table of projections with their type scope-chains
	// Type layout pass will first search for their projection by name within their local chain of scopes
	// Then use this table in a nested pass to discover the layout of the projection
	class ptable_pass : public ast::ast_walk<ptable_pass> {

	};
}

#endif
