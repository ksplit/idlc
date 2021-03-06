#ifndef IDLC_SEMA_SCOPE_H
#define IDLC_SEMA_SCOPE_H

#include <algorithm>
#include <iterator>
#include <vector>
#include <variant>

#include "../string_heap.h"
namespace idlc {
	struct proj_def;
	struct rpc_def;
	struct lock_def;
}

namespace idlc {
	class names_scope {
	public:
		void set_path(gsl::span<const ident> path)
		{
			for (const auto id : path) {
				m_path += "::";
				m_path += id;
			}
		}

		std::string_view get_path()
		{
			return m_path;
		}

		template<typename node_type>
		void insert(ident name, node_type* ptr)
		{
			m_names.emplace_back(name);
			m_symbols.emplace_back(ptr);
		}

		template<typename type>
		type* get(ident name) const
		{
			using namespace std;
			const auto first = begin(m_names);
			const auto last = end(m_names);
			const auto iter = std::find(first, last, name);
			return (iter == last) ? nullptr : std::get<type*>(gsl::at(m_symbols, std::distance(first, iter)));
		}

	private:
		std::vector<ident> m_names {};
		std::vector<std::variant<proj_def*, rpc_def*, lock_def*>> m_symbols {};
		std::string m_path {};
	};
}

#endif
