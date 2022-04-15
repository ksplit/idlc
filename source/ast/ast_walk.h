#ifndef IDLC_AST_AST_WALK_H_
#define IDLC_AST_AST_WALK_H_

#include <type_traits>
#include <iostream>

#include "ast.h"

namespace idlc {
	/*
		Walk deficiencies:
		- Subtree walks have no protection (can have field_walk walk over mutiple fields, for instance)
		- Walk system is perhaps *too* general
		- Easy to forget that explicit traversals are needed *but* this makes complicated walks simpler to write
		- Clang- vs syn- style AST walks
	*/

	// NOTE: traverse* DO NOT visit the node given to them, only subnodes

	template<typename walk>
	bool traverse(walk&& self, file& node)
	{
		const auto visit = [&self](auto&& subnode) -> bool
		{
			using type = std::decay_t<decltype(subnode)>;
			if constexpr (std::is_same_v<type, node_ref<std::vector<node_ref<module_def>>>>) {
				for (auto& def : *subnode) {
					if (!self.visit_module_def(*def))
						return false;
				}

				return true;
			}
			else if constexpr (std::is_same_v<type, node_ref<driver_file>>) {
				return self.visit_driver_file(*subnode);
			}

			std::terminate();
		};			

		return std::visit(visit, node);
	}

	template<typename walk>
	bool traverse(walk&& self, module_def& node)
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
	bool traverse(walk&& self, const driver_file& node)
	{
		return self.visit_driver_def(*node.driver);
	}

	template<typename walk>
	bool traverse(walk&& self, const driver_def& node)
	{
		return true;
	}

	template<typename walk>
	bool traverse(walk&& self, const module_item& node)
	{
		const auto visit = [&self](auto&& subnode) -> bool
		{
			using type = std::decay_t<decltype(subnode)>;
			if constexpr (std::is_same_v<type, header_stmt>)
				return true; // TODO: header_stmt impl
			else if constexpr (std::is_same_v<type, node_ref<proj_def>>)
				return self.visit_proj_def(*subnode);
			else if constexpr (std::is_same_v<type, node_ref<rpc_def>>)
				return self.visit_rpc_def(*subnode);
			else if constexpr (std::is_same_v<type, node_ref<global_def>>)
				return self.visit_global_def(*subnode);
			else if constexpr (std::is_same_v<type, node_ref<lock_def>>)
				return self.visit_lock_def(*subnode);
			
			std::terminate();
		};

		return std::visit(visit, node);
	}

	template<typename walk>
	bool traverse(walk&& self, const proj_def& node)
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
	bool traverse(walk&& self, const rpc_def& node)
	{
		if (!self.visit_type_spec(*node.ret_type))
			return false;

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
	bool traverse(walk&& self, const global_def& node)
	{
		if (!self.visit_type_spec(*node.type))
			return false;

		if (node.items) {
			for (const auto& item : *node.items) {
				if (!self.visit_rpc_item(*item))
					return false;
			}
		}

		return true;
	}

	template<typename walk>
	bool traverse(walk&& self, const lock_def& node)
	{
		return true;
	}

	template<typename walk>
	bool traverse(walk&& self, const lock_scope& node)
	{
		return true;
	}

	template<typename walk>
	bool traverse(walk&& self, const proj_field& node)
	{
		const auto& [def, size] = node;
		const auto visit = [&self](auto&& subnode) -> bool
		{
			using type = std::decay_t<decltype(subnode)>;
			if constexpr (std::is_same_v<type, node_ref<var_decl>>)
				return self.visit_var_decl(*subnode);
			else if constexpr (std::is_same_v<type, node_ref<naked_proj_decl>>)
				return self.visit_naked_proj_decl(*subnode);\
			else if constexpr (std::is_same_v<type, node_ref<lock_scope>>)
				return self.visit_lock_scope(*subnode);
			else if constexpr (std::is_same_v<type, node_ref<lock_def>>)
				return self.visit_lock_def(*subnode);
			
			std::terminate();
		};

		return std::visit(visit, def);
	}

	template<typename walk>
	bool traverse(walk&& self, const var_decl& node)
	{
		return self.visit_type_spec(*node.type);
	}

	template<typename walk>
	bool traverse(walk&& self, const type_spec& node)
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
	bool traverse(walk&& self, const indirection& node)
	{
		return true;
	}

	template<typename walk>
	bool traverse(walk&& self, const type_stem& node)
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
			else if constexpr (std::is_same_v<type, type_none>)
				return self.visit_type_none(subnode);
			else if constexpr (std::is_same_v<type, type_string>) // FIXME: how to properly walk these "folded" nodes?
				return true;
			else if constexpr (std::is_same_v<type, node_ref<type_casted>>)
				return self.visit_type_casted(*subnode);
			else if constexpr (std::is_same_v<type, type_primitive>)
				return true;
			
			std::terminate();
		};

		return std::visit(visit, node);
	}

	template<typename walk>
	bool traverse(walk&& self, const type_array& node)
	{
		if (!self.visit_type_spec(*node.element))
			return false;

		return self.visit_array_size(*node.size);
	}

	template<typename walk>
	bool traverse(walk&& self, const array_size& node)
	{
		// this is a variant but it has no node_refs in it
		return true;
	}

	template<typename walk>
	bool traverse(walk&& self, const type_proj& node)
	{
		return true;
	}

	template<typename walk>
	bool traverse(walk&& self, const type_rpc& node)
	{
		return true;
	}

	template<typename walk>
	bool traverse(walk&& self, const naked_proj_decl& node)
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
	bool traverse(walk&& self, const rpc_item& node)
	{
		const auto visit = [&self](auto&& subnode) -> bool {
			using type = std::decay_t<decltype(subnode)>;
			if constexpr (std::is_same_v<type, node_ref<proj_def>>)
				return self.visit_proj_def(*subnode);
			
			std::terminate();
		};

		return std::visit(visit, node);
	}

	template<typename walk>
	bool traverse(walk&& self, const type_casted& node)
	{
		if (!self.visit_type_spec(*node.declared_type))
			return false;

		return self.visit_type_spec(*node.true_type);
	}

	template<typename derived>
	class ast_walk {
	public:
		bool visit_file(file& node)
		{
			return traverse(self(), node);
		}

		bool visit_module_def(module_def& node)
		{
			return traverse(self(), node);
		}

		bool visit_driver_file(driver_file& node)
		{
			return traverse(self(), node);
		}

		bool visit_driver_def(driver_def& node)
		{
			return traverse(self(), node);
		}

		bool visit_module_item(module_item& node)
		{
			return traverse(self(), node);
		}

		bool visit_proj_def(proj_def& node)
		{
			return traverse(self(), node);
		}

		bool visit_rpc_def(rpc_def& node)
		{
			return traverse(self(), node);
		}

		bool visit_proj_field(proj_field& node)
		{
			return traverse(self(), node);
		}

		bool visit_var_decl(var_decl& node)
		{
			return traverse(self(), node);
		}

		bool visit_type_spec(type_spec& node)
		{
			return traverse(self(), node);
		}

		bool visit_indirection(indirection& node)
		{
			return traverse(self(), node);
		}

		bool visit_type_stem(type_stem& node)
		{
			return traverse(self(), node);
		}

		bool visit_type_array(type_array& node)
		{
			return traverse(self(), node);
		}

		bool visit_array_size(array_size& node)
		{
			return traverse(self(), node);
		}

		bool visit_type_proj(type_proj& node)
		{
			return traverse(self(), node);
		}

		bool visit_type_rpc(type_rpc& node)
		{
			return traverse(self(), node);
		}

		bool visit_naked_proj_decl(naked_proj_decl& node)
		{
			return traverse(self(), node);
		}

		bool visit_rpc_item(rpc_item& node)
		{
			return traverse(self(), node);
		}

		bool visit_global_def(global_def& node)
		{
			return traverse(self(), node);
		}

		bool visit_lock_def(lock_def& node)
		{
			return traverse(self(), node);
		}

		bool visit_lock_scope(lock_scope& node)
		{
			return traverse(self(), node);
		}

		bool visit_type_none(type_none)
		{
			return true;
		}

		bool visit_type_casted(type_casted& node)
		{
			return traverse(self(), node);
		}

	private:
		auto& self()
		{
			return *reinterpret_cast<derived*>(this);
		}
	};
}

#endif
