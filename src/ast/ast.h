#ifndef IDLC_AST_AST_H_
#define IDLC_AST_AST_H_

#include <memory>
#include <vector>
#include <variant>

#include <gsl/gsl>

#include "../parser/string_heap.h"
#include "../sema/scope.h"

#include "../tag_types.h"

 // TODO: move me
 // TODO: since AST uses "external types" extensively, re-organize these to avoid vicious circularities
namespace idlc::sema {
	struct projection;
	struct data_field;
}

namespace idlc::ast {
	template<typename type>
	using node_ptr = std::shared_ptr<type>;
	
	template<typename type>
	using node_ref = gsl::not_null<node_ptr<type>>;

	template<typename type>
	using ptr_vec = std::vector<node_ptr<type>>;

	template<typename type>
	using ref_vec = std::vector<node_ref<type>>;

	using ident_vec = std::vector<ident>;

	struct module_def;
	struct driver_def;
	struct driver_file;
	struct type_rpc;
	struct type_proj;
	struct type_array;
	struct type_any_of;
	struct type_string;
	struct type_spec;

	struct naked_proj_decl;
	struct var_decl;
	
	struct rpc_def;
	
	// Re-export these from the AST
	using idlc::annotation;
	using idlc::type_primitive;

	// struct field_rel_ref;
	// struct field_abs_ref;

	// TODO: can this be removed?
	struct tok_kw_null {}; // Doesn't exist in parse rules, used as marker (represents tok_kw_null)
	
	using file = std::variant<node_ref<driver_file>, node_ref<ref_vec<module_def>>>;
	// using field_ref = std::variant<node_ref<field_abs_ref>, node_ref<field_rel_ref>>;
	using array_size = std::variant<unsigned, tok_kw_null, ident>;
	using proj_field = std::variant<node_ref<var_decl>, node_ref<naked_proj_decl>>;
	using type_stem = std::variant<
		type_primitive,
		type_string,
		node_ref<type_rpc>,
		node_ref<type_proj>,
		node_ref<type_array>,
		node_ref<type_any_of>
	>;

	struct driver_def {
		const ident name;
		const node_ptr<ident_vec> imports;

		driver_def(ident name, node_ptr<ident_vec> imports) :
			name {name},
			imports {imports}
		{}
	};

	struct driver_file {
		const node_ptr<ident_vec> former;
		const node_ref<driver_def> driver;
		const node_ptr<ident_vec> latter;

		driver_file(node_ptr<ident_vec> former,
			node_ref<driver_def> driver,
			node_ptr<ident_vec> latter
		) :
			former {former},
			driver {driver},
			latter {latter}
		{}
	};

	struct type_proj {
		const ident name;

		proj_def* definition;

		type_proj(ident name) :
			name {name}
		{}
	};

	struct type_rpc {
		const ident name;

		rpc_def* definition;

		type_rpc(ident name) :
			name {name}
		{}
	};

	struct type_array {
		const node_ref<type_spec> element;
		const node_ref<array_size> size;

		type_array(node_ref<type_spec> element, node_ref<array_size> size) :
			element {element},
			size {size}
		{}
	};

	struct type_any_of {
		const ident discrim;
		const node_ref<ref_vec<type_spec>> types;

		type_any_of(
			ident discrim,
			node_ref<ref_vec<type_spec>> types
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
	struct type_string {};

	struct indirection {
		const annotation attrs; // Contextually, both ptr and value attrs
		const bool is_const;

		indirection(annotation attrs, bool is_const) :
			attrs {attrs},
			is_const {is_const}
		{}
	};

	// FIXME: field order
	struct type_spec {
		const node_ref<type_stem> stem;
		const ref_vec<indirection> indirs;
		const annotation attrs; // Will only ever have value attrs in it
		const bool is_const;

		// TODO: must these be cached?
		sema::data_field* pgraph; // These are not shared, but cached here

		type_spec(
			node_ref<type_stem> stem,
			ref_vec<indirection> indirs,
			annotation attrs,
			bool is_const
		) :
			stem {stem},
			indirs {indirs},
			attrs {attrs},
			is_const {is_const}
		{}
	};

	struct var_decl {
		const node_ref<type_spec> type;
		const ident name;

		var_decl(node_ref<type_spec> type, ident name) :
			type {type},
			name {name}
		{}
	};

	struct naked_proj_decl {
		const node_ptr<ref_vec<proj_field>> fields;
		const ident name;

		naked_proj_decl(node_ptr<ref_vec<proj_field>> fields, ident name) :
			fields {fields},
			name {name}
		{}
	};

	enum proj_def_kind {
		struct_kind,
		union_kind
	};

	struct proj_def {
		const ident name;
		const ident type;
		const node_ptr<ref_vec<proj_field>> fields;
		const proj_def_kind kind;

		sema::projection* pgraph;

		proj_def(
			ident name,
			ident type,
			node_ptr<ref_vec<proj_field>> fields,
			proj_def_kind kind
		) :
			name {name},
			type {type},
			fields {fields},
			kind {kind},
			pgraph {}
		{}
	};

	using rpc_item = std::variant<node_ref<proj_def>, node_ref<rpc_def>>;

	enum rpc_def_kind {
		direct,
		indirect
	};

	struct rpc_def {
		const node_ptr<type_spec> ret_type; // null type is used for <void>
		const ident name;
		const node_ptr<ref_vec<var_decl>> arguments;
		const node_ptr<ref_vec<rpc_item>> items;
		const rpc_def_kind kind;

		sema::scope scope;

		rpc_def(
			node_ptr<type_spec> ret_type,
			ident name,
			node_ptr<ref_vec<var_decl>> arguments,
			node_ptr<ref_vec<rpc_item>> items,
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
		const ident name;
		const node_ptr<ref_vec<module_item>> items;

		sema::scope scope;

		module_def(ident name, node_ptr<ref_vec<module_item>> items) :
			name {name},
			items {items}
		{}
	};

	/*
		Nodes that have scopes: module_defs, rpc_defs, proj_defs
	*/
}

#endif
