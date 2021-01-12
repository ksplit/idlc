#ifndef IDLC_TAB_OVER_H
#define IDLC_TAB_OVER_H

#include <iostream>

namespace idlc {
	inline auto& tab_over(std::ostream& os, unsigned n)
	{
		for (int i {}; i < n; ++i)
			os << "  ";

		return os;
	}
}

#endif
