#ifndef IDLC_AST_AST_H_
#define IDLC_AST_AST_H_

#include <memory>
#include <string>
#include <vector>
#include <variant>

#include <gsl/gsl>

#include "../string_heap.h"
#include "../utility.h"
#include "tag_types.h"
#include "scope.h"
#include "pgraph.h"
#include "node_ptrs.h"

namespace idlc {
	using ident_vec = std::vector<ident>;

	struct module_def;
	struct driver_def;
	struct driver_file;
	struct type_none;
	struct type_rpc;
	struct type_proj;
	struct type_array;
	struct type_any_of;
	struct type_string;
	struct type_spec;

	struct naked_proj_decl;
	struct var_decl;
	
	struct rpc_def;
	struct global_def;

	struct tok_kw_null {}; // Doesn't exist in parse rules, used as marker (represents tok_kw_null)
	
	using file = std::variant<node_ref<driver_file>, node_ref<ref_vec<module_def>>>;
	// using field_ref = std::variant<node_ref<field_abs_ref>, node_ref<field_rel_ref>>;
	using array_size = std::variant<unsigned, tok_kw_null, ident>;
	using proj_field = std::variant<node_ref<var_decl>, node_ref<naked_proj_decl>>;
	using type_stem = std::variant<
		type_primitive,
		type_string,
		type_none,
		node_ref<type_rpc>,
		node_ref<type_proj>,
		node_ref<type_array>,
		node_ref<type_any_of>
	>;

	struct type_none {};

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

	struct type_proj {
		ident name;

		proj_def* definition;

		type_proj(ident name) :
			name {name},
			definition {}
		{}
	};

	struct type_rpc {
		ident name;

		rpc_def* definition;

		type_rpc(ident name) :
			name {name},
			definition {}
		{}
	};

	struct type_array {
		node_ref<type_spec> element;
		node_ref<array_size> size;

		type_array(node_ref<type_spec> element, node_ref<array_size> size) :
			element {element},
			size {size}
		{}
	};

	struct type_any_of {
		ident discrim;
		node_ref<ref_vec<type_spec>> types;

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
		node_ref<annotation> attrs; // Contextually, both ptr and value attrs
		bool is_const;

		indirection(node_ref<annotation> attrs, bool is_const) :
			attrs {attrs},
			is_const {is_const}
		{}
	};

	struct type_spec {
		node_ref<type_stem> stem;
		ref_vec<indirection> indirs;
		annotation_kind attrs; // Will only ever have value attrs in it
		bool is_const;

		type_spec(
			node_ref<type_stem> stem,
			ref_vec<indirection> indirs,
			annotation_kind attrs,
			bool is_const
		) :
			stem {stem},
			indirs {indirs},
			attrs {attrs},
			is_const {is_const}
		{}
	};

	struct var_decl {
		node_ref<type_spec> type;
		ident name;

		var_decl(node_ref<type_spec> type, ident name) :
			type {type},
			name {name}
		{}
	};

	struct naked_proj_decl {
		node_ptr<ref_vec<proj_field>> fields;
		ident name;

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
		ident name;
		ident type;
		node_ptr<ref_vec<proj_field>> fields;
		proj_def_kind kind;

		std::string scoped_name;
		std::shared_ptr<projection> in_proj;
		std::shared_ptr<projection> out_proj;
		std::shared_ptr<projection> in_out_proj;

		const rpc_def* parent;

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
			in_proj {},
			out_proj {},
			in_out_proj {}
		{}
	};

	using rpc_item = std::variant<node_ref<proj_def>, node_ref<rpc_def>>;

	enum rpc_def_kind {
		direct,
		indirect
	};

	struct rpc_def {
		node_ref<type_spec> ret_type;
		ident name;
		node_ptr<ref_vec<var_decl>> arguments;
		node_ptr<ref_vec<rpc_item>> items;
		rpc_def_kind kind;

		names_scope scope;
		node_ptr<value> ret_pgraph;
		ptr_vec<value> arg_pgraphs;

		std::string enum_id;
		std::string callee_id;
		std::string ctx_id;

		std::string typedef_id;
		std::string impl_typedef_id;
		std::string trmp_id;
		std::string impl_id;

		std::string ret_string;
		std::string args_string;
		std::string params_string;


		rpc_def(
			node_ptr<type_spec> ret_type,
			ident name,
			node_ptr<ref_vec<var_decl>> arguments,
			node_ptr<ref_vec<rpc_item>> items,
			rpc_def_kind kind) :
			ret_type {ret_type},
			name {name},
			arguments {arguments},
			items {items},
			kind {kind},
			scope {},
			ret_pgraph {},
			arg_pgraphs {},
			enum_id {},
			callee_id {},
			typedef_id {},
			impl_typedef_id {},
			trmp_id {},
			impl_id {},
			ret_string {},
			args_string {},
			params_string {}
		{
			append(enum_id, "RPC_ID_", name);
			append(callee_id, name, "_callee");
			append(ctx_id, name, "_call_ctx");
			if (kind == rpc_def_kind::indirect) {
				append(trmp_id, "trmp_", name);
				append(impl_id, "trmp_impl_", name);
				append(typedef_id, "fptr_", name);
				append(impl_typedef_id, "fptr_impl_", name);
			}
		}
	};

	// TODO: impl
	// Headers named in these nodes will get included into the generated "common" header
	struct header_stmt {};

	using module_item = std::variant<
		header_stmt,
		node_ref<proj_def>,
		node_ref<rpc_def>,
		node_ref<global_def>
	>;

	struct module_def {
		ident name;
		node_ptr<ref_vec<module_item>> items;

		names_scope scope;

		module_def(ident name, node_ptr<ref_vec<module_item>> items) :
			name {name},
			items {items},
			scope {}
		{}
	};

	struct global_def {
		ident name;
		node_ref<type_spec> type;
		node_ptr<ref_vec<rpc_item>> items;

		names_scope scope;
		node_ptr<value> pgraph;

		global_def(ident name, node_ref<type_spec> type, node_ptr<ref_vec<rpc_item>> items) :
			name {name},
			type {type},
			items {items},
			scope {},
			pgraph {}
		{}
	};

	/*
		Nodes that have scopes: module_defs, rpc_defs, proj_defs, global_defs
	*/
}

#endif
