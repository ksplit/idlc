#ifndef IDLC_AST_AST_H_
#define IDLC_AST_AST_H_

#include <memory>
#include <string>
#include <variant>
#include <vector>

#include <gsl/gsl>

#include "../string_heap.h"
#include "../utility.h"
#include "node_ptrs.h"
#include "pgraph.h"
#include "scope.h"
#include "tag_types.h"

namespace idlc {
	using ident_vec = std::vector<ident>;

	struct module_def;
	struct driver_def;
	struct driver_file;
	struct type_none;
	struct type_rpc;
	struct type_proj;
	struct type_array;
	struct type_string;
	struct type_casted;
	struct type_spec;

	struct naked_proj_decl;
	struct var_decl;

	struct rpc_def;

	struct tok_kw_null {
	}; // Doesn't exist in parse rules, used as marker (represents tok_kw_null)

	using file = std::variant<node_ref<driver_file>, node_ref<ref_vec<module_def>>>;
	// using field_ref = std::variant<node_ref<field_abs_ref>, node_ref<field_rel_ref>>;
	using array_size = std::variant<unsigned, tok_kw_null, ident>;
	using proj_field = std::tuple<std::variant<node_ref<var_decl>, node_ref<naked_proj_decl>>, std::uint8_t>;

	using type_stem = std::variant<
		type_primitive,
		type_string,
		type_none,
		node_ref<type_rpc>,
		node_ref<type_proj>,
		node_ref<type_array>,
		node_ref<type_casted>>;

	struct type_none {
	};

	struct driver_def {
		const ident name;
		const node_ptr<ident_vec> imports;

		driver_def(ident name, node_ptr<ident_vec> imports) : name {name}, imports {imports} {}
	};

	struct driver_file {
		const node_ptr<ident_vec> former;
		const node_ref<driver_def> driver;
		const node_ptr<ident_vec> latter;

		driver_file(node_ptr<ident_vec> former, node_ref<driver_def> driver, node_ptr<ident_vec> latter) :
			former {former},
			driver {driver},
			latter {latter}
		{
		}
	};

	struct type_proj {
		const ident name;

		proj_def* definition;

		type_proj(ident name) : name {name}, definition {} {}
	};

	struct type_rpc {
		const ident name;
		const bool is_static;

		rpc_def* definition;

		type_rpc(ident name, bool is_static = false) :
			name {name},
			is_static {is_static},
			definition {}
		{}
	};

	struct type_array {
		const node_ref<type_spec> element;
		const node_ref<array_size> size;

		type_array(node_ref<type_spec> element, node_ref<array_size> size) : element {element}, size {size} {}
	};

	// Another marker
	// NOTE: since these don't actually exist in any meaningful sense, their own parse rules don't produce them
	struct type_string {
	};

	struct type_casted {
		// NOTE: this type is only here for the benefit of making a c-specifier
		const node_ref<type_spec> declared_type;
		const node_ref<type_spec> true_type;

		type_casted(node_ref<type_spec> declared_type, node_ref<type_spec> true_type) :
			declared_type {declared_type},
			true_type {true_type}
		{
		}
	};

	struct indirection {
		const node_ref<annotation_set> attrs; // Contextually, both ptr and value attrs
		const bool is_const;

		indirection(node_ref<annotation_set> attrs, bool is_const) : attrs {attrs}, is_const {is_const} {}
	};

	struct type_spec {
		const node_ref<type_stem> stem;
		const ref_vec<indirection> indirs;
		const annotation_bitfield attrs; // Will only ever have value attrs in it
		bool is_const; // HACK: see string_const_walk, this can't be immutable otherwise
		const bool is_volatile;

		type_spec(
			node_ref<type_stem> stem,
			ref_vec<indirection> indirs,
			annotation_bitfield attrs,
			bool is_const,
			bool is_volatile) :
			stem {stem},
			indirs {indirs},
			attrs {attrs},
			is_const {is_const},
			is_volatile {is_volatile}
		{
		}
	};

	struct var_decl {
		const node_ref<type_spec> type;
		const ident name;

		var_decl(node_ref<type_spec> type, ident name) : type {type}, name {name} {}
	};

	struct naked_proj_decl {
		const node_ptr<ref_vec<proj_field>> fields;
		const ident name;

		naked_proj_decl(node_ptr<ref_vec<proj_field>> fields, ident name) : fields {fields}, name {name} {}
	};

	enum proj_def_kind { struct_kind, union_kind };

	struct proj_def {
		const ident name;
		const ident type;
		const node_ptr<ref_vec<proj_field>> fields;
		const proj_def_kind kind;

		std::string scoped_name;
		std::shared_ptr<projection> in_proj;
		std::shared_ptr<projection> out_proj;
		std::shared_ptr<projection> in_out_proj;

		const rpc_def* parent;

		proj_def(ident name, ident type, node_ptr<ref_vec<proj_field>> fields, proj_def_kind kind) :
			name {name},
			type {type},
			fields {fields},
			kind {kind},
			in_proj {},
			out_proj {},
			in_out_proj {}
		{
		}
	};

	using rpc_item = std::variant<node_ref<proj_def>, node_ref<rpc_def>>;

	enum rpc_def_kind {
		direct,
		indirect,
		export_sym,
	};

	struct rpc_def {
		const node_ref<type_spec> ret_type;
		const ident name;
		// I don't enjoy it, but the vector has to be indirected as well to play nice with the parser lifetime stuff
		const node_ptr<ref_vec<var_decl>> arguments;
		const node_ptr<ref_vec<rpc_item>> items;
		const rpc_def_kind kind;

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
	struct header_stmt {
	};

	using module_item = std::variant<header_stmt, node_ref<proj_def>, node_ref<rpc_def>>;

	struct module_def {
		const ident name;
		const node_ptr<ref_vec<module_item>> items;

		names_scope scope;

		module_def(ident name, node_ptr<ref_vec<module_item>> items) : name {name}, items {items}, scope {} {}
	};
}

#endif
