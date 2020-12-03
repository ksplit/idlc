#include "build_pgraph.h"

#include <iostream>
#include <memory>
#include <type_traits>
#include <variant>

#include "../ast/ast.h"
#include "../ast/walk.h"

// TODO: rewrite me to use find_type()
// TODO: move me please
namespace idlc::marshal {
	pgraph::field build_field(
		const ast::tyname& node,
		const ptable& idl_ptable,
		const std::vector<const sema::types_rib*>& schain
	)
	{
		pgraph::field elem {};
		field_pass lpass {idl_ptable, schain, elem};
		lpass.visit_tyname(node);
		return elem;
	}
}

bool idlc::marshal::type_layout_pass::visit_tyname_array(const ast::tyname_array& node)
{
	const auto visit = [this, &node](auto&& inner)
	{
		using type = std::decay_t<decltype(inner)>;
		if constexpr (std::is_same_v<type, unsigned>) {
			std::cout << "[Passgraph] Adding const-sized array layout\n";
			out_ = std::make_unique<pgraph::array_layout>(pgraph::array_layout {
				inner,
				std::make_unique<pgraph::field>(build_field(*node.element, ptable_, schain_))
			});
		}
		else if constexpr (std::is_same_v<type, ast::tok_kw_null>) {
			std::cout << "[Passgraph] Adding null-terminated array layout\n";
			out_ = std::make_unique<pgraph::null_array_layout>(pgraph::null_array_layout {
				std::make_unique<pgraph::field>(build_field(*node.element, ptable_, schain_))
			});
		}
		else if constexpr (std::is_same_v<type, gsl::czstring<>>) {
			std::cout << "[Passgraph] Adding variable-size array layout\n";
			out_ = std::make_unique<pgraph::dyn_array_layout>(pgraph::dyn_array_layout {
				inner,
				std::make_unique<pgraph::field>(build_field(*node.element, ptable_, schain_))
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

// TODO: move me
namespace idlc::marshal {
	// TODO: must this be a walk?
	class proj_layout_pass : public ast::ast_walk<proj_layout_pass> {
	public:
		proj_layout_pass(const ptable& ptable) :
			ptable_ {ptable}
		{
		}

		bool visit_union_proj_def(const ast::union_proj_def& node)
		{
			return true;
		}

	private:
		const ptable& ptable_;
	};
}

bool idlc::marshal::type_layout_pass::visit_tyname_proj(const ast::tyname_proj& node)
{
	// Logically, we wish to locate the proj definition by name in our given scope chain
	// (note that since this pass is assumed to be used on only one type at a time, we need only concern ourselves with
	// a single scope chain). Given this, we look up its scope chain and start building its layout information
	// (not field), before memoizing it

	const auto def = find_type(schain_, node.name);
	const auto str = std::get_if<const ast::struct_proj_def*>(&def);
	if (str) {
		const auto& node = **str; // FIXME: this may not need to be a double indirection
		std::cout << "[Res] Located struct projection " << node.name << "\n";
		proj_layout_pass pass {ptable_};
		pass.visit_struct_proj_def(node);
		return true;
	}

	const auto uni = std::get_if<const ast::union_proj_def*>(&def);
	if (uni) {
		const auto& node = **uni;
		std::cout << "[Res] Located union projection " << node.name << "\n";
		proj_layout_pass pass {ptable_};
		pass.visit_union_proj_def(node);
		return true;
	}

	std::cout << "[Res] Did not resolve " << node.name << "\n";

	return true;
}
