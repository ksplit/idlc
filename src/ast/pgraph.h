#ifndef IDLC_AST_PGRAPH
#define IDLC_AST_PGRAPH

#include <cassert>
#include <memory>
#include <utility>
#include <variant>

#include "node_ptrs.h"
#include "tag_types.h"
#include "../string_heap.h"
#include "../utility.h"

namespace idlc {
	struct proj_def;
	struct rpc_def;
}

namespace idlc {
	using primitive = type_primitive;
	struct null_terminated_array;
	struct static_array;
	struct dyn_array;
	struct pointer;
	struct static_void_ptr; // a void* that is "actually" a pointer to some other type
	struct projection;
	struct rpc_ptr;

	struct value;

	// TODO: much of these are exclusively owned (except projection nodes), but node_ptrs/node_refs are shared
	using passed_type = std::variant<
		primitive,
		node_ptr<null_terminated_array>,
		node_ptr<dyn_array>,
		node_ptr<pointer>,
		node_ptr<static_void_ptr>,
		node_ptr<rpc_ptr>,
		node_ref<projection>,
		gsl::not_null<proj_def*> // NOTE: will *never* appear in a finished pgraph, part of lowering
	>;

	struct value {
		passed_type type;
		annotation value_annots;
		// FIXME: add const-ness here, drop from <<pointer>>

		std::string c_specifier;

		value(passed_type&& type, annotation value_annots) :
			type {std::move(type)},
			value_annots {value_annots},
			c_specifier {}
		{}
	};

	struct null_terminated_array {
		node_ptr<value> element;
		bool is_const; // referring to its elements

		null_terminated_array(node_ptr<value> element, bool is_const) :
			element {std::move(element)},
			is_const {is_const}
		{}
	};

	// struct static_array {
	// 	node_ptr<value> element;
	// 	unsigned size;
	// };

	struct dyn_array {
		node_ptr<value> element;
		ident size;
		bool is_const; // referring to its elements

		dyn_array(node_ptr<value> element, ident size, bool is_const) :
			element {std::move(element)},
			size {size},
			is_const {is_const}
		{}
	};

	struct pointer {
		node_ptr<value> referent;
		annotation pointer_annots;
		bool is_const;

		pointer(node_ptr<value> referent, annotation pointer_annots, bool is_const) :
			referent {std::move(referent)},
			pointer_annots {pointer_annots},
			is_const {is_const}
		{}
	};

	// TODO: this has no syntax
	struct static_void_ptr {
		node_ptr<value> referent;
		annotation pointer_annots;
		bool is_const;

		static_void_ptr(node_ptr<value> referent, annotation pointer_annots, bool is_const) :
			referent {std::move(referent)},
			pointer_annots {pointer_annots},
			is_const {is_const}
		{}
	};

	struct rpc_ptr {
		rpc_def* definition;

		rpc_ptr(rpc_def* definition) : definition {definition} {}
	};

	using projection_field = std::pair<ident, node_ptr<value>>;

	class projection {
	public:
		ident real_name {};
		std::vector<projection_field> fields {};

		std::string arg_marshal_visitor {};
		std::string arg_unmarshal_visitor {};
		std::string arg_remarshal_visitor {};
		std::string arg_unremarshal_visitor {};
		std::string ret_marshal_visitor {};
		std::string ret_unmarshal_visitor {};

		projection(ident real_name, const std::string& name) :
			real_name {real_name},
			fields {},
			arg_marshal_visitor {},
			arg_unmarshal_visitor {},
			arg_remarshal_visitor {},
			arg_unremarshal_visitor {},
			ret_marshal_visitor {},
			ret_unmarshal_visitor {}
		{
			populate_names(name);
		};
		
		projection(ident real_name, const std::string& name, decltype(fields)&& fields) :
			real_name {real_name},
			fields {std::move(fields)},
			arg_marshal_visitor {},
			arg_unmarshal_visitor {},
			arg_remarshal_visitor {},
			arg_unremarshal_visitor {},
			ret_marshal_visitor {},
			ret_unmarshal_visitor {}
		{
			populate_names(name);
		}

	private:
		void populate_names(const std::string& name)
		{
			arg_marshal_visitor = concat("arg_marshal_", name);
			arg_unmarshal_visitor = concat("arg_unmarshal_", name);
			arg_remarshal_visitor = concat("arg_remarshal_", name);
			arg_unremarshal_visitor = concat("arg_unremarshal_", name);
			ret_marshal_visitor = concat("ret_marshal_", name);
			ret_unmarshal_visitor = concat("ret_unmarshal_", name);
		}
	};
}

#endif
