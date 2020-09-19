#pragma once

#include <memory>
#include <optional>
#include <variant>
#include <type_traits>
#include <vector>

#include <gsl/gsl>

namespace idlc {
    struct void_type;
    struct driver_definition;
    struct module_import;
    struct header_import;
    struct rpc_definition;
    struct signature;
    struct struct_projection_definition;
    struct idl_include;
    struct driver_file;
    struct module_file;
    struct module_definition;
    enum class primitive_type;
    struct string_type;
    struct projection_type;
    struct rpc_type;
    struct variable;
    struct attribute_set;
    struct type_specifier;

    using idl_file = std::variant<driver_file, module_file>;
    using rpc_definition_subitem = std::variant<struct_projection_definition, signature>;
    using module_subitem = std::variant<header_import, rpc_definition, struct_projection_definition>;
    using stem_type = std::variant<void_type, rpc_type, primitive_type, projection_type, string_type>;

    struct string_type {};

    enum class primitive_type {
        type_char,
        type_uchar,
        type_schar,
        type_short,
        type_ushort,
        type_int,
        type_uint,
        type_long,
        type_ulong,
        type_longlong,
        type_ulonglong
    };

    struct projection_type {
        const gsl::czstring<> id;
    };

    struct rpc_type {
        const gsl::czstring<> id;
    };

    struct void_type {};

    enum class passing_direction {
        in,
        out
    };

    enum class pointer_action {
        alloc_caller,
        alloc_callee,
        dealloc_caller,
        dealloc_callee,
        bind_caller,
        bind_callee
    };

    struct attribute_set {
        const passing_direction direction;
        const pointer_action action;
    };

    struct type_specifier {
        const stem_type stem;
        const std::vector<attribute_set> stars;
    };

    struct variable {
        const type_specifier type;
        const gsl::czstring<> id;
    };

    struct signature {
        const type_specifier ret_type;
        const std::vector<variable> arguments;
    };

    struct struct_projection_definition {
        const gsl::czstring<> id;
        const gsl::czstring<> underlying;
        const std::vector<variable> fields;
    };

    struct rpc_definition {
        const gsl::czstring<> id;
        const std::vector<rpc_definition_subitem> items;
    };

    struct header_import {
        const gsl::czstring<> raw_path;
    };

    struct idl_include {
        const gsl::czstring<> raw_path;
    };

    struct module_definition {
        const gsl::czstring<> id;
        const std::vector<module_subitem> items;
    };

    struct driver_definition {
        const gsl::czstring<> id;
        const std::vector<module_import> imports;
    };

    struct driver_file {
        const driver_definition driver;
        const std::vector<idl_include> includes;
    };

    struct module_import {
        const gsl::czstring<> id;
    };

    struct module_file {
        const std::vector<module_definition> modules;
    };
}
