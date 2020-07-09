#ifndef _TYPE_MAP_H_
#define _TYPE_MAP_H_

#include "gsl/gsl"

namespace idlc {
	class projection;

	class type_map {
	public:
		const projection* get(gsl::czstring<> identifier) const;

		// Returns false if we're trying to add an existing type
		bool insert(const projection& projection);

	private:
		std::vector<gsl::czstring<>> m_keys;
		std::vector<const projection*> m_types;
	};
}

#endif