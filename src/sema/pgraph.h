#ifndef IDLC_SEMA_PGRAPH
#define IDLC_SEMA_PGRAPH

#include <cassert>
#include <memory>
#include <utility>
#include <variant>

#include "../tag_types.h"
#include "../parser/string_heap.h"

namespace idlc::sema {
	using primitive = type_primitive;
	struct null_terminated_array;
	struct static_array;
	struct dyn_array;
	struct pointer;
	struct static_void_ptr; // a void* that is "actually" a pointer to some other type
	struct projection;
	struct rpc_ptr;

	template<typename type>
	using node_ptr = std::unique_ptr<type>;

	struct data_field;

	using field_type = std::variant<
		primitive,
		node_ptr<null_terminated_array>,
		node_ptr<dyn_array>,
		node_ptr<pointer>,
		node_ptr<static_void_ptr>,
		node_ptr<rpc_ptr>,
		node_ptr<projection>
	>;

	struct data_field {
		field_type type;
		annotation value_annots;
		// FIXME: add const-ness here, drop from <<pointer>>

		data_field(field_type&& type, annotation value_annots) :
			type {std::move(type)},
			value_annots {value_annots}
		{}
	};

	struct null_terminated_array {
		node_ptr<data_field> element;
		bool is_const; // referring to its elements

		null_terminated_array(node_ptr<data_field> element, bool is_const) :
			element {std::move(element)},
			is_const {is_const}
		{}
	};

	// struct static_array {
	// 	node_ptr<data_field> element;
	// 	unsigned size;
	// };

	struct dyn_array {
		node_ptr<data_field> element;
		ident size;
		bool is_const; // referring to its elements

		dyn_array(node_ptr<data_field> element, ident size, bool is_const) :
			element {std::move(element)},
			size {size},
			is_const {is_const}
		{}
	};

	struct pointer {
		node_ptr<data_field> referent;
		annotation pointer_annots;
		bool is_const;

		pointer(node_ptr<data_field> referent, annotation pointer_annots, bool is_const) :
			referent {std::move(referent)},
			pointer_annots {pointer_annots},
			is_const {is_const}
		{}
	};

	// TODO: this has no syntax
	struct static_void_ptr {
		node_ptr<data_field> referent;
		annotation pointer_annots;
		bool is_const;

		static_void_ptr(node_ptr<data_field> referent, annotation pointer_annots, bool is_const) :
			referent {std::move(referent)},
			pointer_annots {pointer_annots},
			is_const {is_const}
		{}
	};

	// TODO: implement me, currently a dummy
	struct rpc_ptr {

	};

	struct projection {
		std::vector<std::pair<ident, node_ptr<data_field>>> fields {};

		projection() = default;
		projection(decltype(fields)&& fields) : fields {std::move(fields)} {}
	};
}

#endif
