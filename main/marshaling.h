#ifndef _MARSHALING_H_
#define _MARSHALING_H_

#include "../parser/ast.h"

namespace idlc {
	struct marshal_unit {
		const signature* sig;
		std::string identifier;
	};

	enum class marshal_act {
		marshal_val,
		unmarshal_val,
		alloc_cspace,
		free_cspace,
		bind_cspace
	};

	void process_marshal_units(gsl::span<const marshal_unit> units);
}

#endif // !_MARSHALING_H_
