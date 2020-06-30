#include <vector>
#include <variant>
#include <gsl/gsl>

namespace idlc {
	class string_heap {
	public:
		~string_heap()
		{
			for (const auto block_ptr : m_blocks) {
				delete block_ptr;
			}
		}

		gsl::czstring<> intern(std::string_view string)
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

		// Convenience method for transferring blocks between string heaps
		template<std::size_t size>
		void intern_all(std::array<gsl::czstring<>, size>& strings)
		{
			for (const auto& string : strings) {
				string = intern(string);
			}
		}

	private:
		static constexpr std::size_t block_size {4096}; // TODO: Tune
		using block = std::array<char, block_size>;

		std::vector<gsl::czstring<>> m_strings {};
		std::vector<block*> m_blocks {new block {}}; // Allocate one free block
		std::uint16_t m_free_index {};

		gsl::czstring<> add_string(std::string_view string)
		{
			const auto end_index = m_free_index + string.length() + 1;
			if (end_index < block_size) {
				// Since the block is zero-initialized, we just copy the string bytes
				const gsl::zstring<> new_string {&(*m_blocks.back())[m_free_index]};
				std::memcpy(new_string, string.data(), string.length());
				m_free_index = end_index;
				return new_string;
			}
			else {
				m_free_index = 0;
				m_blocks.push_back(new block {});
				return add_string(string); // does the branch get optimized out?
			}
		}
	};

	enum class value_type_kind {
		primitive,
		projection
	};

	enum class primitive_type_kind {
		bool_k,
		float_k,
		double_k,
		char_k,
		short_k,
		int_k,
		long_k,
		long_long_k,
		unsigned_char_k,
		unsigned_short_k,
		unsigned_int_k,
		unsigned_long_k,
		unsigned_long_long_k
	};

	inline primitive_type_kind to_unsigned(primitive_type_kind kind)
	{
		switch (kind) {
		case primitive_type_kind::char_k:
			return primitive_type_kind::unsigned_char_k;

		case primitive_type_kind::short_k:
			return primitive_type_kind::unsigned_short_k;

		case primitive_type_kind::int_k:
			return primitive_type_kind::unsigned_int_k;

		case primitive_type_kind::long_k:
			return primitive_type_kind::unsigned_long_k;

		case primitive_type_kind::long_long_k:
			return primitive_type_kind::unsigned_long_long_k;

		default:
			Expects(false);
		}
	}

	class primitive_type {
	public:
		primitive_type(primitive_type_kind kind) : m_kind {kind}
		{
		}

		primitive_type_kind kind() const
		{
			return m_kind;
		}

	private:
		primitive_type_kind m_kind;
	};

	class projection_type {
	public:
		projection_type(gsl::czstring<> identifier) : m_identifier {identifier}
		{
		}

		gsl::czstring<> identifier() const
		{
			return m_identifier;
		}

	private:
		gsl::czstring<> m_identifier;
	};

	class value_type {
	public:
		template<typename type>
		value_type(std::unique_ptr<type>&& obj) : m_variant {move(obj)}
		{
		}

		value_type_kind kind() const
		{
			return static_cast<value_type_kind>(m_variant.index());
		}

		template<value_type_kind kind>
		const auto& get() const
		{
			return std::get<static_cast<std::size_t>(kind)>(m_variant);
		}

	private:
		std::variant<std::unique_ptr<primitive_type>, std::unique_ptr<projection_type>> m_variant;
	};

	class void_type {
	public:
		void_type() = delete; // Please don't instantiate this. It's a marker object, just use nullptr
	};

	enum class pointable_type_kind {
		void_k,
		value_type
	};

	class pointable_type {
	public:
		template<typename type>
		pointable_type(std::unique_ptr<type>&& obj) : m_variant {move(obj)}
		{
		}

		pointable_type_kind kind() const
		{
			return static_cast<pointable_type_kind>(m_variant.index());
		}

		template<pointable_type_kind kind>
		const auto& get() const
		{
			return std::get<static_cast<std::size_t>(kind)>(m_variant);
		}

	private:
		std::variant<std::unique_ptr<void_type>, std::unique_ptr<value_type>> m_variant;
	};

	class pointer_type {
	public:
		pointer_type(std::unique_ptr<pointable_type>&& real_type) : m_real_type {move(real_type)}
		{
		}

		const pointable_type* real_type() const
		{
			return m_real_type.get();
		}

	private:
		std::unique_ptr<pointable_type> m_real_type;
	};

	enum class annotated_type_kind {
		pointer_type,
		value_type
	};

	class annotated_type {
	public:
		template<typename type>
		annotated_type(std::unique_ptr<type>&& obj) : m_variant {move(obj)}
		{
		}

		annotated_type_kind kind() const
		{
			return static_cast<annotated_type_kind>(m_variant.index());
		}

		template<annotated_type_kind kind>
		const auto& get() const
		{
			return std::get<static_cast<std::size_t>(kind)>(m_variant);
		}

	private:
		std::variant<std::unique_ptr<pointer_type>, std::unique_ptr<value_type>> m_variant;
	};

	enum class return_type_kind {
		void_k,
		annotated_type
	};

	class return_type {
	public:
		template<typename type>
		return_type(std::unique_ptr<type>&& obj) : m_variant {move(obj)}
		{
		}

		return_type_kind kind() const
		{
			return static_cast<return_type_kind>(m_variant.index());
		}

		template<pointable_type_kind kind>
		const auto& get() const
		{
			return std::get<static_cast<std::size_t>(kind)>(m_variant);
		}

	private:
		std::variant<std::unique_ptr<void_type>, std::unique_ptr<annotated_type>> m_variant;
	};
	
	class projection_field {
	public:
		projection_field(gsl::czstring<> identifier) : m_identifier {identifier}
		{
		}

		gsl::czstring<> identifier()
		{
			return m_identifier;
		}

	private:
		gsl::czstring<> m_identifier;
	};

	enum class item_kind {
		rpc,
		projection
	};

	class rpc {
	public:
		rpc(gsl::czstring<> identifier) :
			m_identifier {identifier}
		{}

		gsl::czstring<> identifier() const
		{
			return m_identifier;
		}

	private:
		gsl::czstring<> m_identifier;
	};

	class projection {
	public:
		projection(gsl::czstring<> identifier, gsl::czstring<> real_type) :
			m_identifier {identifier},
			m_real_type {real_type}
		{
		}

		gsl::czstring<> identifier() const
		{
			return m_identifier;
		}

		gsl::czstring<> real_type() const
		{
			return m_real_type;
		}

		gsl::span<const std::unique_ptr<projection_field>> fields() const
		{
			return m_fields;
		}

	private:
		gsl::czstring<> m_identifier;
		gsl::czstring<> m_real_type;
		std::vector<std::unique_ptr<projection_field>> m_fields;
	};

	class item {
	public:
		template<typename type>
		item(std::unique_ptr<type>&& obj) : m_variant {move(obj)}
		{
		}

		item_kind kind() const
		{
			return static_cast<item_kind>(m_variant.index());
		}

		template<item_kind kind>
		const auto& get() const
		{
			return std::get<static_cast<std::size_t>(kind)>(m_variant);
		}

	private:
		std::variant<std::unique_ptr<rpc>, std::unique_ptr<projection>> m_variant;
	};

	class module {
	public:
		module(gsl::czstring<> identifier, std::vector<std::unique_ptr<item>>&& items) :
			m_identifier {identifier},
			m_items {move(items)}
		{
		}

		gsl::czstring<> identifier() const
		{
			return m_identifier;
		}

		gsl::span<const std::unique_ptr<item>> items() const
		{
			return m_items;
		}

	private:
		gsl::czstring<> m_identifier;
		std::vector<std::unique_ptr<item>> m_items;
	};

	class file {
	public:
		file(std::vector<std::unique_ptr<module>>&& modules) : m_modules {move(modules)}
		{
		}

		gsl::span<const std::unique_ptr<module>> modules()
		{
			return m_modules;
		}
	
	private:
		std::vector<std::unique_ptr<module>> m_modules;
	};
}