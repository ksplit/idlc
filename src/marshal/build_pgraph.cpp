#include "build_pgraph.h"

#include <iostream>
#include <memory>
#include <type_traits>

#include "../ast/ast.h"
#include "../ast/walk.h"

bool idlc::marshal::type_layout_pass::visit_tyname_array(const ast::tyname_array& node)
{
	pgraph::field elem {};
	field_pass lpass {elem};
	lpass.visit_tyname(*node.element);
	const auto visit = [this, &elem](auto&& inner)
	{
		using type = std::decay_t<decltype(inner)>;
		if constexpr (std::is_same_v<type, unsigned>) {
			std::cout << "[Passgraph] Adding const-sized array layout\n";
			out_ = std::make_unique<pgraph::array_layout>(pgraph::array_layout {
				inner,
				std::make_unique<pgraph::field>(std::move(elem))
			});
		}
		else if constexpr (std::is_same_v<type, ast::tok_kw_null>) {
			std::cout << "[Passgraph] Adding null-terminated array layout\n";
			out_ = std::make_unique<pgraph::null_array_layout>(pgraph::null_array_layout {
				std::make_unique<pgraph::field>(std::move(elem))
			});
		}
		else if constexpr (std::is_same_v<type, gsl::czstring<>>) {
			std::cout << "[Passgraph] Adding variable-size array layout\n";
			out_ = std::make_unique<pgraph::dyn_array_layout>(pgraph::dyn_array_layout {
				inner,
				std::make_unique<pgraph::field>(std::move(elem))
			});
		}
	};

	std::visit(visit, *node.size);

	return true;
}

// Handles both string markers and primitives
bool idlc::marshal::type_layout_pass::visit_tyname_stem(const ast::tyname_stem& node)
{
	const auto visit = [this, &node](auto&& inner)
	{
		using type = std::decay_t<decltype(inner)>;
		if constexpr (std::is_same_v<type, ast::tyname_string>) {
			std::cout << "[Passgraph] Adding layout for string value (NOT A POINTER)\n";
			// NOTE: strings are just null-terminated arrays of char
			// passgraph makes no distinction
			out_ = std::make_unique<pgraph::null_array_layout>(pgraph::null_array_layout {
				std::make_unique<pgraph::field>(pgraph::field {
					std::make_unique<pgraph::layout>(pgraph::prim::ty_char)
				})
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

bool idlc::marshal::type_layout_pass::visit_tyname_rpc(const ast::tyname_rpc& node)
{
	std::cout << "[Passgraph] Adding layout for rpc pointer\n";
	auto rpc_layout = std::make_unique<pgraph::rpc_ptr_layout>(pgraph::rpc_ptr_layout {node.name});
	out_ = pgraph::layout {std::move(rpc_layout)};
	return true;
}

bool idlc::marshal::type_layout_pass::visit_tyname_proj(const ast::tyname_proj& node)
{
	// Logically, we wish to locate the proj definition by name in our given scope chain
	// (note that since this pass is assumed to be used on only one type at a time, we need only concern ourselves with
	// a single scope chain). Given this, we look up its scope chain and start building its layout information
	// (not field), before memoizing it

	// TODO: mapping of projection definitions to scope chains
	return true;
}
