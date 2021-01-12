#ifndef IDLC_SEMA_NAME_BINDING_H
#define IDLC_SEMA_NAME_BINDING_H

#include <iostream>

#include "../ast/ast.h"

namespace idlc::sema {
	bool bind_all(ast::file& root);
}

#endif
