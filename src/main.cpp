#include <string>
#include <iostream>
#include <variant>

#include <gsl/gsl>

#include "parser/idl_parse.h"
#include "ast/walk.h"
#include "sema/scope_walk.h"
#include "sema/type_walk.h"

// NOTE: we keep the identifier heap around for basically the entire life of the compiler

/*
	There's not really a need for AST re-writing as of yet, the resolution tasks can be done in-place,
	and the passing tree IR will most likely replace it in later stages. Passing tree gets rooted
	in a particular rpc node, which contains enough information to generate correctly-named stubs.
	An rpc node *could* be tagged as a function pointer, which slightly modifies how the stub is generated,
	adds a trampoline for it, and how the impl itself is called, but otherwise does not modify the marshaling logic.

	Marshaling logic gets outputted as a per-passing-tree list of "blocks" that form a graph structure.
	These are essentially visitors, which visit the fields at runtime to correctly pack them into the message buffer.
	The goal is to encode enough information here that we could later do optimization passes over it. Such passes
	would only be justified if it's places where the compiler cannot feasibly optimize it for us, or places where
	said optimization improves debuggability. The major candidates are trampoline elision, static assignment of IPC
	registers, 

	Identifiers: we often require identifier variants. The important ones are for RPCs: we need the original, the RPC
	enum identifier, the RPC impl identifier, and the RPC trampoline identifier, if one is needed. There also the
	variants marshaling needs to give meaningful names to temporaries.

	As for outptutting C code for marshaling instructions: we've run into the issue before of having to wrap function
	pointer types in awkward ways. IIRC, we needed to know the pointer types of RPC pointers in a very "full" way,
	both the specifier form and the actual type name. Check lvd_linux's generated drivers for more info.

	These can be handled similar to variants: pre-computed strings that we can generate in batches. We could generate
	the type "forms" on a per-node basis in the passing trees, and simply cache that.

	These type specifiers / declarators and the identifier variants could be done during the C output stage, or as part
	of marshalling generation: temporary variable names could be decided during IR generation, or perhaps the IR itself
	encodes enough information to generate that later.

	Note that each IR "block" will end up generating a marshal_* function.
	Each block is per-projection, and RPC-scoped projections will get names correspondingly prepended with the RPC name

	Follow the valgrind conventions on efficiency, preferably

	NOTE: front-end passes are irrelevant rn, work on:
	- annotation defaulting pass
	- pass-graph building
	- marshal IR
	- 6 IR generation passes (!!)
	- code generation

	Package as much as possible into helper functions, this greatly simplifies code generation
	- you can package bind / alloc / dealloc into helpers

	Vikram notes:
	- initial pass recording IDs and types of every object in the graph
	- serialize to flattened array of registers, pointers replaced by IDs
	- unrealistic to build right now
	- focus on subset of features
		- static arrays
		- pointers (non-cyclic?)
		- unions
		- structs
		- minimum viable: focus on nullnet, waiting on vikram to generate IDL

	NOTE: Currently we work at the scale of a single file. All modules within a file are treated as implicitly imported.
	TODO: Support merging ASTs via import / use keywords.

	Roslyn red-green trees may be useful in some places
*/

namespace idlc {
	namespace {
		constexpr auto enable_nullwalk = false;

		void dump_tree(idlc::ast::file& root)
		{
			if constexpr (enable_nullwalk) {
				ast::null_walk walk {};
				walk.visit_file(root);
			}
		}

		auto create_type_pgraphs(idlc::ast::file& file)
		{
			sema::type_walk walk {};
			walk.visit_file(file);
			return walk.get_pgraph_owner();
		}
	}
}

// TODO: we could store projection pgraphs in-tree, would avoid having to engineer a separate DB,
// could possibly help with any future extension efforts

// Note that Roslyn appears to render the AST completely immutable, forcing node properties to be stored
// by association (hash table, red node, etc.)

// NOTE: Default to a walk, re-write later as needed (premature optimization, etc.)

int main(int argc, char** argv)
{
    const gsl::span<gsl::zstring<>> args {argv, gsl::narrow<std::size_t>(argc)};
    if (argc != 2) {
        std::cout << "Usage: idlc <idl-file>" << std::endl;
        return 1;
    }

    const auto driver_idl = idlc::parser::parse_file(gsl::at(args, 1));
    if (!driver_idl)
        return 1;

	auto& file = *driver_idl;
	std::cout << "[parse] File was parsed correctly" << std::endl;
	idlc::dump_tree(file);

	idlc::sema::symbol_walk walk {};
	if (!walk.visit_file(file)) {
		std::cout << "[debug] symbol_walk failed\n";
		return 1;
	}

	idlc::sema::bind_walk bind_walk {};
	if (!bind_walk.visit_file(file)) {
		std::cout << "[debug] Not all names were bound\n";
		return 1;
	}

	const auto data_fields = idlc::create_type_pgraphs(file);
}