#ifndef IDLC_SEMA_TYPES_H
#define IDLC_SEMA_TYPES_H

#include <memory>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

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

	// TODO: share with the AST
	template<typename type>
	using node_ptr = std::unique_ptr<type>;

	using field_type = std::variant<
		primitive,
		node_ptr<null_terminated_array>,
		node_ptr<dyn_array>,
		node_ptr<pointer>,
		node_ptr<projection>,
		node_ptr<static_void_ptr>
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
	};

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

	// TODO: introduce the none_ptr system for "raw" void pointers

	class type_walk : public ast::ast_walk<type_walk> {
	public:
		bool visit_type_spec(ast::type_spec& node)
		{
			// TODO: move type logic here
			return traverse(*this, node);
		}

		bool visit_type_stem(ast::type_stem& node)
		{
			const auto visit = [this](auto&& item)
			{
				using type = std::decay_t<decltype(item)>;
				if constexpr (std::is_same_v<type, ast::type_primitive>) {
					stem_ = item;
				}
				else if constexpr (std::is_same_v<type, ast::type_string>) {
					stem_ = std::make_unique<null_terminated_array>(
						std::make_unique<data_field>(
							primitive::ty_char,
							ast::annotation::use_default
						),
						true // FIXME: respect const keyword for strings, const-ness is invisible at type-stem level
					);
				}
				else {
					return ast::traverse(*this, item);
				}

				return true;
			};

			return std::visit(visit, node);
		}

	private:
		field_type stem_ {};
	};
}

#endif
