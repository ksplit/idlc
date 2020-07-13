#ifndef _MARSHALING_H_
#define _MARSHALING_H_

#include "../parser/ast.h"

namespace idlc {
	struct marshal_unit {
		const signature* sig;
		std::string identifier;
	};
}

#endif // !_MARSHALING_H_
