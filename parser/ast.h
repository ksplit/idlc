#ifndef _AST_H_
#define _AST_H_

#include <vector>
#include <variant>
#include <gsl/gsl>
#include <optional>

#include "../main/log.h"

#ifdef __GNUC__
#if __GNUC__ < 8
#include <experimental/filesystem>
namespace std {
	namespace filesystem = std::experimental::filesystem;
}
#else
#include <filesystem>
#endif
#else
#include <filesystem>
#endif

#include "../main/node_map.h"
#include "../main/string_heap.h"

namespace idlc {
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

	inline primitive_type_kind to_unsigned(primitive_type_kind kind) noexcept
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
		primitive_type(primitive_type_kind kind) noexcept : m_kind {kind}
		{
		}

		primitive_type_kind kind() const noexcept
		{
			return m_kind;
		}

	private:
		primitive_type_kind m_kind;
	};

	class projection;

	class projection_type {
	public:
		projection_type(gsl::not_null<gsl::czstring<>> identifier) noexcept :
			m_identifier {identifier},
			m_definition {nullptr}
		{
		}

		gsl::czstring<> identifier() const noexcept
		{
			return m_identifier;
		}

		projection& definition() const noexcept
		{
			Expects(m_definition != nullptr);
			return *m_definition;
		}

		void definition(projection* proj) noexcept
		{
			Expects(proj != nullptr);
			m_definition = proj;
		}

	private:
		gsl::czstring<> m_identifier;
		projection* m_definition;
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
	// An rpc field has a rpc_signature, not a type
	// Eliminate pointer_type

	// What's the best place to check for non-nullnes? The constructor?

	class copy_type {
	public:
		template<typename type>
		copy_type(std::unique_ptr<type>&& obj) : m_variant {move(obj)}
		{
			Expects(std::get<std::unique_ptr<type>>(m_variant) != nullptr);
		}

		auto kind() const noexcept
		{
			return gsl::narrow_cast<copy_type_kind>(m_variant.index());
		}

		template<copy_type_kind kind>
		const auto& get() const
		{
			return *std::get<static_cast<std::size_t>(kind)>(m_variant);
		}

		template<copy_type_kind kind>
		auto& get()
		{
			return *std::get<static_cast<std::size_t>(kind)>(m_variant);
		}

	private:
		std::variant<
			std::unique_ptr<primitive_type>,
			std::unique_ptr<projection_type>> m_variant;
	};

	enum class attribute_type : std::uint8_t {
		copy,
		alloc,
		dealloc,
		bind
	};

	// because why have a state machine in attributes() when you can just do bit-twiddling
	enum class copy_direction : std::uint8_t {
		none = 0b00,
		in = 0b01,
		out = 0b10,
		both = 0b11
	};

	// for copy attributes, these correspond to caller -> out, callee -> in
	enum class rpc_side : std::uint8_t {
		none,
		callee,
		caller,
		both
	};

	enum class sharing_op : std::uint8_t {
		alloc,
		dealloc,
		bind
	};

	struct compact_attribute {
		rpc_side share_op_side;
		attribute_type attribute;
	};

	inline bool operator==(compact_attribute a, compact_attribute b) noexcept
	{
		return a.share_op_side == b.share_op_side && a.attribute == b.attribute;
	}

	inline void* to_void_ptr(compact_attribute v) noexcept
	{
		void* tmp;
		std::memcpy(&tmp, &v, sizeof(v));
		return tmp;
	}

	inline compact_attribute from_void_ptr(const void* ptr) noexcept
	{
		compact_attribute v;
		std::memcpy(&v, &ptr, sizeof(v));
		return v;
	}

	static_assert(sizeof(compact_attribute) == 2);
	
	// An odd consequence of how apply() works: the list [alloc] specifies that the value is only allocated, but no value is shared
	class attributes {
	public:
		// NOTE: OFficially undefined
		attributes() = default;
		
		// Need to be able to "fail" construction, thus the factory
		static std::optional<attributes> make(const std::vector<compact_attribute>& attribs)
		{
			attributes tmp;
			tmp.m_value_copy = 0;
			if (tmp.apply(attribs)) {
				return tmp;
			}
			else {
				return std::nullopt;
			}
		}

		copy_direction get_value_copy_direction() const noexcept
		{
			return static_cast<copy_direction>(m_value_copy);
		}

		rpc_side get_sharing_op_side() const noexcept
		{
			return m_share_op_side;
		}

		sharing_op get_sharing_op() const noexcept
		{
			return m_share_op;
		}

	private:
		std::uint8_t m_value_copy {};
		rpc_side m_share_op_side {};
		sharing_op m_share_op {};


		bool apply(const std::vector<compact_attribute>& attribs)
		{
			for (const auto attr : attribs) {
				if (attr.attribute == attribute_type::copy) {
					update_value_copy(attr.share_op_side);
				}
				else {
					if (!update_sharing(attr)) {
						return false;
					}
				}
			}

			return true;
		}
		
		void update_value_copy(rpc_side side) noexcept
		{
			switch (side) {
			case rpc_side::caller:
				m_value_copy |= static_cast<std::uint8_t>(copy_direction::out);
				break;

			case rpc_side::callee:
				m_value_copy |= static_cast<std::uint8_t>(copy_direction::in);
				break;
			}
		}

		bool update_sharing(compact_attribute attr)
		{
			if (m_share_op_side != rpc_side::none) {
				log_error("Over-specified sharing ops");
				return false;
			}

			m_share_op_side = attr.share_op_side;
			switch (attr.attribute) {
			case attribute_type::alloc:
				m_share_op = sharing_op::alloc;
				break;

			case attribute_type::dealloc:
				m_share_op = sharing_op::dealloc;
				break;

			case attribute_type::bind:
				m_share_op = sharing_op::bind;
				break;
			}

			return true;
		}
	};

	class type {
	public:
		type(std::unique_ptr<copy_type>&& copy_type, std::unique_ptr<attributes>&& attributes, unsigned int stars) noexcept :
			m_copy_type {move(copy_type)},
			m_attributes {move(attributes)},
			m_stars {stars}
		{
		}

		const copy_type* get_copy_type() const noexcept
		{
			return m_copy_type.get();
		}

		copy_type* get_copy_type() noexcept
		{
			return m_copy_type.get();
		}

		const attributes* get_attributes() const noexcept
		{
			return m_attributes.get();
		}

		attributes* get_attributes() noexcept
		{
			return m_attributes.get();
		}

		unsigned int stars() const noexcept
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
		var_field(gsl::czstring<> identifier, std::unique_ptr<type>&& type) noexcept :
			m_identifier {identifier},
			m_type {move(type)}
		{
			Expects(m_type);
		}

		gsl::czstring<> identifier() const noexcept
		{
			return m_identifier;
		}

		const type& get_type() const noexcept
		{
			return *m_type;
		}

		type& get_type() noexcept
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
		gsl::czstring<> mangled_signature {};

		rpc_field(gsl::czstring<> identifier, std::unique_ptr<signature>&& sig, std::unique_ptr<attributes>&& attrs) noexcept :
			m_identifier {identifier},
			m_signature {move(sig)},
			m_attributes {move(attrs)}
		{
			Expects(m_signature != nullptr);
		}

		gsl::czstring<> identifier() const noexcept
		{
			return m_identifier;
		}

		const signature& get_signature() const noexcept
		{
			return *m_signature;
		}

		signature& get_signature() noexcept
		{
			return *m_signature;
		}

		const attributes* get_attributes() const noexcept
		{
			return m_attributes.get();
		}

		attributes* get_attributes() noexcept
		{
			return m_attributes.get();
		}

	private:
		gsl::czstring<> m_identifier;
		std::unique_ptr<signature> m_signature;
		std::unique_ptr<attributes> m_attributes;
	};

	enum class field_kind {
		var,
		rpc
	};

	class field {
	public:
		template<typename type>
		field(std::unique_ptr<type>&& obj) :
			m_variant {move(obj)}
		{
			Expects(std::get<std::unique_ptr<type>>(m_variant) != nullptr);
		}

		auto kind() const noexcept
		{
			return gsl::narrow_cast<field_kind>(m_variant.index());
		}

		template<field_kind kind>
		const auto& get() const
		{
			return *std::get<static_cast<std::size_t>(kind)>(m_variant);
		}

		template<field_kind kind>
		auto& get()
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
		signature(std::unique_ptr<field>&& return_field, std::vector<std::unique_ptr<field>>&& arg_fields) noexcept :
			m_return_field {move(return_field)},
			m_arguments {move(arg_fields)}
		{
			Expects(m_return_field != nullptr);
			for (const auto& arg : m_arguments)
				Expects(arg != nullptr);
		}

		const field& return_field() const noexcept
		{
			return *m_return_field;
		}

		field& return_field() noexcept
		{
			return *m_return_field;
		}

		gsl::span<const std::unique_ptr<field>> arguments() const noexcept
		{
			return m_arguments;
		}

		gsl::span<std::unique_ptr<field>> arguments() noexcept
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
		rpc(gsl::not_null<gsl::czstring<>> identifier, std::unique_ptr<signature>&& signature) :
			m_identifier {identifier},
			m_signature {move(signature)}
		{
			Expects(m_signature != nullptr);
		}

		gsl::czstring<> identifier() const noexcept
		{
			return m_identifier;
		}

		const signature& get_signature() const noexcept
		{
			return *m_signature;
		}

		signature& get_signature() noexcept
		{
			return *m_signature;
		}

	private:
		gsl::czstring<> m_identifier;
		std::unique_ptr<signature> m_signature;
	};

	class projection {
	public:
		gsl::czstring<> parent_module {};

		projection(
			gsl::not_null<gsl::czstring<>> identifier,
			gsl::not_null<gsl::czstring<>> real_type,
			std::vector<std::unique_ptr<field>>&& fields
		) :
			m_identifier {identifier},
			m_real_type {real_type},
			m_fields {move(fields)}
		{
			for (const auto& f : m_fields)
				Expects(f != nullptr);
		}

		gsl::czstring<> identifier() const noexcept
		{
			return m_identifier;
		}

		gsl::czstring<> real_type() const noexcept
		{
			return m_real_type;
		}

		gsl::span<const std::unique_ptr<field>> fields() const noexcept
		{
			return m_fields;
		}

		gsl::span<std::unique_ptr<field>> fields() noexcept
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

		gsl::czstring<> identifier() const noexcept
		{
			return m_identifier;
		}

	private:
		gsl::czstring<> m_identifier;
	};

	class module_item {
	public:
		template<typename type>
		module_item(std::unique_ptr<type>&& obj) : m_variant {move(obj)}
		{
			Expects(std::get<std::unique_ptr<type>>(m_variant) != nullptr);
		}

		auto kind() const noexcept
		{
			return gsl::narrow_cast<module_item_kind>(m_variant.index());
		}

		template<module_item_kind kind>
		const auto& get() const
		{
			return *std::get<static_cast<std::size_t>(kind)>(m_variant).get();
		}

		template<module_item_kind kind>
		auto& get()
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
		// May seem strange to "leave this out", but the contents are entirely up to the compiler passes
		node_map<projection> types;

		module(gsl::not_null<gsl::czstring<>> identifier, std::vector<std::unique_ptr<module_item>>&& items) :
			m_identifier {identifier},
			m_items {move(items)}
		{
			for (const auto& i : m_items)
				Expects(i != nullptr);
		}

		gsl::czstring<> identifier() const noexcept
		{
			return m_identifier;
		}

		gsl::span<const std::unique_ptr<module_item>> items() const noexcept
		{
			return m_items;
		}

		gsl::span<std::unique_ptr<module_item>> items() noexcept
		{
			return m_items;
		}

	private:
		gsl::czstring<> m_identifier;
		std::vector<std::unique_ptr<module_item>> m_items;
	};

	class file;

	class include {
	public:
		std::unique_ptr<file> parsed_file;

		include(const std::filesystem::path& path) : m_path {path}
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
		file_item(std::unique_ptr<type>&& obj) : m_variant {move(obj)}
		{
			Expects(std::get<std::unique_ptr<type>>(m_variant) != nullptr);
		}

		auto kind() const noexcept
		{
			return gsl::narrow_cast<file_item_kind>(m_variant.index());
		}

		template<file_item_kind kind>
		const auto& get() const
		{
			return *std::get<static_cast<std::size_t>(kind)>(m_variant).get();
		}

		template<file_item_kind kind>
		auto& get()
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
		file(std::vector<std::unique_ptr<file_item>>&& modules) noexcept :
			m_items {move(modules)}
		{
			for (const auto& m : m_items)
				Expects(m != nullptr);
		}

		gsl::span<const std::unique_ptr<file_item>> items() const noexcept
		{
			return m_items;
		}

		gsl::span<std::unique_ptr<file_item>> items() noexcept
		{
			return m_items;
		}
	
	private:
		std::vector<std::unique_ptr<file_item>> m_items;
	};
}

#endif