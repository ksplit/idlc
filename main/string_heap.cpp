#include "string_heap.h"

#include <gsl/gsl>

gsl::czstring<> idlc::string_heap::intern(std::string_view string)
{
	Expects(string.length() != block_size - 1); // Or else we will never fit the string

	const auto sentinel = end(m_strings);
	const auto find_iterator = std::find(begin(m_strings), sentinel, string);
	if (find_iterator == sentinel) {
		const gsl::czstring<> new_string {add_string(string)};
		m_strings.push_back(new_string);
		return new_string;
	}
	else {
		return *find_iterator;
	}
}

gsl::czstring<> idlc::string_heap::add_string(std::string_view string)
{
	const auto end_index = m_free_index + string.length() + 1;
	if (end_index < block_size) {
		// Since the block is zero-initialized, we just copy the string bytes
		const gsl::zstring<> new_string {&(*m_blocks.back())[m_free_index]};
		std::memcpy(new_string, string.data(), string.length());
		m_free_index = gsl::narrow<decltype(m_free_index)>(end_index);
		return new_string;
	}
	else {
		m_free_index = 0;
		m_blocks.emplace_back(std::make_unique<block>());
		return add_string(string); // does the branch get_field optimized out?
	}
}