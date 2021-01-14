#include "pgraph_generation.h"

#include <cassert>
#include <exception>
#include <memory>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "../parser/string_heap.h"
#include "../ast/ast.h"
#include "../ast/walk.h"
#include "../tag_types.h"
#include "pgraph.h"

namespace idlc::sema {
	namespace {
		// Remember: field_type is quite slim, only a tag value and a pointer (16 bytes total)

		field_type build_string_type(bool is_const);
		field_type build_stem_type(const ast::type_stem& node, bool is_const);
		field_type build_array_type(const ast::type_array& node, bool is_const);
		field_type build_projection(ast::type_proj& node);
		node_ptr<data_field> build_data_field(const ast::type_spec& node);

		field_type build_string_type(bool is_const)
		{
			return std::make_unique<null_terminated_array>(
				std::make_unique<data_field>(primitive::ty_char, ast::annotation::use_default),
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
					return item->definition;
				}
				else if constexpr (std::is_same_v<type, ast::node_ref<ast::type_rpc>>) {
					// std::cout << "[debug] generating dummy pgraph node for RPC pointer \"" << item->name << "\"\n";
					return std::make_unique<rpc_ptr>();
				}

				std::terminate();
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
					std::terminate();
				}
				else if constexpr (std::is_same_v<type, ident>) {
					return std::make_unique<dyn_array>(
						build_data_field(*node.element),
						item,
						is_const
					);
				}

				std::terminate();
			};

			return {};
		}

		auto build_field(ast::proj_field& node)
		{
			const auto visit = [](auto&& item) -> std::pair<ident, node_ptr<data_field>>
			{
				using type = std::decay_t<decltype(item)>;
				if constexpr (std::is_same_v<type, ast::node_ref<ast::naked_proj_decl>>) {
					std::cout << "[debug] Naked projections are not yet implemented\n";
					std::cout << "[debug] Unknown how to proceed, aborting\n";
					std::terminate();
				}
				else if constexpr (std::is_same_v<type, ast::node_ref<ast::var_decl>>) {
					// std::cout << "[pgraph] building var_decl data_field for \"" << item->name << "\"\n";
					return {item->name, build_data_field(*item->type)};
				}

				std::terminate();
			};

			return std::visit(visit, node);
		}

		auto build_union_dummy(ast::proj_def& def)
		{
			std::cout << "[debug] Union projections are not yet implemented\n";
			std::cout << "[debug] Will implement as an empty struct projection \"" << def.name << "\"\n";
			return std::make_shared<projection>(def.type);
		}

		auto build_empty_struct(ast::proj_def& def)
		{
			// std::cout << "[pgraph] generating empty pgraph projection for \"" << def.name << "\"\n";
			return std::make_shared<projection>(def.type);
		}

		auto build_struct(ast::proj_def& def)
		{
			const auto& field_nodes = *def.fields;
			decltype(projection::fields) fields {};
			fields.reserve(field_nodes.size());
			for (const auto& field : field_nodes) {
				auto pgraph = build_field(*field);
				// std::cout << "[pgraph] completed field \"" << pgraph.first << "\"\n";
				fields.emplace_back(std::move(pgraph));
			}

			// std::cout << "[pgraph] finished \"" << def.name << "\"\n";

			return std::make_shared<projection>(def.type, std::move(fields));
		}

		class type_walk : public ast::ast_walk<type_walk> {
		public:
			bool visit_rpc_def(ast::rpc_def& node)
			{
				if (node.ret_type)
					insert_field(node, *node.ret_type, "<ret-type>");

				if (node.arguments) {
					for (auto& argument : *node.arguments)
						insert_field(node, *argument->type, argument->name);
				}

				return ast::traverse(*this, node);
			}

			// TODO: support naked projections

			// FIXME: this is probably a hack, thanks to the unclean split between the AST and the assoicated data
			// structures
			auto get_pgraph_owner()
			{
				return std::move(store_);
			}

		private:
			// NOTE: idents are only for debugging
			std::vector<std::pair<gsl::czstring<>, node_ptr<data_field>>> store_ {};

			// NOTE: the name is not necessarily an ident!!!
			void insert_field(ast::rpc_def& parent, const ast::type_spec& node, gsl::czstring<> name)
			{
				auto pgraph_node = build_data_field(node);
				parent.pgraphs.emplace_back(pgraph_node.get());
				store_.emplace_back(name, std::move(pgraph_node));
			}
		};

		node_ptr<data_field> build_data_field(const ast::type_spec& node)
		{
			// TODO: finish indirection handling
			auto type = build_stem_type(*node.stem, node.is_const);
			for (const auto& ptr_node : node.indirs) {
				const auto annots = ptr_node->attrs;
				const auto ptr_annots = annots & ast::annotation::is_ptr;
				const auto val_annots = annots & ast::annotation::is_val;
				auto field = std::make_unique<data_field>(std::move(type), val_annots);
				type = std::make_unique<pointer>(std::move(field), ptr_annots, ptr_node->is_const);
			}

			assert((node.attrs & ast::annotation::is_val) == node.attrs);
			auto field = std::make_unique<data_field>(std::move(type), node.attrs);

			return std::move(field);
		}
	}
}

using namespace idlc;
using namespace idlc::sema;

// The references in the AST are *not* owners, this vector is
std::vector<std::pair<ident, node_ptr<data_field>>> idlc::sema::generate_pgraphs(
	gsl::span<const gsl::not_null<ast::rpc_def*>> rpcs)
{
	type_walk walk {};
	for (const auto& rpc : rpcs) {
		const auto succeeded = walk.visit_rpc_def(*rpc);
		assert(succeeded);
	}

	return walk.get_pgraph_owner();
}

// TODO: is it necessary to detect if a projection self-references by value?
std::shared_ptr<idlc::sema::projection> idlc::sema::build_projection(ast::proj_def& node)
{
	if (node.seen_before) {
		std::cout << "[debug] Self-referential projections are not yet supported\n";
		std::terminate();
	}
	
	// std::cout << "[pgraph] generating new pgraph for \"" << def.name << "\" (" << &def << ")\n";
	node.seen_before = true;
	auto pgraph_node = [&node] {
		if (!node.fields)
			return build_empty_struct(node);
		else if (node.kind == ast::proj_def_kind::union_kind)
			return build_union_dummy(node);
		else
			return build_struct(node);
	}();

	node.seen_before = false;

	return std::move(pgraph_node);
}
