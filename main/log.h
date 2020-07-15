#ifndef _LOG_H_
#define _LOG_H_

#include <iostream>
#include <string_view>

namespace idlc {
	inline void log_insert() noexcept {}

	template<typename first_arg_type, typename ... arg_types>
	inline void log_insert(first_arg_type arg, arg_types... args)
	{
		std::cout << arg;
		log_insert(args...);
	}

	template<typename ... arg_types>
	void log_error(arg_types... args)
	{
		std::cout << "\x1b[31m[error] ";
		log_insert(args...);
		std::cout << "\x1b[0m\n";
	}

	template<typename ... arg_types>
	void log_warning(arg_types... args)
	{
		std::cout << "\x1b[33m[warning] ";
		log_insert(args...);
		std::cout << "\x1b[0m\n";
	}

	template<typename ... arg_types>
	void log_debug(arg_types... args)
	{
		std::cout << "\x1b[36m[debug] ";
		log_insert(args...);
		std::cout << "\x1b[0m\n";
	}

	template<typename ... arg_types>
	void log_note(arg_types... args)
	{
		std::cout << "[note] ";
		log_insert(args...);
		std::cout << "\n";
	}
}

#endif