#ifndef _LCDS_IDL_PARSER_AST_H_
#define _LCDS_IDL_PARSER_AST_H_

#include <memory>
#include <vector>
#include <variant>

#include <gsl/gsl>

namespace idlc::parser {
	// TODO: wrap the "node-like" vectors?

	template<typename type>
	using node_ptr = std::shared_ptr<type>;
	
	template<typename type>
	using node_ref = gsl::not_null<node_ptr<type>>;

	struct driver_def;
	struct driver_file;
	enum class tyname_arith;
	struct tyname_rpc;
	struct tyname_proj;
	struct tyname_array;
	struct tyname_any_of_ptr;
	struct tyname_string;
	struct tyname;
	
	struct field_rel_ref;
	struct field_abs_ref;

	struct null_array_size; // Doesn't exist in parse rules, used as marker (represents tok_kw_null)
	
	using file = std::variant<node_ref<driver_file>>;
	using field_ref = std::variant<node_ref<field_abs_ref>, node_ref<field_rel_ref>>;
	using array_size = std::variant<unsigned, null_array_size, node_ref<field_ref>>;
	using tyname_stem = std::variant<
		tyname_arith,
		tyname_string,
		node_ref<tyname_rpc>,
		node_ref<tyname_proj>,
		node_ref<tyname_array>,
		node_ref<tyname_any_of_ptr>
	>;

	// TODO: class this and give operators
	enum class tags {
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
		is_set			= 0b100000000
	};

	inline auto& operator|=(tags& a, tags b) {
		auto val = static_cast<std::uintptr_t>(a);
		val |= static_cast<std::uintptr_t>(b);
		a = static_cast<tags>(val);
		return a;
	}

	struct driver_def {
		gsl::czstring<> name;
		node_ptr<std::vector<gsl::czstring<>>> imports;
	};

	struct driver_file {
		node_ptr<std::vector<gsl::czstring<>>> former;
		node_ref<driver_def> driver;
		node_ptr<std::vector<gsl::czstring<>>> latter;
	};

	enum class tyname_arith {
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

	struct tyname_proj {
		gsl::czstring<> name;
	};

	struct tyname_rpc {
		gsl::czstring<> name;
	};

	struct tyname_array {
		node_ref<tyname> element;
		node_ref<array_size> size;
	};

	struct tyname_any_of_ptr {
		node_ref<field_ref> discrim;
		node_ref<std::vector<node_ref<tyname>>> types;
	};

	// Marker, subject to removal if I have a better way of representing it in-tree
	struct null_array_size {};

	struct field_abs_ref {
		node_ref<field_rel_ref> link;
	};

	struct field_rel_ref {
		std::vector<gsl::czstring<>> links;
	};

	// Another marker
	// NOTE: since these don't actually exist in any meaningful sense, their own parse rules don't produce them
	struct tyname_string {};

	struct indirection {
		tags attrs; // Contectually, both ptr and value attrs
		bool is_const;
	};

	struct tyname {
		bool is_const;
		node_ref<tyname_stem> stem;
		std::vector<node_ref<indirection>> indirs;
		tags attrs; // Will only ever have value attrs in it
	};

	struct var_decl {
		node_ref<tyname> type;
		gsl::czstring<> name;
	};
}

#endif
