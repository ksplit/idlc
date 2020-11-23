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
			if (!self().visit_file(node))
				return false;

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

		bool visit_file(const file& node)
		{
			std::cout << "file" << std::endl;
			return true;
		}

		bool traverse_module_def(const module_def& node)
		{
			if (!visit_module_def(node))
				return false;

			if (node.items) {
				for (const auto& item : *node.items) {
					if (!self().traverse_module_item(*item))
						return false;
				}
			}

			return true;
		}

		bool visit_module_def(const module_def& node)
		{
			std::cout << "module_def" << std::endl;
			return true;
		}

		bool traverse_driver_file(const driver_file& node)
		{
			if (!visit_driver_file(node))
				return false;

			return self().traverse_driver_def(*node.driver);
		}

		bool visit_driver_file(const driver_file& node)
		{
			std::cout << "driver_file" << std::endl;
			return true;
		}

		bool traverse_driver_def(const driver_def& node)
		{
			if (!visit_driver_def(node))
				return false;

			return true;
		}

		bool visit_driver_def(const driver_def& node)
		{
			std::cout << "driver_def" << std::endl;
			return true;
		}

		bool traverse_module_item(const module_item& node)
		{
			if (!visit_module_item(node))
				return false;

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

		bool visit_module_item(const module_item& node)
		{
			std::cout << "module_item" << std::endl;
			return true;
		}

		bool traverse_union_proj_def(const union_proj_def& node)
		{
			if (!visit_union_proj_def(node))
				return false;

			if (node.fields) {
				for (const auto& item : *node.fields) {
					if (!self().traverse_proj_field(*item))
						return false;
				}
			}

			return true;
		}

		bool visit_union_proj_def(const union_proj_def& node)
		{
			std::cout << "union_proj_def" << std::endl;
			return true;
		}

		// FIXME: code duplication between struct / union projection defs

		bool traverse_struct_proj_def(const struct_proj_def& node)
		{
			if (!visit_struct_proj_def(node))
				return false;

			if (node.fields) {
				for (const auto& item : *node.fields) {
					if (!self().traverse_proj_field(*item))
						return false;
				}
			}

			return true;
		}

		bool visit_struct_proj_def(const struct_proj_def& node)
		{
			std::cout << "struct_proj_def" << std::endl;
			return true;
		}

		bool traverse_rpc_def(const rpc_def& node)
		{
			if (!visit_rpc_def(node))
				return false;

			if (node.ret_type) {
				if (!self().traverse_tyname(*node.ret_type))
					return false;
			}

			if (node.arguments) {
				for (const auto& item : *node.arguments) {
					if (!self().traverse_var_decl(*item))
						return false;
				}
			}

			if (node.items) {
				for (const auto& item : *node.items) {
					if (!self().traverse_rpc_item(*item))
						return false;
				}
			}

			return true;
		}

		bool visit_rpc_def(const rpc_def& node)
		{
			std::cout << "rpc_def" << std::endl;
			return true;
		}

		// FIXME: code duplication between rpc_ptr and rpc
		bool traverse_rpc_ptr_def(const rpc_ptr_def& node)
		{
			if (!visit_rpc_ptr_def(node))
				return false;

			if (node.ret_type) {
				if (!self().traverse_tyname(*node.ret_type))
					return false;
			}

			if (node.arguments) {
				for (const auto& item : *node.arguments) {
					if (!self().traverse_var_decl(*item))
						return false;
				}
			}

			if (node.items) {
				for (const auto& item : *node.items) {
					if (!self().traverse_rpc_item(*item))
						return false;
				}
			}

			return true;
		}

		bool visit_rpc_ptr_def(const rpc_ptr_def& node)
		{
			std::cout << "rpc_ptr_def" << std::endl;
			return true;
		}

		bool traverse_proj_field(const proj_field& node)
		{
			if (!visit_proj_field(node))
				return false;

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

		bool visit_proj_field(const proj_field& node)
		{
			std::cout << "proj_field" << std::endl;
			return true;
		}

		bool traverse_var_decl(const var_decl& node)
		{
			if (!visit_var_decl(node))
				return false;

			return self().traverse_tyname(*node.type);
		}

		bool visit_var_decl(const var_decl& node)
		{
			std::cout << "var_decl" << std::endl;
			return true;
		}

		bool traverse_tyname(const tyname& node)
		{
			if (!visit_tyname(node))
				return false;

			if (!self().traverse_tyname_stem(*node.stem))
				return false;
			
			for (const auto& item : node.indirs) {
				if (!self().traverse_indirection(*item))
					return false;
			}

			return true;
		}

		bool visit_tyname(const tyname& node)
		{
			std::cout << "tyname" << std::endl;
			return true;
		}

		bool traverse_indirection(const indirection& node)
		{
			if (!visit_indirection(node))
				return false;

			return true;
		}

		bool visit_indirection(const indirection& node)
		{
			std::cout << "indirection" << std::endl;
			return true;
		}

		bool traverse_tyname_stem(const tyname_stem& node)
		{
			if (!visit_tyname_stem(node))
				return false;

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
				else if constexpr (std::is_same_v<type, tyname_string>)
					return true;
				else if constexpr (std::is_same_v<type, tyname_arith>)
					return true;
				else
					assert(false);
			};

			return std::visit(visit, node);
		}

		bool visit_tyname_stem(const tyname_stem& node)
		{
			std::cout << "tyname_stem" << std::endl;
			return true;
		}

		bool traverse_tyname_any_of(const tyname_any_of_ptr& node)
		{
			if (!visit_tyname_any_of(node))
				return false;

			for (const auto& item : *node.types) {
				if (!self().traverse_tyname(*item))
					return false;
			}
			
			return true;
		}

		bool visit_tyname_any_of(const tyname_any_of_ptr& node)
		{
			std::cout << "tyname_any_of" << std::endl;
			return true;
		}

		bool traverse_tyname_array(const tyname_array& node)
		{
			if (!visit_tyname_array(node))
				return false;

			if (!self().traverse_tyname(*node.element))
				return false;

			return self().traverse_array_size(*node.size);
		}

		bool visit_tyname_array(const tyname_array& node)
		{
			std::cout << "tyname_array" << std::endl;
			return true;
		}

		bool traverse_array_size(const array_size& node)
		{
			if (!visit_array_size(node))
				return false;

			// this is a variant but it has no node_refs in it
			return true;
		}

		bool visit_array_size(const array_size& node)
		{
			std::cout << "array_size" << std::endl;
			return true;
		}

		bool traverse_tyname_proj(const tyname_proj& node)
		{
			if (!visit_tyname_proj(node))
				return false;

			return true;
		}

		bool visit_tyname_proj(const tyname_proj& node)
		{
			std::cout << "tyname_proj" << std::endl;
			return true;
		}

		bool traverse_tyname_rpc(const tyname_rpc& node)
		{
			if (!visit_tyname_rpc(node))
				return false;

			return true;
		}

		bool visit_tyname_rpc(const tyname_rpc& node)
		{
			std::cout << "tyname_rpc" << std::endl;
			return true;
		}

		bool traverse_naked_proj_decl(const naked_proj_decl& node)
		{
			if (!visit_naked_proj_decl(node))
				return false;

			if (node.fields) {
				for (const auto& item : *node.fields) {
					if (!self().traverse_proj_field(*item))
						return false;
				}
			}

			return true;
		}

		bool visit_naked_proj_decl(const naked_proj_decl& node)
		{
			std::cout << "naked_proj_decl" << std::endl;
			return true;
		}

		bool traverse_rpc_item(const rpc_item& node)
		{
			if (!visit_rpc_item(node))
				return false;

			const auto visit = [this](auto&& subnode) -> bool {
				using type = std::decay_t<decltype(subnode)>;
				if constexpr (std::is_same_v<type, node_ref<union_proj_def>>)
					return self().traverse_union_proj_def(*subnode);
				else if constexpr (std::is_same_v<type, node_ref<struct_proj_def>>)
					return self().traverse_struct_proj_def(*subnode);
				else if constexpr (std::is_same_v<type, node_ref<rpc_ptr_def>>)
					return self().traverse_rpc_ptr_def(*subnode);
				else
					assert(false);
			};

			return std::visit(visit, node);
		}

		bool visit_rpc_item(const rpc_item& node)
		{
			std::cout << "rpc_item" << std::endl;
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
