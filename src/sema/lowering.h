#ifndef IDLC_SEMA_LOWERING_H
#define IDLC_SEMA_LOWERING_H

#include <vector>

#include <gsl/gsl>

#include "../ast/ast.h"

namespace idlc::sema {
	std::vector<gsl::not_null<ast::rpc_def*>> get_rpcs(ast::file& root);
	bool lower(gsl::span<const gsl::not_null<ast::rpc_def*>> rpcs);
}

#endif
