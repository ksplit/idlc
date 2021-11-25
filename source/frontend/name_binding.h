#ifndef IDLC_SEMA_NAME_BINDING_H
#define IDLC_SEMA_NAME_BINDING_H

#include <iostream>

#include "../ast/ast.h"

namespace idlc {
	bool bind_all_names(file& root);
}

#endif
