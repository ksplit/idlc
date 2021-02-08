#ifndef IDLC_AST_PGRAPH
#define IDLC_AST_PGRAPH

#include <cassert>
#include <memory>
#include <utility>
#include <variant>

#include "node_ptrs.h"
#include "tag_types.h"
#include "../string_heap.h"

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

		std::string type_string;

		value(passed_type&& type, annotation value_annots) :
			type {std::move(type)},
			value_annots {value_annots}
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

	class projection {
	public:
		ident real_name {};
		std::vector<std::pair<ident, node_ptr<value>>> fields {};

		std::string visit_arg_marshal_name {};
		std::string visit_arg_unmarshal_name {};
		std::string visit_arg_remarshal_name {};
		std::string visit_arg_unremarshal_name {};
		std::string visit_ret_marshal_name {};
		std::string visit_ret_unmarshal_name {};

		projection(ident real_name, const std::string& name) : real_name {real_name}, fields {}
		{
			populate_names(name);
		};
		
		projection(ident real_name, const std::string& name, decltype(fields)&& fields) :
			real_name {real_name},
			fields {std::move(fields)},
			visit_arg_marshal_name {},
			visit_arg_unmarshal_name {},
			visit_arg_remarshal_name {},
			visit_arg_unremarshal_name {},
			visit_ret_marshal_name {},
			visit_ret_unmarshal_name {}
		{
			populate_names(name);
		}

	private:
		void populate_names(const std::string& name)
		{
			visit_arg_marshal_name = "visit_arg_marshal_" + name;
			visit_arg_unmarshal_name = "visit_arg_unmarshal_" + name;
			visit_arg_remarshal_name = "visit_arg_remarshal_" + name;
			visit_arg_unremarshal_name = "visit_arg_unremarshal_" + name;
			visit_ret_marshal_name = "visit_ret_marshal_" + name;
			visit_ret_unmarshal_name = "visit_ret_unmarshal_" + name;
		}
	};
}

#endif
