#ifndef _LCDS_IDL_PARSER_AST_H_
#define _LCDS_IDL_PARSER_AST_H_

#include <memory>
#include <vector>
#include <variant>

#include <gsl/gsl>

namespace idlc::ast {
	// TODO: wrap the "node-like" vectors?

	template<typename type>
	using node_ptr = std::shared_ptr<type>;
	
	template<typename type>
	using node_ref = gsl::not_null<node_ptr<type>>;

	template<typename type>
	using ptr_vec = std::vector<node_ptr<type>>;

	template<typename type>
	using ref_vec = std::vector<node_ref<type>>;

	using ident = gsl::czstring<>;
	using ident_vec = std::vector<ident>;

	struct module_def;
	struct driver_def;
	struct driver_file;
	enum class tyname_arith;
	struct tyname_rpc;
	struct tyname_proj;
	struct tyname_array;
	struct tyname_any_of_ptr;
	struct tyname_string;
	struct tyname;

	struct naked_proj_decl;
	struct var_decl;
	
	struct rpc_def;
	
	// struct field_rel_ref;
	// struct field_abs_ref;

	// TODO: can this be removed?
	struct tok_kw_null {}; // Doesn't exist in parse rules, used as marker (represents tok_kw_null)
	
	using file = std::variant<node_ref<driver_file>, node_ref<std::vector<node_ref<module_def>>>>;
	// using field_ref = std::variant<node_ref<field_abs_ref>, node_ref<field_rel_ref>>;
	using array_size = std::variant<unsigned, tok_kw_null, ident>;
	using proj_field = std::variant<node_ref<var_decl>, node_ref<naked_proj_decl>>;
	using tyname_stem = std::variant<
		tyname_arith,
		tyname_string,
		node_ref<tyname_rpc>,
		node_ref<tyname_proj>,
		node_ref<tyname_array>,
		node_ref<tyname_any_of_ptr>
	>;

	// TODO: clean this up?
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
		is_ptr			= is_bind | is_dealloc | is_alloc,
		is_val			= out | in,
		is_set			= 0b100000000,
		use_default		= 0b000000000, // will not set the is_set flag, thus ensuring it will be defaulted
	};

	inline auto& operator|=(tags& a, tags b) {
		auto val = static_cast<std::uintptr_t>(a);
		val |= static_cast<std::uintptr_t>(b);
		a = static_cast<tags>(val);
		return a;
	}

	inline auto operator&(tags a, tags b) {
		auto val = static_cast<std::uintptr_t>(a);
		val &= static_cast<std::uintptr_t>(b);
		return static_cast<tags>(val);
	}

	struct driver_def {
		ident name;
		node_ptr<ident_vec> imports;

		driver_def(ident name, node_ptr<ident_vec> imports) :
			name {name},
			imports {imports}
		{}
	};

	struct driver_file {
		node_ptr<ident_vec> former;
		node_ref<driver_def> driver;
		node_ptr<ident_vec> latter;

		driver_file(node_ptr<ident_vec> former,
			node_ref<driver_def> driver,
			node_ptr<ident_vec> latter
		) :
			former {former},
			driver {driver},
			latter {latter}
		{}
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
		ident name;

		tyname_proj(ident name) :
			name {name}
		{}
	};

	struct tyname_rpc {
		ident name;

		tyname_rpc(ident name) :
			name {name}
		{}
	};

	struct tyname_array {
		node_ref<tyname> element;
		node_ref<array_size> size;

		tyname_array(node_ref<tyname> element, node_ref<array_size> size) :
			element {element},
			size {size}
		{}
	};

	struct tyname_any_of_ptr {
		ident discrim;
		node_ref<ref_vec<tyname>> types;

		tyname_any_of_ptr(
			ident discrim,
			node_ref<ref_vec<tyname>> types
		) :
			discrim {discrim},
			types {types}
		{}
	};

	// struct field_abs_ref {
	// 	node_ref<field_rel_ref> link;
	// };

	// struct field_rel_ref {
	// 	idents links;
	// };

	// Another marker
	// NOTE: since these don't actually exist in any meaningful sense, their own parse rules don't produce them
	struct tyname_string {};

	struct indirection {
		tags attrs; // Contextually, both ptr and value attrs
		bool is_const;

		indirection(tags attrs, bool is_const) :
			attrs {attrs},
			is_const {is_const}
		{}
	};

	// FIXME: field order
	struct tyname {
		bool is_const;
		node_ref<tyname_stem> stem;
		std::vector<node_ref<indirection>> indirs;
		tags attrs; // Will only ever have value attrs in it

		tyname(
			bool is_const,
			node_ref<tyname_stem> stem,
			std::vector<node_ref<indirection>> indirs,
			tags attrs
		) :
			is_const {is_const},
			stem {stem},
			indirs {indirs},
			attrs {attrs}
		{}
	};

	struct var_decl {
		node_ref<tyname> type;
		ident name;

		var_decl(node_ref<tyname> type, ident name) :
			type {type},
			name {name}
		{}
	};

	struct naked_proj_decl {
		node_ptr<std::vector<node_ref<proj_field>>> fields;
		ident name;

		naked_proj_decl(node_ptr<std::vector<node_ref<proj_field>>> fields, ident name) :
			fields {fields},
			name {name}
		{}
	};

	enum proj_def_kind {
		struct_kind,
		union_kind
	};

	struct proj_def {
		ident name;
		ident type;
		node_ptr<std::vector<node_ref<proj_field>>> fields;
		proj_def_kind kind;

		proj_def(
			ident name,
			ident type,
			node_ptr<std::vector<node_ref<proj_field>>> fields,
			proj_def_kind kind
		) :
			name {name},
			type {type},
			fields {fields},
			kind {kind}
		{}
	};

	using rpc_item = std::variant<node_ref<proj_def>, node_ref<rpc_def>>;

	enum rpc_def_kind {
		direct,
		indirect
	};

	struct rpc_def {
		node_ptr<tyname> ret_type; // null type is used for <void>
		ident name;
		node_ptr<std::vector<node_ref<var_decl>>> arguments;
		node_ptr<std::vector<node_ref<rpc_item>>> items;
		rpc_def_kind kind;

		rpc_def(
			node_ptr<tyname> ret_type,
			ident name,
			node_ptr<std::vector<node_ref<var_decl>>> arguments,
			node_ptr<std::vector<node_ref<rpc_item>>> items,
			rpc_def_kind kind
		) :
			ret_type {ret_type},
			name {name},
			arguments {arguments},
			items {items},
			kind {kind}
		{}
	};

	// TODO: impl
	struct header_stmt {};

	using module_item = std::variant<
		header_stmt,
		node_ref<proj_def>,
		node_ref<rpc_def>
	>;

	struct module_def {
		ident name;
		node_ptr<std::vector<node_ref<module_item>>> items;

		module_def(ident name, node_ptr<std::vector<node_ref<module_item>>> items) :
			name {name},
			items {items}
		{}
	};
}

#endif
