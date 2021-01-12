#include <string>
#include <iostream>
#include <variant>

#include <gsl/gsl>

#include "parser/idl_parse.h"
#include "ast/walk.h"
#include "sema/name_binding.h"
#include "sema/type_walk.h"
#include "sema/default_walk.h"

// NOTE: we keep the identifier heap around for basically the entire life of the compiler
// NOTE: Currently we work at the scale of a single file. All modules within a file are treated as implicitly imported.
// TODO: Support merging ASTs via import / use keywords.

/*
	Marshaling logic gets outputted as a per-passing-tree list of "blocks" that form a graph structure.
	These are essentially visitors, which visit the fields at runtime to correctly pack them into the message buffer.
	The goal is to encode enough information here that we could later do optimization passes over it. Such passes
	would only be justified if it's places where the compiler cannot feasibly optimize it for us, or places where
	said optimization improves debuggability. The major candidates are trampoline elision, static assignment of IPC
	registers, (??)
*/

/*
	Identifiers: we often require identifier variants. The important ones are for RPCs: we need the original, the RPC
	enum identifier, the RPC impl identifier, and the RPC trampoline identifier, if one is needed. There also the
	variants marshaling needs to give meaningful names to temporaries.
*/

/*
	NOTE: Marshaling

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
	
	Package as much as possible into helper functions, this greatly simplifies code generation
	- you can package bind / alloc / dealloc into helpers
*/

/*
	NOTE: front-end passes are irrelevant rn, work on:
	- annotation defaulting pass
	- marshal IR
	- 6 IR generation passes (!!)
	- code generation
*/

/*
	NOTE: Vikram notes:
	- initial pass recording IDs and types of every object in the graph
	- serialize to flattened array of registers, pointers replaced by IDs
	- unrealistic to build right now
	- focus on subset of features
		- static arrays
		- pointers (non-cyclic?)
		- unions
		- structs
		- minimum viable: focus on nullnet, waiting on vikram to generate IDL
*/

namespace idlc {
	namespace {
		constexpr auto enable_nullwalk = true;

		void dump_tree(idlc::ast::file& root)
		{
			if constexpr (enable_nullwalk) {
				ast::null_walk walk {};
				walk.visit_file(root);
			}
		}
	}
}

// Note that Roslyn appears to render the AST completely immutable, forcing node properties to be stored
// by association (hash table, red node, etc.)

// NOTE: Default to a walk, re-write later as needed (premature optimization, etc.)
// TODO: sort out the somewhat hellish logging situation

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

	if (!idlc::sema::bind_all(file)) {
		std::cout << "Error: Not all names were bound\n";
		return 1;
	}

	const auto data_fields = idlc::sema::generate_pgraphs(file);
	for (auto& [name, field]: data_fields) {
		std::cout << "[debug] For \"" << name << "\":\n";
		idlc::sema::null_walk pgraph_null {};
		if (!pgraph_null.visit_data_field(*field)) {
			std::cout << "[debug] Could not walk a pgraph\n";
			return 1;
		}
	}
}