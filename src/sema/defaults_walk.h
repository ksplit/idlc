#ifndef IDLC_SEMA_DEFAULTS_WALK_H
#define IDLC_SEMA_DEFAULTS_WALK_H

#include "../ast/ast.h"

namespace idlc::sema {
	bool propagate_defaults(ast::file& root);
}

#endif
