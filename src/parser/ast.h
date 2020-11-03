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

	struct null_array_size; // Doesn't exist in parse rules
	
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

	struct driver_def {
		gsl::czstring<> name;
		std::shared_ptr<std::vector<gsl::czstring<>>> imports;
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

	struct tyname {
		node_ref<tyname_stem> stem;
	};
}

#endif
