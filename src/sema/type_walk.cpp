#include "type_walk.h"

#include <memory>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "../ast/ast.h"
#include "../ast/walk.h"

namespace idlc::sema {
	namespace {
		field_type build_string_type(bool is_const);
		field_type build_stem_type(const ast::type_stem& node, bool is_const);
		field_type build_array_type(const ast::type_array& node, bool is_const);
		field_type build_projection(ast::type_proj& node);

		field_type build_string_type(bool is_const)
		{
			return std::make_unique<null_terminated_array>(
				std::make_unique<data_field>(
					primitive::ty_char,
					ast::annotation::use_default
				),
				is_const
			);
		}

		field_type build_stem_type(const ast::type_stem& node, bool is_const)
		{
			const auto visit = [is_const](auto&& item) -> field_type
			{
				using type = std::decay_t<decltype(item)>;
				if constexpr (std::is_same_v<type, ast::type_primitive>) {
					return item;
				}
				else if constexpr (std::is_same_v<type, ast::type_string>) {
					return build_string_type(is_const);
				}
				else if constexpr (std::is_same_v<type, ast::node_ref<ast::type_array>>) {
					return build_array_type(*item, is_const);
				}
				else if constexpr (std::is_same_v<type, ast::node_ref<ast::type_proj>>) {
					std::cout << "[debug] Stopped at projection type\n";
					return build_projection(*item);
				}
				else if constexpr (std::is_same_v<type, ast::node_ref<ast::type_rpc>>) {
					std::cout << "[debug] Stopped at RPC type\n";
				}
			
				assert(false);
			};

			return std::visit(visit, node);
		}

		field_type build_array_type(const ast::type_array& node, bool is_const)
		{
			const auto visit = [&node, is_const](auto&& item) -> field_type
			{
				using type = std::decay_t<decltype(item)>;
				if constexpr (std::is_same_v<type, ast::tok_kw_null>) {
					return std::make_unique<null_terminated_array>(
						build_data_field(*node.element),
						is_const
					);
				}
				else if constexpr (std::is_same_v<type, unsigned>) {
					std::cout << "[debug] static-sized arrays are not yet implemented\n";
					assert(false);
				}
				else if constexpr (std::is_same_v<type, ident>) {
					return std::make_unique<dyn_array>(
						build_data_field(*node.element),
						item,
						is_const
					);
				}

				assert(false);
			};

			return {};
		}

		field_type build_projection(ast::type_proj& node)
		{
			// NOTE: cannot use unique_ptr if we allow node reuse. Simplest solution is shared_ptr, but I dislike it
			assert(false);
		}
	}
}

idlc::sema::node_ptr<idlc::sema::data_field> idlc::sema::build_data_field(const ast::type_spec& node)
{
	auto type = build_stem_type(*node.stem, node.is_const);
	return std::make_unique<data_field>(std::move(type), ast::annotation::use_default);
}
