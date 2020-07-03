#include <vector>
#include <variant>
#include <gsl/gsl>
#include <iostream>
#include <filesystem>

namespace idlc {
	class string_heap {
	public:
		string_heap()
		{
			m_blocks.emplace_back(std::make_unique<block>());
		}

		string_heap(string_heap&) = delete;
		string_heap* operator=(string_heap&) = delete;

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
		static constexpr std::size_t block_size {1024}; // TODO: Tune
		using block = std::array<char, block_size>;

		std::vector<gsl::czstring<>> m_strings {};
		std::vector<std::unique_ptr<block>> m_blocks; // Allocate one free block
		std::uint16_t m_free_index {};

		gsl::czstring<> add_string(std::string_view string)
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
				return add_string(string); // does the branch get optimized out?
			}
		}
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

	constexpr primitive_type_kind to_unsigned(primitive_type_kind kind)
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

	template<typename type>
	std::unique_ptr<type>&& move_not_null(std::unique_ptr<type> obj)
	{
		Expects(obj != nullptr);
		return move(obj);
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
		projection_type(gsl::not_null<gsl::czstring<>> identifier) : m_identifier {identifier}
		{
		}

		gsl::czstring<> identifier() const
		{
			return m_identifier;
		}

	private:
		gsl::czstring<> m_identifier;
	};

	enum class copy_type_kind {
		primitive,
		projection
	};

	// A pointer type points to one of three: void, primitive, or projection
	// A return type is either void, primitive, projection, or pointer
	// And argument/field types ("annotated_types") are either primitive, projection, or pointer

	// Have to stop thinking of pointers as types
	// A "field" (arg, field, return slot, etc.) has a type, a set of annotations, and one or more stars
	// An rpc field has a signature, not a type
	// Eliminate pointer_type

	// What's the best place to check for non-nullnes? The constructor?

	class copy_type {
	public:
		template<typename type>
		copy_type(std::unique_ptr<type> obj) : m_variant {move(obj)}
		{
			Expects(std::get<std::unique_ptr<type>>(m_variant) != nullptr);
		}

		auto kind() const
		{
			return gsl::narrow_cast<copy_type_kind>(m_variant.index());
		}

		template<copy_type_kind kind>
		const auto& get() const
		{
			return *std::get<static_cast<std::size_t>(kind)>(m_variant);
		}

	private:
		std::variant<
			std::unique_ptr<primitive_type>,
			std::unique_ptr<projection_type>> m_variant;
	};

	class attributes {

	};

	class type {
	public:
		type(std::unique_ptr<copy_type> copy_type, std::unique_ptr<attributes> attributes, unsigned int stars) :
			m_copy_type {move(copy_type)},
			m_attributes {move(attributes)},
			m_stars {stars}
		{
		}

		const copy_type* get_copy_type() const
		{
			return m_copy_type.get();
		}

		const attributes* get_attributes() const
		{
			return m_attributes.get();
		}

		unsigned int stars() const
		{
			return m_stars;
		}

	private:
		std::unique_ptr<class copy_type> m_copy_type; // intuitively: copy_type [attributes] *
		std::unique_ptr<class attributes> m_attributes;
		unsigned int m_stars;
	};
	
	// We can speak of a pseudofield, in the sense that it has no identifier (identifier() returns nullptr)
	// These are used for return values

	class var_field {
	public:
		var_field(gsl::czstring<> identifier, std::unique_ptr<type> type) :
			m_identifier {identifier},
			m_type {move(type)}
		{
			Expects(m_type);
		}

		gsl::czstring<> identifier() const
		{
			return m_identifier;
		}

		const type& get_type() const
		{
			return *m_type;
		}

	private:
		gsl::czstring<> m_identifier;
		std::unique_ptr<class type> m_type;
	};

	class signature;

	class rpc_field {
	public:
		rpc_field(gsl::czstring<> identifier, std::unique_ptr<signature> sig) :
			m_identifier {identifier},
			m_signature {move(sig)}
		{
			Expects(m_signature != nullptr);
		}

		gsl::czstring<> identifier() const
		{
			return m_identifier;
		}

		const signature& get_signature() const
		{
			return *m_signature;
		}

	private:
		gsl::czstring<> m_identifier;
		std::unique_ptr<signature> m_signature;
	};

	enum class field_kind {
		var,
		rpc
	};

	class field {
	public:
		template<typename type>
		field(std::unique_ptr<type> obj) :
			m_variant {move(obj)}
		{
			Expects(std::get<std::unique_ptr<type>>(m_variant) != nullptr);
		}

		auto kind() const
		{
			return gsl::narrow_cast<field_kind>(m_variant.index());
		}

		template<field_kind kind>
		const auto& get() const
		{
			return *std::get<static_cast<std::size_t>(kind)>(m_variant);
		}

	private:
		std::variant<
			std::unique_ptr<var_field>,
			std::unique_ptr<rpc_field>> m_variant;
	};

	class signature {
	public:
		signature(std::unique_ptr<field> return_field, std::vector<std::unique_ptr<field>> arg_fields) :
			m_return_field {move(return_field)},
			m_arguments {move(arg_fields)}
		{
			Expects(m_return_field != nullptr);
			for (const auto& arg : m_arguments)
				Expects(arg != nullptr);
		}

		const field& return_field() const
		{
			return *m_return_field;
		}

		gsl::span<const std::unique_ptr<field>> arguments() const
		{
			return m_arguments;
		}

	private:
		std::unique_ptr<field> m_return_field;
		std::vector<std::unique_ptr<field>> m_arguments;
	};

	enum class module_item_kind {
		rpc,
		projection,
		require
	};

	class rpc {
	public:
		rpc(gsl::not_null<gsl::czstring<>> identifier, std::unique_ptr<signature> signature) :
			m_identifier {identifier},
			m_signature {move(signature)}
		{
			Expects(m_signature != nullptr);
		}

		gsl::czstring<> identifier() const
		{
			return m_identifier;
		}

		const signature& get_signature() const
		{
			return *m_signature;
		}

	private:
		gsl::czstring<> m_identifier;
		std::unique_ptr<signature> m_signature;
	};

	class projection {
	public:
		projection(
			gsl::not_null<gsl::czstring<>> identifier,
			gsl::not_null<gsl::czstring<>> real_type,
			std::vector<std::unique_ptr<field>> fields
		) :
			m_identifier {identifier},
			m_real_type {real_type},
			m_fields {move(fields)}
		{
			for (const auto& f : m_fields)
				Expects(f != nullptr);
		}

		gsl::czstring<> identifier() const
		{
			return m_identifier;
		}

		gsl::czstring<> real_type() const
		{
			return m_real_type;
		}

		gsl::span<const std::unique_ptr<field>> fields() const
		{
			return m_fields;
		}

	private:
		gsl::czstring<> m_identifier;
		gsl::czstring<> m_real_type;
		std::vector<std::unique_ptr<field>> m_fields;
	};

	class require {
	public:
		require(gsl::not_null<gsl::czstring<>> identifier) : m_identifier {identifier}
		{
		}

		gsl::czstring<> identifier() const
		{
			return m_identifier;
		}

	private:
		gsl::czstring<> m_identifier;
	};

	class module_item {
	public:
		template<typename type>
		module_item(std::unique_ptr<type> obj) : m_variant {move(obj)}
		{
			Expects(std::get<std::unique_ptr<type>>(m_variant) != nullptr);
		}

		auto kind() const
		{
			return gsl::narrow_cast<module_item_kind>(m_variant.index());
		}

		template<module_item_kind kind>
		const auto& get() const
		{
			return *std::get<static_cast<std::size_t>(kind)>(m_variant).get();
		}

	private:
		std::variant<
			std::unique_ptr<rpc>,
			std::unique_ptr<projection>,
			std::unique_ptr<require>> m_variant;
	};

	enum class file_item_kind {
		include,
		module
	};

	class module {
	public:
		module(gsl::not_null<gsl::czstring<>> identifier, std::vector<std::unique_ptr<module_item>> items) :
			m_identifier {identifier},
			m_items {move(items)}
		{
			for (const auto& i : m_items)
				Expects(i != nullptr);
		}

		gsl::czstring<> identifier() const
		{
			return m_identifier;
		}

		gsl::span<const std::unique_ptr<module_item>> items() const
		{
			return m_items;
		}

	private:
		gsl::czstring<> m_identifier;
		std::vector<std::unique_ptr<module_item>> m_items;
	};

	class include {
	public:
		include(std::filesystem::path& path) : m_path {path}
		{
		}

		std::filesystem::path path() const
		{
			return m_path;
		}

	private:
		std::filesystem::path m_path;
	};

	class file_item {
	public:
		template<typename type>
		file_item(std::unique_ptr<type> obj) : m_variant {move(obj)}
		{
			Expects(std::get<std::unique_ptr<type>>(m_variant) != nullptr);
		}

		auto kind() const
		{
			return gsl::narrow_cast<file_item_kind>(m_variant.index());
		}

		template<file_item_kind kind>
		const auto& get() const
		{
			return *std::get<static_cast<std::size_t>(kind)>(m_variant).get();
		}

	private:
		std::variant<
			std::unique_ptr<include>,
			std::unique_ptr<module>> m_variant;
	};

	class file {
	public:
		file(std::vector<std::unique_ptr<file_item>> modules) : m_items {move(modules)}
		{
			for (const auto& m : m_items)
				Expects(m != nullptr);
		}

		gsl::span<const std::unique_ptr<file_item>> items() const
		{
			return m_items;
		}
	
	private:
		std::vector<std::unique_ptr<file_item>> m_items;
	};
}