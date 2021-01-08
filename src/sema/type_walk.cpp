#include "type_walk.h"

#include <memory>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "../ast/ast.h"
#include "../ast/walk.h"
#include "../tag_types.h"
#include "pgraph.h"

namespace idlc::sema {
	namespace {
		field_type build_string_type(bool is_const);
		field_type build_stem_type(const ast::type_stem& node, bool is_const);
		field_type build_array_type(const ast::type_array& node, bool is_const);
		field_type build_projection(ast::type_proj& node);
		node_ptr<data_field> build_data_field(ast::type_spec& node);

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
					return build_projection(*item);
				}
				else if constexpr (std::is_same_v<type, ast::node_ref<ast::type_rpc>>) {
					std::cout << "[debug] generating dummy pgraph node for RPC pointer \"" << item->name << "\"\n";
					return std::make_unique<rpc_ptr>();
				}
			
				std::cout.flush();
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

		auto build_field(ast::proj_field& node)
		{
			const auto visit = [](auto&& item) -> std::pair<ident, node_ptr<data_field>>
			{
				using type = std::decay_t<decltype(item)>;
				if constexpr (std::is_same_v<type, ast::node_ref<ast::naked_proj_decl>>) {
					std::cout << "[debug] Naked projections are not yet implemented\n";
					std::cout << "[debug] Unknown how to proceed, aborting\n";
					assert(false);
				}
				else if constexpr (std::is_same_v<type, ast::node_ref<ast::var_decl>>) {
					std::cout << "[pgraph] building var_decl data_field for \"" << item->name << "\"\n";
					return {item->name, build_data_field(*item->type)};
				}

				assert(false);
			};

			return std::visit(visit, node);
		}

		field_type build_projection(ast::type_proj& node)
		{
			auto& def = *node.definition;
			if (def.pgraph) {
				std::cout << "[pgraph] re-using cached pgraph for \"" << def.name << "\"\n";
				return projection_ptr {def.pgraph};
			}
			else {
				std::cout << "[pgraph] generating new pgraph for \""
					<< def.name << "\" (" << &def << ")\n";

				if (!def.fields) {
					std::cout << "[pgraph] generating empty pgraph projection for \"" << def.name << "\"\n";
					auto pgraph = make_projection({});
					def.pgraph = pgraph.get();
					return std::move(pgraph);
				}

				if (def.kind == ast::proj_def_kind::union_kind) {
					std::cout << "[debug] Union projections are not yet implemented\n";
					std::cout << "[debug] Will implement as an empty struct projection \"" << def.name << "\"\n";
					auto pgraph = make_projection({});
					def.pgraph = pgraph.get();
					return std::move(pgraph);
				}

				const auto& field_nodes = *def.fields;
				std::vector<std::pair<ident, node_ptr<data_field>>> fields(field_nodes.size());
				for (const auto& field : field_nodes) {
					auto pgraph = build_field(*field);
					std::cout << "[pgraph] completed field \"" << pgraph.first << "\"\n";
					fields.emplace_back(std::move(pgraph));
				}

				std::cout << "[pgraph] finished, caching new pgraph for \"" << def.name << "\"\n";
				auto pgraph = make_projection(std::move(fields));
				def.pgraph = pgraph.get();

				return std::move(pgraph);
			}
		}

		class type_walk : public ast::ast_walk<type_walk> {
		public:
			bool visit_type_spec(ast::type_spec& node)
			{
				// the pgraph weak references assume that each of these live at least as long as the AST projection nodes
				store_.emplace_back(build_data_field(node));
				return true;
			}

			auto get_pgraph_owner()
			{
				return std::move(store_);
			}

		private:
			field_type stem_ {};
			std::vector<node_ptr<data_field>> store_ {};
		};

		node_ptr<data_field> build_data_field(ast::type_spec& node)
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
			node.pgraph = field.get();

			return std::move(field);
		}
	}
}

using namespace idlc::sema;

// The references in the AST are *not* owners, this vector is
std::vector<node_ptr<data_field>> idlc::sema::generate_pgraphs(idlc::ast::file& file)
{
	type_walk walk {};
	walk.visit_file(file);
	return walk.get_pgraph_owner();
}
