#ifndef _STRING_HEAP_H_
#define _STRING_HEAP_H_

#include <gsl/gsl>
#include <cstdint>
#include <vector>
#include <memory>

namespace idlc {
	class string_heap {
	public:
		string_heap() noexcept
		{
			m_blocks.emplace_back(std::make_unique<block>());
		}

		string_heap(string_heap&) = delete;
		string_heap* operator=(string_heap&) = delete;

		gsl::czstring<> intern(std::string_view string);

	private:
		static constexpr std::size_t block_size {1024}; // TODO: Tune
		using block = std::array<char, block_size>;

		std::vector<gsl::czstring<>> m_strings {};
		std::vector<std::unique_ptr<block>> m_blocks; // Allocate one free block
		std::uint16_t m_free_index {};

		gsl::czstring<> add_string(std::string_view string);
	};
}

#endif // !_STRING_HEAP_H_