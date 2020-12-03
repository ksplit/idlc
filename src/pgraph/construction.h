#ifndef _IDLC_PGRAPH_CONSTRUCTION_H_
#define _IDLC_PGRAPH_CONSTRUCTION_H_

#include <vector>

#include "tree.h"
#include "../ast/ast.h"
#include "../sema/nodedb.h"

namespace idlc::pgraph {
	struct rpc_tables {
		std::vector<rpc_node> direct;
		std::vector<rpc_node> indirect;
	};

	rpc_tables build_rpc_table(const ast::file& file, const sema::type_scope_db& types);
}

#endif
