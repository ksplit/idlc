#ifndef IDLC_SEMA_SCOPE_H
#define IDLC_SEMA_SCOPE_H

#include <algorithm>
#include <iterator>
#include <vector>
#include <variant>

#include "../string_heap.h"

// TODO: consider merging this into the AST module

// TODO: move these
namespace idlc {
	struct proj_def;
	struct rpc_def;
}

namespace idlc {
	class names_scope {
	public:
		void insert(ident name, proj_def* ptr)
		{
			m_names.emplace_back(name);
			m_symbols.emplace_back(ptr);
		}

		void insert(ident name, rpc_def* ptr)
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
		std::vector<std::variant<proj_def*, rpc_def*>> m_symbols {};
	};
}

#endif
