#ifndef _LCDS_IDL_PARSER_WALK_H_
#define _LCDS_IDL_PARSER_WALK_H_

#include <type_traits>
#include <cassert>
#include <iostream>

#include "ast.h"

namespace idlc::ast {
	template<typename walk>
	bool traverse_file(walk&& self, const file& node)
	{
		const auto visit = [&self](auto&& subnode) -> bool
		{
			using type = std::decay_t<decltype(subnode)>;
			if constexpr (std::is_same_v<type, node_ref<std::vector<node_ref<module_def>>>>) {
				for (const auto& def : *subnode) {
					if (!self.visit_module_def(*def))
						return false;
				}

				return true;
			}
			else if constexpr (std::is_same_v<type, node_ref<driver_file>>) {
				return self.visit_driver_file(*subnode);
			}
			else {
				assert(false);
			}
		};			

		return std::visit(visit, node);
	}

	template<typename walk>
	bool traverse_module_def(walk&& self, const module_def& node)
	{
		if (node.items) {
			for (const auto& item : *node.items) {
				if (!self.visit_module_item(*item))
					return false;
			}
		}

		return true;
	}

	template<typename walk>
	bool traverse_driver_file(walk&& self, const driver_file& node)
	{
		return self.visit_driver_def(*node.driver);
	}

	template<typename walk>
	bool traverse_driver_def(walk&& self, const driver_def& node)
	{
		return true;
	}

	template<typename walk>
	bool traverse_module_item(walk&& self, const module_item& node)
	{
		const auto visit = [&self](auto&& subnode) -> bool
		{
			using type = std::decay_t<decltype(subnode)>;
			if constexpr (std::is_same_v<type, header_stmt>)
				return true; // TODO
			else if constexpr (std::is_same_v<type, node_ref<union_proj_def>>)
				return self.visit_union_proj_def(*subnode);
			else if constexpr (std::is_same_v<type, node_ref<struct_proj_def>>)
				return self.visit_struct_proj_def(*subnode);
			else if constexpr (std::is_same_v<type, node_ref<rpc_def>>)
				return self.visit_rpc_def(*subnode);
			else if constexpr (std::is_same_v<type, node_ref<rpc_ptr_def>>)
				return self.visit_rpc_ptr_def(*subnode);
			else
				assert(false);
		};

		return std::visit(visit, node);
	}

	template<typename walk>
	bool traverse_union_proj_def(walk&& self, const union_proj_def& node)
	{
		if (node.fields) {
			for (const auto& item : *node.fields) {
				if (!self.visit_proj_field(*item))
					return false;
			}
		}

		return true;
	}

	// FIXME: code duplication between struct / union projection defs

	template<typename walk>
	bool traverse_struct_proj_def(walk&& self, const struct_proj_def& node)
	{
		if (node.fields) {
			for (const auto& item : *node.fields) {
				if (!self.visit_proj_field(*item))
					return false;
			}
		}

		return true;
	}

	template<typename walk>
	bool traverse_rpc_def(walk&& self, const rpc_def& node)
	{
		if (node.ret_type) {
			if (!self.visit_tyname(*node.ret_type))
				return false;
		}

		if (node.arguments) {
			for (const auto& item : *node.arguments) {
				if (!self.visit_var_decl(*item))
					return false;
			}
		}

		if (node.items) {
			for (const auto& item : *node.items) {
				if (!self.visit_rpc_item(*item))
					return false;
			}
		}

		return true;
	}

	// FIXME: code duplication between rpc_ptr and rpc
	template<typename walk>
	bool traverse_rpc_ptr_def(walk&& self, const rpc_ptr_def& node)
	{
		if (node.ret_type) {
			if (!self.visit_tyname(*node.ret_type))
				return false;
		}

		if (node.arguments) {
			for (const auto& item : *node.arguments) {
				if (!self.visit_var_decl(*item))
					return false;
			}
		}

		if (node.items) {
			for (const auto& item : *node.items) {
				if (!self.visit_rpc_item(*item))
					return false;
			}
		}

		return true;
	}

	template<typename walk>
	bool traverse_proj_field(walk&& self, const proj_field& node)
	{
		const auto visit = [&self](auto&& subnode) -> bool
		{
			using type = std::decay_t<decltype(subnode)>;
			if constexpr (std::is_same_v<type, node_ref<var_decl>>)
				return self.visit_var_decl(*subnode);
			else if constexpr (std::is_same_v<type, node_ref<naked_proj_decl>>)
				return self.visit_naked_proj_decl(*subnode);
			else
				assert(false);
		};

		return std::visit(visit, node);
	}

	template<typename walk>
	bool traverse_var_decl(walk&& self, const var_decl& node)
	{
		return self.visit_tyname(*node.type);
	}

	template<typename walk>
	bool traverse_tyname(walk&& self, const tyname& node)
	{
		if (!self.visit_tyname_stem(*node.stem))
			return false;
		
		for (const auto& item : node.indirs) {
			if (!self.visit_indirection(*item))
				return false;
		}

		return true;
	}

	template<typename walk>
	bool traverse_indirection(walk&& self, const indirection& node)
	{
		return true;
	}

	template<typename walk>
	bool traverse_tyname_stem(walk&& self, const tyname_stem& node)
	{
		const auto visit = [&self](auto&& subnode) -> bool
		{
			using type = std::decay_t<decltype(subnode)>;
			if constexpr (std::is_same_v<type, node_ref<tyname_rpc>>)
				return self.visit_tyname_rpc(*subnode);
			else if constexpr (std::is_same_v<type, node_ref<tyname_proj>>)
				return self.visit_tyname_proj(*subnode);
			else if constexpr (std::is_same_v<type, node_ref<tyname_array>>)
				return self.visit_tyname_array(*subnode);
			else if constexpr (std::is_same_v<type, node_ref<tyname_any_of_ptr>>)
				return self.visit_tyname_any_of(*subnode);
			else if constexpr (std::is_same_v<type, tyname_string>)
				return true;
			else if constexpr (std::is_same_v<type, tyname_arith>)
				return true;
			else
				assert(false);
		};

		return std::visit(visit, node);
	}

	template<typename walk>
	bool traverse_tyname_any_of(walk&& self, const tyname_any_of_ptr& node)
	{
		for (const auto& item : *node.types) {
			if (!self.visit_tyname(*item))
				return false;
		}
		
		return true;
	}

	template<typename walk>
	bool traverse_tyname_array(walk&& self, const tyname_array& node)
	{
		if (!self.visit_tyname(*node.element))
			return false;

		return self.visit_array_size(*node.size);
	}

	template<typename walk>
	bool traverse_array_size(walk&& self, const array_size& node)
	{
		// this is a variant but it has no node_refs in it
		return true;
	}

	template<typename walk>
	bool traverse_tyname_proj(walk&& self, const tyname_proj& node)
	{
		return true;
	}

	template<typename walk>
	bool traverse_tyname_rpc(walk&& self, const tyname_rpc& node)
	{
		return true;
	}

	template<typename walk>
	bool traverse_naked_proj_decl(walk&& self, const naked_proj_decl& node)
	{
		if (node.fields) {
			for (const auto& item : *node.fields) {
				if (!self.visit_proj_field(*item))
					return false;
			}
		}

		return true;
	}

	template<typename walk>
	bool traverse_rpc_item(walk&& self, const rpc_item& node)
	{
		const auto visit = [&self](auto&& subnode) -> bool {
			using type = std::decay_t<decltype(subnode)>;
			if constexpr (std::is_same_v<type, node_ref<union_proj_def>>)
				return self.visit_union_proj_def(*subnode);
			else if constexpr (std::is_same_v<type, node_ref<struct_proj_def>>)
				return self.visit_struct_proj_def(*subnode);
			else if constexpr (std::is_same_v<type, node_ref<rpc_ptr_def>>)
				return self.visit_rpc_ptr_def(*subnode);
			else
				assert(false);
		};

		return std::visit(visit, node);
	}

	template<typename derived>
	class ast_walk {
	public:
		bool visit_file(const file& node)
		{
			traverse_file(self(), node);
			return true;
		}

		bool visit_module_def(const module_def& node)
		{
			traverse_module_def(self(), node);
			return true;
		}

		bool visit_driver_file(const driver_file& node)
		{
			traverse_driver_file(self(), node);
			return true;
		}

		bool visit_driver_def(const driver_def& node)
		{
			traverse_driver_def(self(), node);
			return true;
		}

		bool visit_module_item(const module_item& node)
		{
			traverse_module_item(self(), node);
			return true;
		}

		bool visit_union_proj_def(const union_proj_def& node)
		{
			traverse_union_proj_def(self(), node);
			return true;
		}

		bool visit_struct_proj_def(const struct_proj_def& node)
		{
			traverse_struct_proj_def(self(), node);
			return true;
		}

		bool visit_rpc_def(const rpc_def& node)
		{
			traverse_rpc_def(self(), node);
			return true;
		}

		bool visit_rpc_ptr_def(const rpc_ptr_def& node)
		{
			traverse_rpc_ptr_def(self(), node);
			return true;
		}

		bool visit_proj_field(const proj_field& node)
		{
			traverse_proj_field(self(), node);
			return true;
		}

		bool visit_var_decl(const var_decl& node)
		{
			traverse_var_decl(self(), node);
			return true;
		}

		bool visit_tyname(const tyname& node)
		{
			traverse_tyname(self(), node);
			return true;
		}

		bool visit_indirection(const indirection& node)
		{
			traverse_indirection(self(), node);
			return true;
		}

		bool visit_tyname_stem(const tyname_stem& node)
		{
			traverse_tyname_stem(self(), node);
			return true;
		}

		bool visit_tyname_any_of(const tyname_any_of_ptr& node)
		{
			traverse_tyname_any_of(self(), node);
			return true;
		}

		bool visit_tyname_array(const tyname_array& node)
		{
			traverse_tyname_array(self(), node);
			return true;
		}

		bool visit_array_size(const array_size& node)
		{
			traverse_array_size(self(), node);
			return true;
		}

		bool visit_tyname_proj(const tyname_proj& node)
		{
			traverse_tyname_proj(self(), node);
			return true;
		}

		bool visit_tyname_rpc(const tyname_rpc& node)
		{
			traverse_tyname_rpc(self(), node);
			return true;
		}

		bool visit_naked_proj_decl(const naked_proj_decl& node)
		{
			traverse_naked_proj_decl(self(), node);
			return true;
		}

		bool visit_rpc_item(const rpc_item& node)
		{
			traverse_rpc_item(self(), node);
			return true;
		}

	private:
		auto& self()
		{
			return *reinterpret_cast<derived*>(this);
		}
	};

	class null_walk : public ast_walk<null_walk> {
	public:
		bool visit_file(const file& node)
		{
			std::cout << "[Nullwalk] file" << std::endl;
			traverse_file(*this, node);
			return true;
		}

		bool visit_module_def(const module_def& node)
		{
			std::cout << "[Nullwalk] module_def" << std::endl;
			traverse_module_def(*this, node);
			return true;
		}

		bool visit_driver_file(const driver_file& node)
		{
			std::cout << "[Nullwalk] driver_file" << std::endl;
			traverse_driver_file(*this, node);
			return true;
		}

		bool visit_driver_def(const driver_def& node)
		{
			std::cout << "[Nullwalk] driver_def" << std::endl;
			traverse_driver_def(*this, node);
			return true;
		}

		bool visit_module_item(const module_item& node)
		{
			std::cout << "[Nullwalk] module_item" << std::endl;
			traverse_module_item(*this, node);
			return true;
		}

		bool visit_union_proj_def(const union_proj_def& node)
		{
			std::cout << "[Nullwalk] union_proj_def" << std::endl;
			traverse_union_proj_def(*this, node);
			return true;
		}

		bool visit_struct_proj_def(const struct_proj_def& node)
		{
			std::cout << "[Nullwalk] struct_proj_def" << std::endl;
			traverse_struct_proj_def(*this, node);
			return true;
		}

		bool visit_rpc_def(const rpc_def& node)
		{
			std::cout << "[Nullwalk] rpc_def" << std::endl;
			traverse_rpc_def(*this, node);
			return true;
		}

		bool visit_rpc_ptr_def(const rpc_ptr_def& node)
		{
			std::cout << "[Nullwalk] rpc_ptr_def" << std::endl;
			traverse_rpc_ptr_def(*this, node);
			return true;
		}

		bool visit_proj_field(const proj_field& node)
		{
			std::cout << "[Nullwalk] proj_field" << std::endl;
			traverse_proj_field(*this, node);
			return true;
		}

		bool visit_var_decl(const var_decl& node)
		{
			std::cout << "[Nullwalk] var_decl" << std::endl;
			traverse_var_decl(*this, node);
			return true;
		}

		bool visit_tyname(const tyname& node)
		{
			std::cout << "[Nullwalk] tyname" << std::endl;
			traverse_tyname(*this, node);
			return true;
		}

		bool visit_indirection(const indirection& node)
		{
			std::cout << "[Nullwalk] indirection" << std::endl;
			traverse_indirection(*this, node);
			return true;
		}

		bool visit_tyname_stem(const tyname_stem& node)
		{
			std::cout << "[Nullwalk] tyname_stem" << std::endl;
			traverse_tyname_stem(*this, node);
			return true;
		}

		bool visit_tyname_any_of(const tyname_any_of_ptr& node)
		{
			std::cout << "[Nullwalk] tyname_any_of" << std::endl;
			traverse_tyname_any_of(*this, node);
			return true;
		}

		bool visit_tyname_array(const tyname_array& node)
		{
			std::cout << "[Nullwalk] tyname_array" << std::endl;
			traverse_tyname_array(*this, node);
			return true;
		}

		bool visit_array_size(const array_size& node)
		{
			std::cout << "[Nullwalk] array_size" << std::endl;
			traverse_array_size(*this, node);
			return true;
		}

		bool visit_tyname_proj(const tyname_proj& node)
		{
			std::cout << "[Nullwalk] tyname_proj" << std::endl;
			traverse_tyname_proj(*this, node);
			return true;
		}

		bool visit_tyname_rpc(const tyname_rpc& node)
		{
			std::cout << "[Nullwalk] tyname_rpc" << std::endl;
			traverse_tyname_rpc(*this, node);
			return true;
		}

		bool visit_naked_proj_decl(const naked_proj_decl& node)
		{
			std::cout << "[Nullwalk] naked_proj_decl" << std::endl;
			traverse_naked_proj_decl(*this, node);
			return true;
		}

		bool visit_rpc_item(const rpc_item& node)
		{
			std::cout << "[Nullwalk] rpc_item" << std::endl;
			traverse_rpc_item(*this, node);
			return true;
		}
	};
}

#endif
