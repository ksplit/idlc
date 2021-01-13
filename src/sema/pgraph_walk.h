#ifndef IDLC_SEMA_DEFAULT_WALK_H
#define IDLC_SEMA_DEFAULT_WALK_H

#include "pgraph.h"

#include <algorithm>
#include <iostream>
#include <type_traits>
#include <variant>

namespace idlc::sema {
	template<typename walk>
	class pgraph_traverse {
	public:
		bool operator()(walk& pass, data_field& node)
		{
			if (!pass.visit_field_type(node.type))
				return false;

			return true;
		}

		bool operator()(walk& pass, field_type& node)
		{
			const auto visit = [this, &pass](auto&& item) -> bool
			{
				using type = std::decay_t<decltype(item)>;
				if constexpr (std::is_same_v<type, primitive>) {
					return pass.visit_primitive(item);
				}
				else if constexpr (std::is_same_v<type, projection_ptr>) {
					return pass.visit_projection(*item);
				}
				else if constexpr (std::is_same_v<type, node_ptr<dyn_array>>) {
					return pass.visit_dyn_array(*item);
				}
				else if constexpr (std::is_same_v<type, node_ptr<null_terminated_array>>) {
					return pass.visit_null_terminated_array(*item);
				}
				else if constexpr (std::is_same_v<type, node_ptr<pointer>>) {
					return pass.visit_pointer(*item);
				}
				else if constexpr (std::is_same_v<type, node_ptr<static_void_ptr>>) {
					// TODO: static_void_ptr
				}
				else if constexpr (std::is_same_v<type, node_ptr<rpc_ptr>>) {
					return pass.visit_rpc_ptr(*item);
				}
				
				std::terminate();
			};

			return std::visit(visit, node);
		}

		bool operator()(walk& pass, projection& node)
		{
			if (visited(&node)) {
				std::cout << "[debug] short-circuiting projection\n";
				return true;
			}
			else {
				visited_.push_back(&node); // NOTE: this must be here, to prevent nested calls from recursing infinitely
				for (auto& [name, item] : node.fields) {
					if (!pass.visit_data_field(*item))
						return false;
				}

				return true;
			}
		}

		bool operator()(walk& pass, dyn_array& node)
		{
			return pass.visit_data_field(*node.element);
		}

		bool operator()(walk& pass, null_terminated_array& node)
		{
			return pass.visit_data_field(*node.element);
		}

		bool operator()(walk& pass, pointer& node)
		{
			return pass.visit_data_field(*node.referent);
		}
		
		// TODO: currently the *only* outside client of this is the dump walk, not clear if this is useful
		// more generally
		bool visited(projection* ptr)
		{
			return std::find(visited_.begin(), visited_.end(), ptr) != visited_.end();
		}

	private:
		std::vector<projection*> visited_;
	};

	template<typename derived>
	class pgraph_walk {
	public:
		bool visit_data_field(data_field& node)
		{
			return traverse(self(), node);
		}

		bool visit_field_type(field_type& node)
		{
			return traverse(self(), node);
		}

		bool visit_projection(projection& node)
		{
			return traverse(self(), node);
		}

		bool visit_dyn_array(dyn_array& node)
		{
			return traverse(self(), node);
		}

		bool visit_null_terminated_array(null_terminated_array& node)
		{
			return traverse(self(), node);
		}

		bool visit_pointer(pointer& node)
		{
			return traverse(self(), node);
		}

		bool visit_rpc_ptr(rpc_ptr& node)
		{
			return traverse(self(), node);
		}

		bool visit_primitive(primitive node)
		{
			return true;
		}

	protected:
		// TODO: does this *need* to be a protected member
		pgraph_traverse<derived> traverse {};

	private:
		auto& self()
		{
			return *reinterpret_cast<derived*>(this);
		}
	};
}

#endif
