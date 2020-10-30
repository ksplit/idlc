#pragma once

#include <cassert>
#include <memory>
#include <variant>

#include <gsl/gsl>

/*
	How much lowering should the parser even be doing?
*/

namespace idlc::ast {
    struct node_driver_file;
	struct node_module_file;
	struct node_include_stmt;
	struct node_driver_def;
	struct node_import_stmt;

	using file_node = std::variant<node_driver_file*>;

	struct node_driver_file {
		gsl::span<node_include_stmt*> includes;
		node_driver_def* driver_def;
	};

	struct node_include_stmt {
		std::string_view path;
	};

	struct node_driver_def {
		gsl::span<node_import_stmt*> imports;
	};

	// If all the AST types are trivially destructible, we can slap them onto a stack and just drop the whole thing
	// when we're done
	static_assert(std::is_trivially_destructible_v<std::string_view>);
	static_assert(std::is_trivially_destructible_v<gsl::span<int>>);

	struct node_import_stmt {
		gsl::czstring<> ident;
	};
}

namespace idlc {
	class stack_memory {
	public:
		template<typename type>
		type* allocate()
		{
			static_assert(std::is_trivially_destructible_v<type>);
			const auto block = alloc_aligned<type>(1);
			return new (block.data()) type {};
		}

		template<typename type>
		gsl::span<type> allocate(std::uint64_t n)
		{
			static_assert(std::is_trivially_destructible_v<type>);
			// FIXME: requires std::launder

			Expects(n > 0);
			const auto block = alloc_aligned<type>(n);
			const auto first = reinterpret_cast<type*>(block.data());
			const auto last = std::uninitialized_value_construct_n(first, n);
			return {first, last};
		}

		template<typename type>
		gsl::span<type> extend(gsl::span<type> block, std::uint64_t n)
		{
			static_assert(std::is_trivially_destructible_v<type>);
			const auto last = &gsl::as_bytes(block).last();
			const auto base = gsl::as_writable_bytes(gsl::span {&storage_, 1}).data();
			assert(std::distance(base, last) == top_);
			const auto extension = allocate<type>(n);
			return {block.data(), block.size() + n};
		}

	private:
		static constexpr auto capacity {1'000'000};
		std::aligned_storage_t<capacity> storage_ {};
		std::uint64_t top_ {};

		template<typename type>
		gsl::span<std::byte> alloc_aligned(std::uint64_t count)
		{
			const auto align = alignof(type);
			const auto size = sizeof(type) * count;

			const auto padding = align - (top_ % align);
			const auto offset = padding + size;
			const auto c_top = top_;

			assert(c_top + offset < capacity);
			const auto base = c_top + padding;
			
			const auto bytes = gsl::as_writable_bytes(gsl::span {&storage_, 1});
			const auto block = bytes.subspan(base, size);

			top_ += offset;
			
			return block;
		}
	};
}
