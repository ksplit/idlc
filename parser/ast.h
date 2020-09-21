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
    enum class attribute_set : std::uint8_t;
    struct type_name;

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

    enum class attribute_set : std::uint8_t {
        none = 0b00000000,
        alloc_caller = 0b00000001,
        alloc_callee = 0b00000010,
        dealloc_caller = 0b00000100,
        dealloc_callee = 0b00001000,
        bind_caller = 0b00010000,
        bind_callee = 0b00100000,
        in = 0b01000000,
        out = 0b10000000
    };

    inline attribute_set operator|(attribute_set flags, attribute_set new_flags)
    {
        return static_cast<attribute_set>(static_cast<std::uint8_t>(flags) | static_cast<std::uint8_t>(new_flags));
    }

    inline attribute_set operator&(attribute_set flags, attribute_set new_flags)
    {
        return static_cast<attribute_set>(static_cast<std::uint8_t>(flags) & static_cast<std::uint8_t>(new_flags));
    }

    inline attribute_set operator~(attribute_set flags)
    {
        return static_cast<attribute_set>(~static_cast<std::uint8_t>(flags));
    }

    inline attribute_set& operator|=(attribute_set& flags, attribute_set new_flags)
    {
        flags = flags | new_flags;
    }

    inline attribute_set& operator&=(attribute_set& flags, attribute_set new_flags)
    {
        flags = flags & new_flags;
    }

    struct type_name {
        const std::vector<attribute_set> stars;
        const stem_type stem;
        const attribute_set attributes; // Conceptually, the "value" attributes
    };

    struct variable {
        const type_name type;
        const gsl::czstring<> id;
    };

    struct signature {
        const type_name ret_type;
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
