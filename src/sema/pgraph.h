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
	class projection_ptr; // because these nodes are shared

	template<typename type>
	using node_ptr = std::unique_ptr<type>;

	struct data_field;

	// Do not confuse this with thread-safe!
	class projection_ptr {
	public:
		projection_ptr() = default;

		projection_ptr(projection* ptr);
		projection_ptr(projection_ptr& o);
		projection_ptr& operator=(projection_ptr& o);
		~projection_ptr();

		projection_ptr(projection_ptr&& o) : ptr_ {o.ptr_}
		{
			o.ptr_ = nullptr;
		}

		projection_ptr& operator=(projection_ptr&& o)
		{
			ptr_ = o.ptr_;
			o.ptr_ = nullptr;
			return *this;
		}


		auto& operator*() const
		{
			return *ptr_;
		}

		auto operator->() const
		{
			return ptr_;
		}

		auto get() const
		{
			return ptr_;
		}

	private:
		projection* ptr_ {};
	};

	using field_type = std::variant<
		primitive,
		node_ptr<null_terminated_array>,
		node_ptr<dyn_array>,
		node_ptr<pointer>,
		node_ptr<static_void_ptr>,
		node_ptr<rpc_ptr>,
		projection_ptr
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
		std::vector<std::pair<ident, node_ptr<data_field>>> fields;
		annotation defaulted_with; // record which anotation triggered the default pass, to detect conflicts
		unsigned refcount; // NOTE: since projection nodes are ultimately shared due to semantics, it becomes necessary
		// to track their lifetime by refcount. See projection_ptr
	};

	inline projection_ptr::~projection_ptr() 
	{
		if (ptr_) {
			assert(ptr_->refcount > 0);

			if (!--ptr_->refcount)
				delete ptr_;
		}
	}

	inline projection_ptr::projection_ptr(projection* ptr) : ptr_ {ptr}
	{
		if (ptr) {
			assert(ptr_->refcount > 0);
			++(ptr->refcount);
		}
	}

	inline projection_ptr::projection_ptr(projection_ptr& o) : ptr_ {o.ptr_}
	{
		if (ptr_) {
			assert(ptr_->refcount > 0);
			++(ptr_->refcount);
		}
	}

	inline projection_ptr& projection_ptr::operator=(projection_ptr& o)
	{
		ptr_ = o.ptr_;
		if (ptr_) {
			assert(ptr_->refcount > 0);
			++(ptr_->refcount);
		}

		return *this;
	}

	inline auto make_projection()
	{
		const auto ptr = new projection {};
		ptr->refcount++;
		auto ref = projection_ptr {ptr};
		ptr->refcount--;
		return ref;
	}
}

#endif
