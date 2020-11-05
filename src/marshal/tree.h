#ifndef _LCDS_IDL_MARSHAL_TREE_H_
#define _LCDS_IDL_MARSHAL_TREE_H_

namespace idlc::marshal {
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
	*/
}

#endif
