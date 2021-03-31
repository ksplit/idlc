#ifndef IDLC_TAG_TYPES_H
#define IDLC_TAG_TYPES_H

#include <cstdint>

#include "../string_heap.h"

// These types are used in both the pgraph and ast

namespace idlc {
	// TODO: clean this up?
	// TODO: I'd prefer strong-typing the val / pointer distinction
	// TODO: bitfield isn't really the sanest choice
	enum class annotation_kind {
		alloc_caller	= 0b000000000001,
		alloc_callee	= 0b000000000010,
		dealloc_caller	= 0b000000000100,
		dealloc_callee	= 0b000000001000,
		bind_caller		= 0b000000010000,
		bind_callee		= 0b000000100000,
		in				= 0b000001000000,
		out				= 0b000010000000,
		ioremap_caller	= 0b000100000000,
		ioremap_callee	= 0b001000000000,
		shared			= 0b010000000000,
		is_bind			= (bind_callee | bind_caller),
		is_dealloc		= (dealloc_callee | dealloc_caller),
		is_alloc		= (alloc_callee | alloc_caller),
		is_ioremap		= (ioremap_callee | ioremap_caller),
		is_ptr			= is_bind | is_dealloc | is_alloc | is_ioremap | shared,
		is_val			= (out | in),
		ptr_only		= is_ptr,
		val_only		= is_val,
		use_default		= 0b000000000
	};

	struct annotation {
		annotation_kind kind;
		ident share_global;
	};

	constexpr auto operator|(annotation_kind a, annotation_kind b)
	{
		return static_cast<annotation_kind>(static_cast<std::uintptr_t>(a) | static_cast<std::uintptr_t>(b));
	}

	constexpr auto& operator|=(annotation_kind& a, annotation_kind b)
	{
		a = a | b;
		return a;
	}

	constexpr auto operator&(annotation_kind a, annotation_kind b)
	{
		return static_cast<annotation_kind>(static_cast<std::uintptr_t>(a) & static_cast<std::uintptr_t>(b));
	}

	constexpr auto operator~(annotation_kind a)
	{
		return static_cast<annotation_kind>(~static_cast<std::uintptr_t>(a));
	}

	constexpr auto is_clear(annotation_kind a)
	{
		return static_cast<std::uintptr_t>(a) == 0;
	}

	constexpr auto flags_set(annotation_kind field, annotation_kind flags)
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
