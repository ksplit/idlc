#include <string>
#include <iostream>
#include <variant>

#include <gsl/gsl>

#include "./parser/idl_parse.h"
#include "./parser/walk.h"
#include "./marshal/tree.h"
#include "resolution.h"

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
*/

int main(int argc, char** argv)
{
    const gsl::span<gsl::zstring<>> args {argv, gsl::narrow<std::size_t>(argc)};
    if (argc != 2) {
        std::cout << "Usage: idlc <idl-file>" << std::endl;
        return 1;
    }

    const auto driver_idl = idlc::parser::parse_file(gsl::at(args, 1));
    if (!driver_idl) {
        return 1;
    }

	const auto& file = *driver_idl;
	std::cout << "File was parsed correctly" << std::endl;

	idlc::scopes_pass scope_walk {};
	idlc::names_pass walk {scope_walk.type_scopes_, scope_walk.val_scopes_};
	traverse_file(scope_walk, file);
	traverse_file(walk, file);

	for (const auto& [key, scope] : scope_walk.type_scopes_) {
		std::cout << "Type Scope for node " << key << ":\n";
		std::cout << "\tStructs:\n";
		for (const auto& [key, node] : scope->structs)
			std::cout << "\t\t" << key << " (" << node << ")\n";

		std::cout << "\tUnions:\n";
		for (const auto& [key, node] : scope->unions)
			std::cout << "\t\t" << key << " (" << node << ")\n";

		std::cout << "\tRPC pointers:\n";
		for (const auto& [key, node] : scope->rpcs)
			std::cout << "\t\t" << key << " (" << node << ")\n";
	}

	for (const auto& [key, scope] : scope_walk.val_scopes_) {
		std::cout << "Value Scope for node " << key << ":\n";
		std::cout << "\tNaked decls:\n";
		for (const auto& [key, node] : scope->naked)
			std::cout << "\t\t" << key << " (" << node << ")\n";

		std::cout << "\tVar decls:\n";
		for (const auto& [key, node] : scope->vars)
			std::cout << "\t\t" << key << " (" << node << ")\n";
	}
}