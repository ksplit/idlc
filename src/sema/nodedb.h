#ifndef _IDLC_SEMA_NODEDB_H_
#define _IDLC_SEMA_NODEDB_H_

#include <map>
#include <vector>
#include <list>
#include <functional>

#include "../ast/ast.h"
#include "../ast/walk.h"

#include "resolution.h"

namespace idlc::sema {
	using node_id = const void*; // Right now, we identify nodes by address
	using node_kind = const void*;

	template<typename type>
	constexpr void dummy() {}

	template<typename type>
	constexpr node_kind get_kind() { return dummy<type>; }

	struct node_meta {
		node_id parent;
		node_kind kind;
		const void* data;

		template<typename type>
		node_meta(node_id parent, const type& node) :
			parent {parent},
			kind {get_kind<type>()},
			data {&node}
		{}
	};
	
	using meta_table = std::map<node_id, node_meta>;

	meta_table build_meta_table(const ast::file&);

	/*******************************************************************************************************************
		Type scope database
	*******************************************************************************************************************/

	using type_scope_chain = std::vector<decltype(std::cref(std::declval<types_rib&>()))>;
	using type_scope_chain_table = std::map<node_id, type_scope_chain>;

	struct type_scope_db {
		std::list<std::unique_ptr<types_rib>> scopes;
		type_scope_chain_table scope_chains;
	};

	type_scope_db build_types_db(const ast::file&);
	void dump(const type_scope_db& db);
}

#endif
