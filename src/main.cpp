#include <algorithm>
#include <fstream>
#include <string>
#include <iostream>
#include <variant>

#include <gsl/gsl>

#include <absl/strings/string_view.h>

#include "parser/idl_parse.h"
#include "ast/pgraph_walk.h"
#include "frontend/name_binding.h"
#include "frontend/analysis.h"
#include "utility.h"
#include "generation.h"

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
	NOTE: Vikram notes:
	- initial pass recording IDs and types of every object in the graph
	- serialize to flattened array of registers, pointers replaced by IDs
	- unrealistic to build right now
	- focus on subset of features
		- static arrays
		- pointers (non-cyclic?)
		- unions
		- structs
		- minimum viable: focus on nullnet
*/

// NOTE: All of these are low-priority, but eventually needed (if it breaks nullnet, it's high-priority)
// TODO: sort out the somewhat hellish logging situation
// TODO: sort out const-ness handling (low-priority, work around in mean time)

// NOTE: all marshaling logic is side-independent, needing only a side-dependent send() primitive
// It's side-dependent where the dispatch loop gets hooked in, however
// and kernel functions must not conflict with the generated marshaling code
// it is the indirect RPCs that are truly side-independent



// TODO: move code generation into its own module
// TODO: add static_void_ptr syntax (low priority)
// TODO: dynamic arrays, and how they must be ordered relative to their size field (which must be adjacent!)
// TODO: ndo_start_xmit uncovered an issue with enum promotion not being applicable to function pointers, i.e. the IDL
// needs a concept of enums (so does the pgraph, etc.)
// TODO: pointer annotation handling
// TODO: correctly passing array sizes (and using them in unmarshaling)

int main(int argc, char** argv)
{
    const gsl::span<gsl::zstring<>> args {argv, gsl::narrow<std::size_t>(argc)};
    if (argc != 2) {
        std::cout << "Usage: idlc <idl-file>" << std::endl;
        return 1;
    }

    const auto file = idlc::parser::parse_file(gsl::at(args, 1));
    if (!file)
        return 1;

	if (!idlc::bind_all_names(*file)) {
		std::cout << "Error: Not all names were bound\n";
		return 1;
	}

	const auto rpcs = idlc::geerate_rpc_pgraphs(*file);
	if (!rpcs) {
		std::cout << "Error: pgraph generation failed\n";
		return 1;
	}

	idlc::generate(*rpcs);
}