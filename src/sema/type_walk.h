#ifndef IDLC_SEMA_TYPE_WALK_H
#define IDLC_SEMA_TYPE_WALK_H

#include "../ast/ast.h"
#include "../ast/walk.h"

namespace idlc::sema {
	using primitive = ast::type_primitive;
	struct null_terminated_array;
	struct static_array;
	struct dyn_array;
	struct pointer;
	struct static_void_ptr; // a void* that is "actually" a pointer to some other type
	struct projection;
	class projection_ptr; // because these nodes are shared

	// TODO: share with the AST
	template<typename type>
	using node_ptr = std::unique_ptr<type>;

	struct data_field;

	// Do not confuse this with thread-safe!
	class projection_ptr {
	public:
		projection_ptr() = default;

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

	private:
		friend auto make_projection(std::vector<std::pair<ident, node_ptr<data_field>>>&& fields);

		projection* ptr_ {};
	};

	using field_type = std::variant<
		primitive,
		node_ptr<null_terminated_array>,
		node_ptr<dyn_array>,
		node_ptr<pointer>,
		node_ptr<static_void_ptr>,
		projection_ptr
	>;

	struct data_field {
		field_type type;
		ast::annotation value_annots;

		data_field(field_type&& type, ast::annotation value_annots) :
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
		ast::annotation pointer_annots;
		bool is_const;

		pointer(node_ptr<data_field> referent, ast::annotation pointer_annots, bool is_const) :
			referent {std::move(referent)},
			pointer_annots {pointer_annots},
			is_const {is_const}
		{}
	};

	// TODO: this has no syntax
	struct static_void_ptr {
		node_ptr<data_field> referent;
		ast::annotation pointer_annots;
		bool is_const;

		static_void_ptr(node_ptr<data_field> referent, ast::annotation pointer_annots, bool is_const) :
			referent {std::move(referent)},
			pointer_annots {pointer_annots},
			is_const {is_const}
		{}
	};

	struct projection {
		std::vector<std::pair<ident, node_ptr<data_field>>> fields;
		unsigned refcount; // NOTE: since projection nodes are ultimately shared due to semantics, it becomes necessary
		// to track their lifetime by refcount. See projection_ptr

		projection(std::vector<std::pair<ident, node_ptr<data_field>>>&& fields) : fields {std::move(fields)} {}
	};

	inline projection_ptr::~projection_ptr() 
	{
		if (ptr_) {
			if (!--ptr_->refcount)
				delete ptr_;
		}
	}

	inline projection_ptr::projection_ptr(projection_ptr& o) : ptr_ {o.ptr_}
	{
		if (ptr_)
			++ptr_->refcount;
	}

	inline projection_ptr& projection_ptr::operator=(projection_ptr& o)
	{
		ptr_ = o.ptr_;
		if (ptr_)
			++ptr_->refcount;

		return *this;
	}

	inline auto make_projection(std::vector<std::pair<ident, node_ptr<data_field>>>&& fields)
	{
		projection_ptr ptr {};
		ptr.ptr_ = new projection {std::move(fields)};
		return ptr;
	}

	// TODO: I recall that any_of was in flux
	// void<some_type> was for the static case, which is the one nullnet needs
	// any_of<> does not currently require work

	/*
		From slack:
		it looks like for nullnet all that will be needed will be struct support, and dynamic array support
		(and void<> static type support)
		- string support is also needed, i.e. null_terminated_array
		- struct projections
		- dynamic arrays
		- primitives
		- pointers
		- void<> static type
		- at *least* a string const hack, if not actual const handling
		- nullnet_vmfunc_idl *did* fill out the sockaddr struct, but it's unclear if that's necessary

		Current avenue is understanding just how *much* of sockaddr needs to be filled out, based off of the
		nullnet_vmfunc example. It seems the caller stub is ifdef-ed out, so it may not even be necessary
	*/

	// TODO: introduce the void<> system for "raw" void pointers

	node_ptr<data_field> build_data_field(const ast::type_spec& node);

	class type_walk : public ast::ast_walk<type_walk> {
	public:
		bool visit_type_spec(ast::type_spec& node)
		{
			build_data_field(node);
			return true;
		}

	private:
		field_type stem_ {};
	};
}

#endif
