#ifndef _MARSHALING_H_
#define _MARSHALING_H_

#include "../parser/ast.h"

namespace idlc {
	struct marshal_unit {
		const signature* rpc_signature;
		gsl::czstring<> identifier;
	};

	enum class marshal_unit_kind {
		direct,
		indirect
	};

	bool process_marshal_units(gsl::span<const marshal_unit> units, marshal_unit_kind kind);
}

#endif // !_MARSHALING_H_
