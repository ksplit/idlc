#ifndef IDLC_TAG_TYPES_H
#define IDLC_TAG_TYPES_H

#include <cstdint>

// These types are used in both the pgraph and ast

namespace idlc {
	// TODO: clean this up?
	// TODO: I'd prefer strong-typing the val / pointer distinction
	enum class annotation {
		alloc_caller	= 0b100000001,
		alloc_callee	= 0b100000010,
		dealloc_caller	= 0b100000100,
		dealloc_callee	= 0b100001000,
		bind_caller		= 0b100010000,
		bind_callee		= 0b100100000,
		in				= 0b101000000,
		out				= 0b110000000,
		is_bind			= 0b100110000,
		is_dealloc		= 0b100001100,
		is_alloc		= 0b100000011,
		is_set			= 0b100000000,
		is_ptr			= is_bind | is_dealloc | is_alloc,
		is_val			= (out | in),
		ptr_only		= is_ptr & (~is_set),
		val_only		= is_val & (~is_set),
		use_default		= 0b000000000, // will not set the is_set flag, thus ensuring it will be defaulted
	};

	constexpr auto operator|(annotation a, annotation b)
	{
		return static_cast<annotation>(static_cast<std::uintptr_t>(a) | static_cast<std::uintptr_t>(b));
	}

	constexpr auto& operator|=(annotation& a, annotation b)
	{
		a = a | b;
		return a;
	}

	constexpr auto operator&(annotation a, annotation b)
	{
		return static_cast<annotation>(static_cast<std::uintptr_t>(a) & static_cast<std::uintptr_t>(b));
	}

	constexpr auto operator~(annotation a)
	{
		return static_cast<annotation>(~static_cast<std::uintptr_t>(a));
	}

	constexpr auto is_clear(annotation a)
	{
		return static_cast<std::uintptr_t>(a) == 0;
	}

	constexpr auto flags_set(annotation field, annotation flags)
	{
		return (field & flags) == flags;
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