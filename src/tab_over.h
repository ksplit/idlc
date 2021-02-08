#ifndef IDLC_TAB_OVER_H
#define IDLC_TAB_OVER_H

#include <iostream>

namespace idlc {
	inline auto& indent(std::ostream& os, unsigned n)
	{
		for (int i {}; i < n; ++i)
			os << "\t";

		return os;
	}
}

#endif
