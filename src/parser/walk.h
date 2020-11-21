#ifndef _LCDS_IDL_PARSER_WALK_H_
#define _LCDS_IDL_PARSER_WALK_H_

#include <type_traits>

#include "ast.h"

namespace idlc::parser {	
	template<typename derived>
	class ast_walk {
	public:
		bool traverse_file(const file& node)
		{
			const auto visit = [this](auto&& subnode) -> bool
			{
				using type = std::decay_t<decltype(subnode)>;
				if constexpr (std::is_same_v<type, node_ref<std::vector<node_ref<module_def>>>>) {
					for (const auto& def : *subnode) {
						if (!self().traverse_module_def(*def))
							return false;
					}

					return true;
				}
				else if constexpr (std::is_same_v<type, node_ref<driver_file>>) {
					return self().traverse_driver_file(*subnode);
				}
				else {
					assert(false);
				}
			};			

			return std::visit(visit, node);
		}

		bool traverse_module_def(const module_def& node)
		{
			if (node.items) {
				for (const auto& item : *node.items) {
					if (!self().traverse_module_item(*item))
						return false;
				}
			}

			return true;
		}

		bool traverse_driver_file(const driver_file& node)
		{
			return self().traverse_driver_def(*node.driver);
		}

		bool traverse_driver_def(const driver_def& node)
		{
			return true;
		}

		bool traverse_module_item(const module_item& node)
		{
			const auto visit = [this](auto&& subnode) -> bool
			{
				using type = std::decay_t<decltype(subnode)>;
				if constexpr (std::is_same_v<type, header_stmt>)
					return true; // TODO
				else if constexpr (std::is_same_v<type, node_ref<union_proj_def>>)
					return self().traverse_union_proj_def(*subnode);
				else if constexpr (std::is_same_v<type, node_ref<struct_proj_def>>)
					return self().traverse_struct_proj_def(*subnode);
				else if constexpr (std::is_same_v<type, node_ref<rpc_def>>)
					return self().traverse_rpc_def(*subnode);
				else if constexpr (std::is_same_v<type, node_ref<rpc_ptr_def>>)
					return self().traverse_rpc_ptr_def(*subnode);
				else
					assert(false);
			};

			return std::visit(visit, node);
		}

		bool traverse_union_proj_def(const union_proj_def& node)
		{
			if (node.fields) {
				for (const auto& item : *node.fields) {
					if (!self().traverse_proj_field(*item))
						return false;
				}
			}

			return true;
		}

		// FIXME: code duplication between struct / union projection defs

		bool traverse_struct_proj_def(const struct_proj_def& node)
		{
			if (node.fields) {
				for (const auto& item : *node.fields) {
					if (!self().traverse_proj_field(*item))
						return false;
				}
			}

			return true;
		}

		bool traverse_rpc_def(const rpc_def& node)
		{
			return true;
		}

		bool traverse_rpc_ptr_def(const rpc_ptr_def& node)
		{
			return true;
		}

		bool traverse_proj_field(const proj_field& node)
		{
			const auto visit = [this](auto&& subnode) -> bool
			{
				using type = std::decay_t<decltype(subnode)>;
				if constexpr (std::is_same_v<type, node_ref<var_decl>>)
					return self().traverse_var_decl(*subnode);
				else if constexpr (std::is_same_v<type, node_ref<naked_proj_decl>>)
					return self().traverse_naked_proj_decl(*subnode);
				else
					assert(false);
			};

			return std::visit(visit, node);
		}

		bool traverse_var_decl(const var_decl& node)
		{
			return self().traverse_tyname(*node.type);
		}

		bool traverse_tyname(const tyname& node)
		{
			if (!self().traverse_tyname_stem(*node.stem))
				return false;
			
			for (const auto& item : node.indirs) {
				if (!self().traverse_indirection(*item))
					return false;
			}

			return true;
		}

		bool traverse_indirection(const indirection& node)
		{
			return true;
		}

		bool traverse_tyname_stem(const tyname_stem& node)
		{
			const auto visit = [this](auto&& subnode) -> bool
			{
				using type = std::decay_t<decltype(subnode)>;
				if constexpr (std::is_same_v<type, node_ref<tyname_rpc>>)
					return self().traverse_tyname_rpc(*subnode);
				else if constexpr (std::is_same_v<type, node_ref<tyname_proj>>)
					return self().traverse_tyname_proj(*subnode);
				else if constexpr (std::is_same_v<type, node_ref<tyname_array>>)
					return self().traverse_tyname_array(*subnode);
				else if constexpr (std::is_same_v<type, node_ref<tyname_any_of_ptr>>)
					return self().traverse_tyname_any_of(*subnode);
				else
					assert(false);
			};

			return std::visit(visit, node);
		}

		bool traverse_tyname_any_of(const tyname_any_of_ptr& node)
		{
			// TODO
			return true;
		}

		bool traverse_tyname_array(const tyname_array& node)
		{
			// TODO
			return true;
		}

		bool traverse_tyname_proj(const tyname_proj& node)
		{
			// TODO
			return true;
		}

		bool traverse_tyname_rpc(const tyname_rpc& node)
		{
			// TODO
			return true;
		}

		bool traverse_naked_proj_decl(const naked_proj_decl& node)
		{
			return true;
		}

	private:
		auto self()
		{
			return *reinterpret_cast<derived*>(this);
		}
	};

	class null_walk : public ast_walk<null_walk> {

	};
}

#endif
