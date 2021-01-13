#ifndef IDLC_SEMA_DEFAULTS_WALK_H
#define IDLC_SEMA_DEFAULTS_WALK_H

#include <vector>

#include <gsl/gsl>

#include "../ast/ast.h"

namespace idlc::sema {
	std::vector<ast::rpc_def*> get_rpcs(ast::file& root);
	bool propagate_defaults(gsl::span<ast::rpc_def* const> rpcs);
}

#endif
