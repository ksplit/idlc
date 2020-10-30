#pragma once

#include <cassert>
#include <memory>
#include <optional>
#include <variant>
#include <type_traits>
#include <vector>

#include <gsl/gsl>

/*
	How much lowering should the parser even be doing?
*/

namespace idlc::ast {
    struct driver_file;
	struct module_file;
	struct driver_def;
	struct module_def;
	struct module_import;
	struct idl_include;
	struct projection_def;
	struct rpc_def;
	struct var_decl;
	struct rpc_signature;
	struct type_name;
	struct stem_type;
	struct void_type;
	struct array_type;
	struct string_type;
	struct projection_type;
	struct primitive_type;
}
