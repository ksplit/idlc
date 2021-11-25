#ifndef _STRING_HEAP_H_
#define _STRING_HEAP_H_

#include <gsl/gsl>

#include <absl/strings/string_view.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace idlc {
	using ident = gsl::czstring<>;

	class string_heap {
	public:
		string_heap() noexcept { m_blocks.emplace_back(std::make_unique<block>()); }

		string_heap(string_heap&) = delete;
		string_heap* operator=(string_heap&) = delete;

		ident intern(absl::string_view string);

	private:
		static constexpr std::size_t block_size {1024}; // NOTE: you can tune this to reduce allocation pressure
		using block = std::array<char, block_size>;

		std::vector<gsl::czstring<>> m_strings {};
		std::vector<std::unique_ptr<block>> m_blocks; // Allocate one free block
		std::uint16_t m_free_index {};

		gsl::czstring<> add_string(absl::string_view string);
	};
}

#endif // !_STRING_HEAP_H_