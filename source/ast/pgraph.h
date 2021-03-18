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
	class projection;
	struct rpc_ptr;

	struct value;

	// TODO: much of these are exclusively owned (except projection nodes), but node_ptrs/node_refs are shared
	using passed_type = std::variant<
		primitive,
		node_ref<null_terminated_array>,
		node_ref<static_array>,
		node_ref<dyn_array>,
		node_ref<pointer>,
		node_ref<static_void_ptr>,
		node_ref<rpc_ptr>,
		node_ref<projection>,
		gsl::not_null<proj_def*> // NOTE: will *never* appear in a finished pgraph, part of lowering
	>;

	struct value {
		passed_type type;
		annotation value_annots;
		bool is_const;

		std::string c_specifier;

		value(passed_type&& type, annotation value_annots, bool is_const) :
			type {std::move(type)},
			value_annots {value_annots},
			is_const {is_const},
			c_specifier {}
		{}
	};

	// NOTE: A non-terminal node is any node that has child nodes
	// NOTE: we *always* walk non-terminals, to be able to marshal otherwise innaccessible [out] fields
	// TODO: add support for array non-terminals
	inline bool is_nonterminal(const value& node)
	{
		const auto visit = [](auto&& item) {
			using type = std::decay_t<decltype(item)>;
			return std::is_same_v<type, node_ref<projection>>
				|| std::is_same_v<type, node_ref<pointer>>;
		};

		return std::visit(visit, node.type);
	}

	struct null_terminated_array {
		node_ptr<value> element;

		null_terminated_array(node_ptr<value> element) :
			element {std::move(element)}
		{}
	};

	// struct static_array {
	// 	node_ptr<value> element;
	// 	unsigned size;
	// };

	struct dyn_array {
		node_ptr<value> element;
		ident size;

		dyn_array(node_ptr<value> element, ident size) :
			element {std::move(element)},
			size {size}
		{}
	};

	struct static_array {
		node_ptr<value> element;
		unsigned size;

		static_array(node_ptr<value> element, unsigned size) :
			element {std::move(element)},
			size {size}
		{}
	};

	struct pointer {
		node_ptr<value> referent;
		annotation pointer_annots;

		pointer(node_ptr<value> referent, annotation pointer_annots) :
			referent {std::move(referent)},
			pointer_annots {pointer_annots}
		{}
	};

	// TODO: this has no syntax
	struct static_void_ptr {
		node_ptr<value> referent;
		annotation pointer_annots;

		static_void_ptr(node_ptr<value> referent, annotation pointer_annots) :
			referent {std::move(referent)},
			pointer_annots {pointer_annots}
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

		std::string caller_marshal_visitor {};
		std::string callee_unmarshal_visitor {};
		std::string callee_marshal_visitor {};
		std::string caller_unmarshal_visitor {};

		projection(ident real_name, const std::string& name) :
			real_name {real_name},
			fields {},
			caller_marshal_visitor {},
			callee_unmarshal_visitor {},
			callee_marshal_visitor {},
			caller_unmarshal_visitor {}
		{
			populate_names(name);
		};
		
		projection(ident real_name, const std::string& name, decltype(fields)&& fields) :
			real_name {real_name},
			fields {std::move(fields)},
			caller_marshal_visitor {},
			callee_unmarshal_visitor {},
			callee_marshal_visitor {},
			caller_unmarshal_visitor {}
		{
			populate_names(name);
		}

	private:
		void populate_names(const std::string& name)
		{
			caller_marshal_visitor = concat("caller_marshal_", name);
			callee_unmarshal_visitor = concat("callee_unmarshal_", name);
			callee_marshal_visitor = concat("callee_marshal_", name);
			caller_unmarshal_visitor = concat("caller_unmarshal_", name);
		}
	};
}

#endif
