# Elevating semantic constructs into syntax

Consider: `rpc void add_widget(const char [string]* name, projection widget_piece [alloc(callee)]* [size=10]* parts);`
`name`'s marshaling is self-evident: it's a constant null-terminated ANSI string. But for `idlc` to recognize this,
it will parse name's type as a `pointer_type` node with a `copy_type` of `const_char_k`, note that it has 1 star, and an annotation set
containing only `string`. Then a later pass will notice the `string` annotation, verify that the type makes sense, involving knowledge of the
node and its child, before finally marking it down as the spiritual equivalent of `const_string`.

Similarly with the array, a significant amount of effort will be expended to notice and verify that this is the equivalent of a size-10 array with
pointers to `widget_piece`s as elements. A better alternative, which loses no readability but greatly simplifies the parser, and entirely eliminates
a class of IDL errors, is a simple modification to the syntax: `rpc void add_widget(const string name, array<projection widget_piece [alloc(callee)]*, 10>* parts);`
This allows work that would be done in compiler passes to occur directly in-parser to recognize these specialized marshaling constructs.
The exact same information is presented to the user, in an arguably more readable form.

# Primitives

For the most part, marshaling doesn't actually care about the *exact* primtive type specification. C has several type specifications that map to the same
underlying data type, and GCC will treat these as interchangeable. As such, instead of supporting all possible C type specifications, the compiler need only
know which of these "size classes" the primitive falls into. From a user standpoint, it's relatively unlikely that an IDL author will miss being able to write
the logical equivalent of `long long` as `signed long long int`. Yongzhe's static analyzer currently appears to emit the verbatim type specs from the original C
code, which would force the parser to support all possible C primtive type specifications. Since it operates on the bitcode, I suspect that it's performing
an LLVM sized type to C type transformation, in which case it would be relatively simple to change the "canonical" forms in which these types are emitted to IDL.
So the grammar should disallow all but one "canonical" form for these logically equivalent types.

# Annotations going forward

The annotation system works well for assigning marshaling information to pointers, as it allows the author / reader to use the familiar star notation.
What it shouldn't be used for is assigning what is effectively type information to pointers that are not marshaled like pointers. 

# Types

- Strings (possibly non-const?)
    - values forbidden, only pointers: `const string*`
    - String-iness refers only to the "object" pointed to
- Arrays: `<elem-type>[<size>]`
    - Both values and pointer to them
- Projections (both union and struct)
    - is it valid to speak of by-value projection?
- void*, effectively treated as a kind of indirected, anonymous union
    - `any_of<<choose-expr>, projection foo, projection bar>`: a pointer referring to any of the specified types (specify how to recognize which?)
    - `any_of<>` values are forbidden, only pointers to them
    - `any_of<>` values *could* be used to express anonymous unions

Context for later "smart" marshaling is effectively the entire call itself. All `visit_*` functions generated for a particular
RPC will use the same context struct with the call args packed within it. Careful to avoid marshaling cycles! I.e., the callee
needing a field that has not been unmarshaled yet to correctly unmarshal some other field. Construct a marshaling deps graph?

```
rpc add_widget {
    call unsigned int (projection widget_desc [alloc(callee)]* widget, string* name);

    projection<union widget_content> content {
        float foo;
        unsigned long bar;
        // generated tags are U_WIDGET_CONTENT_CONTENT_FOO, U_WIDGET_CONTENT_CONTENT_BAR
        // we call get_widget_content_content_tag(), passing whatever contextual information is needed
    }

    projection<struct widget_desc> widget_desc {
        projection content data; // projection *value* here
        unsigned long n_childs;
        projection widget_desc*[n_childs] children; 
    }
}
```
