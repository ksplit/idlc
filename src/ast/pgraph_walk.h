#ifndef IDLC_AST_PGRAPH_WALK_H
#define IDLC_AST_PGRAPH_WALK_H

#include "pgraph.h"

#include <algorithm>
#include <iostream>
#include <type_traits>
#include <variant>

namespace idlc {
	template<typename walk>
	class pgraph_traverse {
	public:
		bool operator()(walk& pass, value& node)
		{
			if (!pass.visit_field_type(node.type))
				return false;

			return true;
		}

		bool operator()(walk& pass, passed_type& node)
		{
			const auto visit = [this, &pass](auto&& item) -> bool
			{
				using type = std::decay_t<decltype(item)>;
				if constexpr (std::is_same_v<type, primitive>) {
					return pass.visit_primitive(item);
				}
				else if constexpr (std::is_same_v<type, node_ref<projection>>) {
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
				else if constexpr (std::is_same_v<type, gsl::not_null<proj_def*>>) {
					return pass.visit_proj_def(*item);
				}
				
				std::terminate();
			};

			return std::visit(visit, node);
		}

		bool operator()(walk& pass, projection& node)
		{
			// NOTE: do not remove me!!! See notes in circularity.idl: direct or indirect self-reference requires
			// a notion of pgraph sharing, which the walk code must account for
			if (visited(&node)) {
				std::cout << "short-circuiting projection\n";
				return true;
			}
			else {
				visited_.push_back(&node); // NOTE: this must be here, to prevent nested calls from recursing infinitely
				for (auto& [name, item] : node.fields) {
					if (!pass.visit_value(*item))
						return false;
				}

				return true;
			}
		}

		bool operator()(walk& pass, dyn_array& node)
		{
			return pass.visit_value(*node.element);
		}

		bool operator()(walk& pass, null_terminated_array& node)
		{
			return pass.visit_value(*node.element);
		}

		bool operator()(walk& pass, pointer& node)
		{
			return pass.visit_value(*node.referent);
		}

		bool operator()(walk& pass, rpc_ptr& node)
		{
			return true;
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
		bool visit_value(value& node)
		{
			return traverse(self(), node);
		}

		bool visit_field_type(passed_type& node)
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

		bool visit_proj_def(proj_def& node)
		{
			return true;
		}

	protected:
		// TODO: does this *need* to be a protected member
		pgraph_traverse<derived> traverse {};
		
		auto& self()
		{
			return *reinterpret_cast<derived*>(this);
		}
	};
}

#endif
