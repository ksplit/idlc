#ifndef IDLC_SEMA_DEFAULT_WALK_H
#define IDLC_SEMA_DEFAULT_WALK_H

#include "pgraph.h"

#include <algorithm>
#include <cassert>
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
			const auto visit = [this, &pass](auto&& item)
			{
				using type = std::decay_t<decltype(item)>;
				if constexpr (std::is_same_v<type, primitive>) {
					// There is nothing to be walked here
					// TODO: yet?
					return true;
				}
				else if constexpr (std::is_same_v<type, projection_ptr>) {
					if (!visited(item.get())) {
						visited_.emplace_back(item.get());
						return pass.visit_projection(*item);
					}

					std::cout << "[debug] short-circuited projection_ptr\n";

					return true;
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
				else {
					assert(false); // something isn't implemented
				}

				return false;
			};

			return std::visit(visit, node);
		}

		bool operator()(walk& pass, projection& node)
		{
			for (auto& [name, item] : node.fields) {
				if (!pass.visit_data_field(*item))
					return false;
			}

			return true;
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

	private:
		std::vector<projection*> visited_;

		bool visited(projection* ptr)
		{
			return std::find(visited_.begin(), visited_.end(), ptr) != visited_.end();
		}
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

	protected:
		// TODO: does this *need* to be a protected member
		pgraph_traverse<derived> traverse {};

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

	class null_walk : public pgraph_walk<null_walk> {
	public:
		bool visit_data_field(data_field& node)
		{
			tab_over(std::cout << "[debug]", level_) << "data_field\n";
			++level_;
			if (!traverse(*this, node))
				return false;

			--level_;

			return true;
		}

		bool visit_field_type(field_type& node)
		{
			tab_over(std::cout << "[debug]", level_) << "field_type\n";
			++level_;
			if (!traverse(*this, node))
				return false;

			--level_;

			return true;
		}

		bool visit_projection(projection& node)
		{
			tab_over(std::cout << "[debug]", level_) << "projection\n";
			++level_;
			if (!traverse(*this, node))
				return false;

			--level_;

			return true;
		}

		bool visit_dyn_array(dyn_array& node)
		{
			tab_over(std::cout << "[debug]", level_) << "dyn_array\n";
			++level_;
			if (!traverse(*this, node))
				return false;

			--level_;

			return true;
		}

		bool visit_null_terminated_array(null_terminated_array& node)
		{
			tab_over(std::cout << "[debug]", level_) << "null_terminated_array\n";
			++level_;
			if (!traverse(*this, node))
				return false;

			--level_;

			return true;
		}

		bool visit_pointer(pointer& node)
		{
			tab_over(std::cout << "[debug]", level_) << "pointer\n";
			++level_;
			if (!traverse(*this, node))
				return false;

			--level_;

			return true;
		}

		bool visit_rpc_ptr(rpc_ptr& node)
		{
			tab_over(std::cout << "[debug]", level_) << "rpc_ptr\n";
			return true;
		}

	private:
		unsigned level_ {1};
	};
}

#endif
