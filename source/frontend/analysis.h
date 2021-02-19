#ifndef IDLC_SEMA_ANALYSIS_H
#define IDLC_SEMA_ANALYSIS_H

#include <optional>
#include <vector>

#include <gsl/gsl>

#include "../string_heap.h"
#include "../ast/ast.h"
#include "../ast/ast_walk.h"
#include "../ast/pgraph.h"

namespace idlc {
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

	using rpc_vec = std::vector<gsl::not_null<rpc_def*>>;
	using rpc_vec_view = gsl::span<const gsl::not_null<rpc_def*>>;

	std::optional<rpc_vec> generate_rpc_pgraphs(file& root);
}

#endif
