#ifndef _LCDS_IDL_PARSER_WALK_H_
#define _LCDS_IDL_PARSER_WALK_H_

#include <type_traits>
#include <cassert>
#include <iostream>

#include "ast.h"

namespace idlc::ast {
	/*
		Walk deficiencies:
		- Subtree walks have no protection (can have field_walk walk over mutiple fields, for instance)
		- Walk system is perhaps *too* general
		- Easy to forget that explicit traversals are needed *but* this makes complicated walks simpler to write
		- Clang- vs syn- style AST walks
	*/

	// NOTE: traverse_* DO NOT visit the node given to them, only subnodes

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
			else if constexpr (std::is_same_v<type, node_ref<proj_def>>)
				return self.visit_proj_def(*subnode);
			else if constexpr (std::is_same_v<type, node_ref<rpc_def>>)
				return self.visit_rpc_def(*subnode);
			else
				assert(false);
		};

		return std::visit(visit, node);
	}

	template<typename walk>
	bool traverse_proj_def(walk&& self, const proj_def& node)
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
			if (!self.visit_type_spec(*node.ret_type))
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
		return self.visit_type_spec(*node.type);
	}

	template<typename walk>
	bool traverse_type_spec(walk&& self, const type_spec& node)
	{
		if (!self.visit_type_stem(*node.stem))
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
	bool traverse_type_stem(walk&& self, const type_stem& node)
	{
		const auto visit = [&self](auto&& subnode) -> bool
		{
			using type = std::decay_t<decltype(subnode)>;
			if constexpr (std::is_same_v<type, node_ref<type_rpc>>)
				return self.visit_type_rpc(*subnode);
			else if constexpr (std::is_same_v<type, node_ref<type_proj>>)
				return self.visit_type_proj(*subnode);
			else if constexpr (std::is_same_v<type, node_ref<type_array>>)
				return self.visit_type_array(*subnode);
			else if constexpr (std::is_same_v<type, node_ref<type_any_of>>)
				return self.visit_type_any_of(*subnode);
			else if constexpr (std::is_same_v<type, type_string>) // FIXME: how to properly walk these "folded" nodes?
				return true;
			else if constexpr (std::is_same_v<type, type_primitive>)
				return true;
			else
				assert(false);
		};

		return std::visit(visit, node);
	}

	template<typename walk>
	bool traverse_type_any_of(walk&& self, const type_any_of& node)
	{
		for (const auto& item : *node.types) {
			if (!self.visit_type_spec(*item))
				return false;
		}
		
		return true;
	}

	template<typename walk>
	bool traverse_type_array(walk&& self, const type_array& node)
	{
		if (!self.visit_type_spec(*node.element))
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
	bool traverse_type_proj(walk&& self, const type_proj& node)
	{
		return true;
	}

	template<typename walk>
	bool traverse_type_rpc(walk&& self, const type_rpc& node)
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
			if constexpr (std::is_same_v<type, node_ref<proj_def>>)
				return self.visit_proj_def(*subnode);
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

		bool visit_proj_def(const proj_def& node)
		{
			traverse_proj_def(self(), node);
			return true;
		}

		bool visit_rpc_def(const rpc_def& node)
		{
			traverse_rpc_def(self(), node);
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

		bool visit_type_spec(const type_spec& node)
		{
			traverse_type_spec(self(), node);
			return true;
		}

		bool visit_indirection(const indirection& node)
		{
			traverse_indirection(self(), node);
			return true;
		}

		bool visit_type_stem(const type_stem& node)
		{
			traverse_type_stem(self(), node);
			return true;
		}

		bool visit_type_any_of(const type_any_of& node)
		{
			traverse_type_any_of(self(), node);
			return true;
		}

		bool visit_type_array(const type_array& node)
		{
			traverse_type_array(self(), node);
			return true;
		}

		bool visit_array_size(const array_size& node)
		{
			traverse_array_size(self(), node);
			return true;
		}

		bool visit_type_proj(const type_proj& node)
		{
			traverse_type_proj(self(), node);
			return true;
		}

		bool visit_type_rpc(const type_rpc& node)
		{
			traverse_type_rpc(self(), node);
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

	inline auto& tab_over(std::ostream& os, unsigned n)
	{
		for (int i {}; i < n; ++i)
			os << "  ";

		return os;
	}

	class null_walk : public ast_walk<null_walk> {
	public:
		bool visit_file(const file& node)
		{
			tab_over(std::cout << "[nullwalk]  ", level_) << "file" << std::endl;
			++level_;
			traverse_file(*this, node);
			--level_;
			return true;
		}

		bool visit_module_def(const module_def& node)
		{
			tab_over(std::cout << "[nullwalk]  ", level_) << "module_def" << std::endl;
			++level_;
			traverse_module_def(*this, node);
			--level_;
			return true;
		}

		bool visit_driver_file(const driver_file& node)
		{
			tab_over(std::cout << "[nullwalk]  ", level_) << "driver_file" << std::endl;
			++level_;
			traverse_driver_file(*this, node);
			--level_;
			return true;
		}

		bool visit_driver_def(const driver_def& node)
		{
			tab_over(std::cout << "[nullwalk]  ", level_) << "driver_def" << std::endl;
			++level_;
			traverse_driver_def(*this, node);
			--level_;
			return true;
		}

		bool visit_module_item(const module_item& node)
		{
			tab_over(std::cout << "[nullwalk]  ", level_) << "module_item" << std::endl;
			++level_;
			traverse_module_item(*this, node);
			--level_;
			return true;
		}

		bool visit_proj_def(const proj_def& node)
		{
			tab_over(std::cout << "[nullwalk]  ", level_) << "union_proj_def" << std::endl;
			++level_;
			traverse_proj_def(*this, node);
			--level_;
			return true;
		}

		bool visit_rpc_def(const rpc_def& node)
		{
			tab_over(std::cout << "[nullwalk]  ", level_) << "rpc_def" << std::endl;
			++level_;
			traverse_rpc_def(*this, node);
			--level_;
			return true;
		}

		bool visit_proj_field(const proj_field& node)
		{
			tab_over(std::cout << "[nullwalk]  ", level_) << "proj_field" << std::endl;
			++level_;
			traverse_proj_field(*this, node);
			--level_;
			return true;
		}

		bool visit_var_decl(const var_decl& node)
		{
			tab_over(std::cout << "[nullwalk]  ", level_) << "var_decl" << std::endl;
			++level_;
			traverse_var_decl(*this, node);
			--level_;
			return true;
		}

		bool visit_type_spec(const type_spec& node)
		{
			tab_over(std::cout << "[nullwalk]  ", level_) << "type_spec" << std::endl;
			++level_;
			traverse_type_spec(*this, node);
			--level_;
			return true;
		}

		bool visit_indirection(const indirection& node)
		{
			tab_over(std::cout << "[nullwalk]  ", level_) << "indirection" << std::endl;
			++level_;
			traverse_indirection(*this, node);
			--level_;
			return true;
		}

		bool visit_type_stem(const type_stem& node)
		{
			tab_over(std::cout << "[nullwalk]  ", level_) << "type_stem" << std::endl;
			++level_;
			traverse_type_stem(*this, node);
			--level_;
			return true;
		}

		bool visit_type_any_of(const type_any_of& node)
		{
			tab_over(std::cout << "[nullwalk]  ", level_) << "type_any_of" << std::endl;
			++level_;
			traverse_type_any_of(*this, node);
			--level_;
			return true;
		}

		bool visit_type_array(const type_array& node)
		{
			tab_over(std::cout << "[nullwalk]  ", level_) << "type_array" << std::endl;
			++level_;
			traverse_type_array(*this, node);
			--level_;
			return true;
		}

		bool visit_array_size(const array_size& node)
		{
			tab_over(std::cout << "[nullwalk]  ", level_) << "array_size" << std::endl;
			++level_;
			traverse_array_size(*this, node);
			--level_;
			return true;
		}

		bool visit_type_proj(const type_proj& node)
		{
			tab_over(std::cout << "[nullwalk]  ", level_) << "type_proj" << std::endl;
			++level_;
			traverse_type_proj(*this, node);
			--level_;
			return true;
		}

		bool visit_type_rpc(const type_rpc& node)
		{
			tab_over(std::cout << "[nullwalk]  ", level_) << "type_rpc" << std::endl;
			++level_;
			traverse_type_rpc(*this, node);
			--level_;
			return true;
		}

		bool visit_naked_proj_decl(const naked_proj_decl& node)
		{
			tab_over(std::cout << "[nullwalk]  ", level_) << "naked_proj_decl" << std::endl;
			++level_;
			traverse_naked_proj_decl(*this, node);
			--level_;
			return true;
		}

		bool visit_rpc_item(const rpc_item& node)
		{
			tab_over(std::cout << "[nullwalk]  ", level_) << "rpc_item" << std::endl;
			++level_;
			traverse_rpc_item(*this, node);
			--level_;
			return true;
		}

	private:
		unsigned level_ {};
	};
}

#endif
