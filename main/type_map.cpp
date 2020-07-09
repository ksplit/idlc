#include "type_map.h"
#include "../parser/ast.h"

const idlc::projection* idlc::type_map::get(gsl::czstring<> identifier) const
{
	const auto first = begin(m_keys);
	const auto last = end(m_keys);
	const auto find_it = std::find(first, last, identifier);

	if (find_it == last) {
		return nullptr;
	}
	else {
		return m_types.at(std::distance(first, find_it));
	}
}

bool idlc::type_map::insert(const projection& projection)
{
	const auto identifier = projection.identifier();
	const auto last = end(m_keys);
	const auto find_it = std::find(begin(m_keys), last, identifier);

	if (find_it == last) {
		m_keys.push_back(identifier);
		m_types.push_back(&projection);
		return true;
	}
	else {
		return false;
	}
}