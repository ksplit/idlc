#ifndef IDLC_TAG_TYPES_H
#define IDLC_TAG_TYPES_H

#include <cstdint>

#include "../string_heap.h"

// These types are used in both the pgraph and ast

namespace idlc {
	// TODO: clean this up?
	// TODO: I'd prefer strong-typing the val / pointer distinction
	// TODO: bitfield isn't really the sanest choice
	// FIXME: the entire annotation set system needs an overhaul to better encode caller/callee, mutually exclusive
	// annotations, among other things
	enum class annotation_bitfield {
		use_default,
		alloc_caller = 1 << 0,
		alloc_callee = 1 << 1,
		dealloc_caller = 1 << 2,
		dealloc_callee = 1 << 3,
		bind_caller = 1 << 4,
		bind_callee = 1 << 5,
		in = 1 << 6,
		out = 1 << 7,
		ioremap_caller = 1 << 8,
		ioremap_callee = 1 << 9,
		alloc_once_caller = 1 << 10,
		alloc_once_callee = 1 << 11,
		shared = 1 << 12,
		unused = 1 << 13,
		bind_memberof_caller = 1 << 14,
		bind_memberof_callee = 1 << 15,
		alloc_stack_caller = 1 << 16,
		alloc_stack_callee = 1 << 17,
		user_ptr = 1 << 18,
		within_ptr = 1 << 19,
		err_ptr = 1 << 20,
		is_bind_memberof = bind_memberof_caller | bind_memberof_callee,
		is_bind = (bind_callee | bind_caller),
		is_dealloc = (dealloc_callee | dealloc_caller),
		is_alloc = (alloc_callee | alloc_caller),
		is_alloc_once = (alloc_once_callee | alloc_once_caller),
		is_alloc_stack = (alloc_stack_callee | alloc_stack_caller),
		is_ioremap = (ioremap_callee | ioremap_caller),
		val_only = out | in | unused,
		ptr_only = ~val_only,
		in_out = out | in,
		io_only = in_out,
	};

	struct bind_memberof {
		ident struct_type {};
		ident field {};

		bind_memberof() = default;

		bind_memberof(ident type, ident field) : struct_type {type}, field {field} {}

		operator bool() const { return field || struct_type; }
	};

	inline bool operator==(const bind_memberof& a, const bind_memberof& b)
	{
		return a.struct_type == b.struct_type && a.field == b.field;
	}

	template <typename type>
	class set_once {
	public:
		set_once() = default;

		set_once(const type& item) : m_item {item} {}

		set_once& operator=(const type& item)
		{
			if (m_item)
				assert(m_item == item || !item);
			else
				m_item = item;

			return *this;
		}

		set_once& operator=(const set_once& item)
		{
			*this = item.m_item;
			return *this;
		}

		auto& get() { return m_item; };
		const auto& get() const { return m_item; };

	private:
		type m_item {};
	};

	// TODO: clean me up for the love of all that is good and green on this earth
	// Semantically, some of these fields are of suspect meaning
	// TODO: remove the notion of verbatims, they were always a workaround
	struct annotation_set {
		annotation_bitfield kind {};
		set_once<ident> share_global {};
		set_once<ident> size_verbatim {}; // this really shouldn't be an ident
		set_once<ident> flags_verbatim {}; // this really shouldn't be an ident ;)
		set_once<bind_memberof> member {};
		set_once<ident> parent_pointer {};
	};

	constexpr auto operator|(annotation_bitfield a, annotation_bitfield b)
	{
		return static_cast<annotation_bitfield>(static_cast<std::uintptr_t>(a) | static_cast<std::uintptr_t>(b));
	}

	constexpr auto& operator|=(annotation_bitfield& a, annotation_bitfield b)
	{
		a = a | b;
		return a;
	}

	constexpr auto operator&(annotation_bitfield a, annotation_bitfield b)
	{
		return static_cast<annotation_bitfield>(static_cast<std::uintptr_t>(a) & static_cast<std::uintptr_t>(b));
	}

	constexpr auto& operator&=(annotation_bitfield& a, annotation_bitfield b)
	{
		a = a & b;
		return a;
	}

	constexpr auto operator~(annotation_bitfield a)
	{
		return static_cast<annotation_bitfield>(~static_cast<std::uintptr_t>(a));
	}

	constexpr auto is_clear(annotation_bitfield a) { return static_cast<std::uintptr_t>(a) == 0; }

	constexpr auto flags_set(annotation_bitfield field, annotation_bitfield flags) { return (field & flags) == flags; }

	inline annotation_set& operator&=(annotation_set& a, const annotation_set& b) noexcept
	{
		const auto flags = a.kind | b.kind;
		a = b;
		a.kind = flags;
		return a;
	}

	enum class type_primitive {
		ty_invalid,
		ty_bool,
		ty_char,
		ty_schar,
		ty_uchar,
		ty_short,
		ty_ushort,
		ty_int,
		ty_uint,
		ty_long,
		ty_ulong,
		ty_llong,
		ty_ullong
	};
}

#endif
