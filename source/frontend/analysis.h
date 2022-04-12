#ifndef IDLC_SEMA_ANALYSIS_H
#define IDLC_SEMA_ANALYSIS_H

#include <optional>
#include <vector>

#include <gsl/gsl>

#include "../ast/ast.h"
#include "../ast/ast_walk.h"
#include "../ast/pgraph.h"
#include "../string_heap.h"

namespace idlc {
	using rpc_vec = std::vector<gsl::not_null<rpc_def*>>;
	using rpc_vec_view = gsl::span<const gsl::not_null<rpc_def*>>;
	using global_vec = std::vector<gsl::not_null<global_def*>>;
	using global_vec_view = gsl::span<const gsl::not_null<global_def*>>;
	std::optional<std::tuple<rpc_vec, global_vec>> generate_all_pgraphs(file& root);
}

#endif
