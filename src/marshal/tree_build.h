#ifndef _LCDS_IDL_MARSHAL_TREE_BUILD_H_
#define _LCDS_IDL_MARSHAL_TREE_BUILD_H_

#include "../parser/walk.h"
#include "tree.h"

namespace idlc::marshal {
	class layout_pass : public parser::ast_walk<layout_pass> {
	public:
		layout_pass(layout& out) : out_ {out}
		{
		}

		bool visit_tyname_rpc(const parser::tyname_rpc& node)
		{
			std::cout << "[Passgraph] Adding layout for rpc pointer\n";
			out_ = std::make_unique<rpc_ptr_layout>(rpc_ptr_layout {node.name});
			return true;
		}

		bool visit_tyname_array(const parser::tyname_array& node)
		{
			const auto visit = [](auto&& inner)
			{
				using type = std::decay_t<decltype(inner)>;
				if constexpr (std::is_same_v<type, unsigned>) {
					std::cout << "[Passgraph] Adding const-sized array layout\n";
				}
				else if constexpr (std::is_same_v<type, parser::tok_kw_null>) {
					std::cout << "[Passgraph] Adding null-terminated array layout\n";
				}
				else if constexpr (std::is_same_v<type, gsl::czstring<>>) {
					std::cout << "[Passgraph] Adding variable-size array layout\n";
				}
			};

			std::visit(visit, *node.size);

			return true;
		}

	private:
		layout& out_;
	};

	class passgraph_builder : public parser::ast_walk<passgraph_builder> {
	public:
		bool visit_tyname(const parser::tyname& node)
		{
			layout l {};
			layout_pass base_layout_pass {l};
			parser::traverse_tyname(base_layout_pass, node);
			const auto layout_node = std::make_unique<layout>(std::move(l));
			return true;
		}

	private:
	};
}

#endif
