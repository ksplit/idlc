#ifndef IDLC_SEMA_DEFAULT_WALK_H
#define IDLC_SEMA_DEFAULT_WALK_H

#include "pgraph.h"

namespace idlc::sema {
	class default_walk {
	public:
		bool visit_data_field(data_field& node)
		{
			return true;
		}

	private:
	};
}

#endif
