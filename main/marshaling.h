#ifndef _MARSHALING_H_
#define _MARSHALING_H_

#include "../parser/ast.h"

namespace idlc {
	struct marshal_unit {
		const signature* rpc_signature;
		std::string identifier;
	};

	bool process_marshal_units(gsl::span<const marshal_unit> units);
}

#endif // !_MARSHALING_H_
