#ifndef IDLC_UTILITY_H
#define IDLC_UTILITY_H

#include <iostream>
#include <string>

namespace idlc {
	inline auto& indent(std::ostream& os, unsigned n)
	{
		for (int i {}; i < n; ++i)
			os << "\t";

		return os;
	}

	inline void append(std::string&) {}
	
	template<typename item_type, typename... arg_types>
	void append(std::string& str, item_type item, arg_types... args)
	{
		str += item;
		append(str, args...);
	}

	template<typename... arg_types>
	auto concat(arg_types... args)
	{
		std::string str {};
		append(str, args...);
		return str;
	}
}

#endif
