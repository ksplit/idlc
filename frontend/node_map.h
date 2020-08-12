#ifndef _TYPE_MAP_H_
#define _TYPE_MAP_H_

#include "gsl/gsl"

namespace idlc {
	class projection;

	template<typename node_type>
	class node_map {
	public:
		node_type* get(gsl::czstring<> identifier) const
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

		// Returns false if we're trying to add an existing type
		bool insert(node_type& node)
		{
			const auto identifier = node.identifier();
			const auto last = end(m_keys);
			const auto find_it = std::find(begin(m_keys), last, identifier);

			if (find_it == last) {
				m_keys.push_back(identifier);
				m_types.push_back(&node);
				return true;
			}
			else {
				return false;
			}
		}

	private:
		std::vector<gsl::czstring<>> m_keys;
		std::vector<node_type*> m_types;
	};
}

#endif