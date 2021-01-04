#ifndef IDLC_SEMA_SCOPE_H
#define IDLC_SEMA_SCOPE_H

#include <algorithm>
#include <iterator>
#include <vector>
#include <variant>

#include "../parser/string_heap.h"

// TODO: move these
namespace idlc::ast {
	struct proj_def;
	struct rpc_def;
}

namespace idlc::sema {
	class scope {
	public:
		void insert(ident name, ast::proj_def* ptr)
		{
			names.emplace_back(name);
			symbols.emplace_back(ptr);
		}

		void insert(ident name, ast::rpc_def* ptr)
		{
			names.emplace_back(name);
			symbols.emplace_back(ptr);
		}

		template<typename type>
		type* get(ident name)
		{
			using namespace std;
			const auto first = begin(names);
			const auto last = end(names);
			const auto iter = std::find(first, last, name);
			return (iter == last) ? nullptr : std::get<type*>(gsl::at(symbols, std::distance(first, iter)));
		}

	private:
		std::vector<ident> names {};
		std::vector<std::variant<ast::proj_def*, ast::rpc_def*>> symbols {};
	};
}

#endif
