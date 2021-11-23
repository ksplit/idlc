#ifndef IDLC_AST_PGRAPH
#define IDLC_AST_PGRAPH

#include <cassert>
#include <memory>
#include <utility>
#include <variant>

#include "../string_heap.h"
#include "../utility.h"
#include "node_ptrs.h"
#include "tag_types.h"

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
	struct casted_type; // a void* that is "actually" a pointer to some other type
	class projection;
	struct rpc_ptr;
	struct none {
	};

	struct value;

	// TODO: much of these are exclusively owned (except projection nodes), but node_ptrs/node_refs are shared
	using passed_type = std::variant<
		primitive,
		none,
		node_ref<null_terminated_array>,
		node_ref<static_array>,
		node_ref<dyn_array>,
		node_ref<pointer>,
		node_ref<casted_type>,
		node_ref<rpc_ptr>,
		node_ref<projection>,
		gsl::not_null<proj_def*> // NOTE: will *never* appear in a finished pgraph, part of lowering
		>;

	struct value {
		passed_type type; // Not const, but only because we do lowering from a raw proj_def* to the actual node
		annotation_bitfield value_annots; // needs to be non-const, we do defaulting walks
		bool is_const; // not entirely convinced that this can't be const, see const_propagation_walk
		bool is_volatile; // we don't use this at all afaik

		std::string c_specifier;

		value(passed_type&& type, annotation_bitfield value_annots, bool is_const, bool is_volatile) :
			type {std::move(type)},
			value_annots {value_annots},
			is_const {is_const},
			is_volatile {is_volatile},
			c_specifier {}
		{
		}
	};

	// NOTE: A non-terminal node is any node that has child nodes
	// NOTE: we *always* walk non-terminals, to be able to marshal otherwise innaccessible [out] fields
	// TODO: replace this non-terminal stuff with some kind of reachability analysis
	// TODO: add support for array non-terminals
	inline bool is_nonterminal(const value& node)
	{
		const auto visit = [](auto&& item) {
			using type = std::decay_t<decltype(item)>;
			return std::is_same_v<type, node_ref<projection>> || std::is_same_v<type, node_ref<pointer>>;
		};

		return std::visit(visit, node.type);
	}

	struct null_terminated_array {
		const node_ptr<value> element;

		null_terminated_array(node_ptr<value> element) : element {std::move(element)} {}
	};

	// struct static_array {
	// 	node_ptr<value> element;
	// 	unsigned size;
	// };

	struct dyn_array {
		const node_ptr<value> element;
		const ident size_expr;

		dyn_array(node_ptr<value> element, ident size) : element {std::move(element)}, size_expr {size} {}
	};

	struct static_array {
		const node_ptr<value> element;
		const unsigned size;

		static_array(node_ptr<value> element, unsigned size) : element {std::move(element)}, size {size} {}
	};

	struct pointer {
		const node_ptr<value> referent;
		annotation_set pointer_annots;

		pointer(node_ptr<value> referent, annotation_set pointer_annots) :
			referent {std::move(referent)},
			pointer_annots {pointer_annots}
		{
		}
	};

	struct casted_type {
		const node_ptr<value> referent;
		const node_ptr<value> facade; // NOTE: mostly meaningless, we special-case this in the walk logic to avoid
									  // marshaling it, kind of hacky

		casted_type(node_ptr<value> facade, node_ptr<value> referent) :
			facade {std::move(facade)},
			referent {std::move(referent)}
		{
		}
	};

	struct rpc_ptr {
		const rpc_def* definition;

		rpc_ptr(rpc_def* definition) : definition {definition} {}
	};

	using projection_field = std::pair<ident, node_ptr<value>>;

	struct projection {
		const ident real_name {};
		const std::vector<projection_field> fields {};

		const std::string caller_marshal_visitor {};
		const std::string callee_unmarshal_visitor {};
		const std::string callee_marshal_visitor {};
		const std::string caller_unmarshal_visitor {};

		proj_def* def {};

		projection(ident real_name, const std::string& name) :
			real_name {real_name},
			fields {},
			caller_marshal_visitor {concat("caller_marshal_", name)},
			callee_unmarshal_visitor {concat("callee_unmarshal_", name)},
			callee_marshal_visitor {concat("callee_marshal_", name)},
			caller_unmarshal_visitor {concat("caller_unmarshal_", name)} {};

		projection(ident real_name, const std::string& name, decltype(fields)&& fields) :
			real_name {real_name},
			fields {std::move(fields)},
			caller_marshal_visitor {concat("caller_marshal_", name)},
			callee_unmarshal_visitor {concat("callee_unmarshal_", name)},
			callee_marshal_visitor {concat("callee_marshal_", name)},
			caller_unmarshal_visitor {concat("caller_unmarshal_", name)}
		{
		}
	};
}

#endif
