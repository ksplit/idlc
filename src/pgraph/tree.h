#ifndef _LCDS_IDL_MARSHAL_TREE_H_
#define _LCDS_IDL_MARSHAL_TREE_H_

#include <variant>

#include "../ast/ast.h"

namespace idlc::pgraph {
	// TODO: give all of these constructors so that make_unique can deal with them better

	/*
		Every "block" has an extent and a layout. For example, the block for a union projection
		has an extent equal to that of its largest element, but a layout that is determined at runtime.
		The only time we must care about the actual extent is when allocating a new block, in which case
		we use either the underlying type of a projection, the "real type" of an any_of_ptr, or the type of an array
		times its size, whether that is statically specified or dynamically determined.

		Layouts can be nested: if a projection has another as a value within it, the layout of the nested projection
		is nested within the outer one, and we specify that it is the layout of that particular field. Dynamic layouts
		exist: they are associated with a "resolving field" and a resolver function, which will be given that field
		as an input and output the corresponding "type tag" enum value (I.e. FOO_TYPE_BAR, FOO_TYPE_INT).

		To marshal a layout is simple: we marshal all of its primitive fields, and recursively marshal its sub-layouts.
		Marshaling code must know how to access relevant fields: something that is complicated by C having different
		member-access syntax based on whether the variable is pointer or a value. To reduce such difficulties,
		we might simply use pointers, producing pointer-initializing statements as needed to refer to each element.
		Or, assuming that C allows to mostly chain accesses, (i.e. foo->foo1.foo2[1].foo3->foo4), we could reason about
		how to produce an access to the field of interest. Note that I use "field" to refer generically to array
		elements, union members, etc.. But for clarity and ease of debugging I will likely end up producing multiple
		intermediate pointers that are used to access immediate child fields, and new pointers are created for deeper
		traversal. 

		Pointers are marked specially: they specify how to link them to the block they refer to, and NULL is always
		interpreted as the "none" block. To correctly link the pointer the type must be known, in the case of
		the `any_of_ptr` (which we might call a dyn_ptr at this scale), the generator will produce IR for each of the
		declared types, and wrap them in a switch-statement that dispatches off of a similar reolver-field-resolver
		pattern. Note that although union projections have a dynamically-selected layout, any_of_ptr has a dynamically
		selected layout and type (and thus extent).
	
		Layouts are variants in this tree, with primitive layout enum (for int, uint, etc.), or composite layouts
		(mostly like vectors of fields).

		Every union-typed field has its own discriminator potentially, so we are forced to make the distinction
		dyn_layout and an actual layout. dyn_layouts refer to their discriminating field (probably by name)

		Note the distinction between fields and layouts
		Fields (regions of memory) have layouts, but may be tagged with value annotations (in / out)
		A layout captures a tree-like structure of fields, with their annotations embedded
	*/

	using prim = ast::tyname_arith;
	using ast::tags;
	struct struct_layout;
	struct union_layout;
	struct dyn_ptr;
	struct ptr;
	struct array_layout;
	struct dyn_array_layout;
	struct null_array_layout;
	struct rpc_ptr_layout;
	struct field;
	using layout = std::variant<
		prim,
		std::unique_ptr<rpc_ptr_layout>,
		std::unique_ptr<struct_layout>,
		std::unique_ptr<union_layout>,
		std::unique_ptr<dyn_ptr>,
		std::unique_ptr<ptr>,
		std::unique_ptr<array_layout>,
		std::unique_ptr<dyn_array_layout>,
		std::unique_ptr<null_array_layout>
	>;

	struct field {
		std::unique_ptr<layout> layout;
		tags tags; // Value tags
	};

	struct rpc_ptr_layout {
		gsl::czstring<> name;
	};

	struct struct_layout {
		std::vector<std::pair<gsl::czstring<>, field>> fields;
	};

	struct array_layout {
		unsigned size;
		std::unique_ptr<field> elem;
	};

	struct dyn_array_layout {
		gsl::czstring<> length;
		std::unique_ptr<field> elem;
	};

	// FIXME: inefficient to have this as a pointer wrapper, is it needed?
	// Note that this will require some IR support for a recursive is_null call
	// Generate is_null methods as needed, similar to the recursive visits
	struct null_array_layout {
		std::unique_ptr<field> elem;
	};

	struct union_layout {
		gsl::czstring<> discriminator; // this is the name of the union member which marks the union type
		std::vector<std::pair<gsl::czstring<>, field>> members;
	};

	struct dyn_ptr {
		tags tags; // Pointer tags, consider strong-typing these
		gsl::czstring<> discriminator;
		std::vector<field> layouts;
	};

	struct ptr {
		tags tags; // Pointer tags, consider strong-typing these
		std::unique_ptr<field> layout;
	};

	enum class rpc_kind {
		direct,
		indirect
	};

	// pgraph construction delivers an array of these
	// These will be associated with a family of identifiers (RPC IDs, impl names, callee names, trampolines, etc.)
	// and for indirect nodes, will be associated with an fptr typedef (somehow??)
	// TODO: above
	struct rpc_node {
		rpc_kind kind;
		field ret;
		std::vector<std::pair<gsl::czstring<>, field>> args;
	};

	// Marshaling will need to somehow build up type strings for arbitrary field layouts
	// Ironically, this is easier to do as a walk over the AST, since the IDL is already similar to C in this regard,
	// so every layout may store a reference to the AST node it was built from

	// TODO: get_type_left_string(pnode) support for any field node, which computes the C-like declarator for any
	// portion of the pgraph (notably stopping at rpc pointer fields and projection fields)
	// NOTE: fairly certain it's impossible to create declarators for anonymous unions / structs (marshalling must never
	// request these)
	// TODO: get_typedef_for_rpc(node) produces the typedef statement of a particular rpc node
	// TODO: how to deal with arrays? array<const char, /* anything */> maps to const char* for the pointer type,
	// and can't be cast from a register
	// NOTE: get_field_ptr_type(node) vs get_reg_cast_type(node)
}

#endif
