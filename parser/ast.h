#pragma once

#include <cassert>
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
    struct union_projection_definition;
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

    using id_ref = gsl::not_null<gsl::czstring<>>;

    /*
        FIXME: if we ever end up needing to modify the tree frequently, it will become useful to put every node
        behind a unique_ptr. If we intend the tree to be completely immutable, we'll probably end up being better off
        doing such a pointer-based layout anyways, but allocating them depth-first on a stack allocator.
    */

    using idl_file = std::variant<driver_file, module_file>;
    using rpc_definition_subitem = std::variant<struct_projection_definition, union_projection_definition, signature>;
    using stem_type = std::variant<void_type, rpc_type, primitive_type, projection_type, string_type>;
    using module_subitem = std::variant<
        header_import,
        rpc_definition,
        struct_projection_definition,
        union_projection_definition
    >;

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
        const id_ref id;
    };

    struct rpc_type {
        const id_ref id;
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
        return flags;
    }

    inline attribute_set& operator&=(attribute_set& flags, attribute_set new_flags)
    {
        flags = flags & new_flags;
        return flags;
    }

    struct type_name {
        const std::vector<attribute_set> stars;
        const stem_type stem;
        const attribute_set attributes; // Conceptually, the "value" attributes
    };

    struct variable {
        const type_name type;
        const id_ref id;
    };

    struct signature {
        const type_name ret_type;
        const std::vector<variable> arguments;
    };

    struct struct_projection_definition {
        const id_ref id;
        const id_ref underlying;
        const std::vector<variable> fields;
    };

    struct union_projection_definition {
        const id_ref id;
        const id_ref underlying;
        const std::vector<id_ref> value_ref;
        const std::vector<variable> fields;
    };

    struct rpc_definition {
        const id_ref id;
        const std::vector<rpc_definition_subitem> items;
    };

    struct header_import {
        const id_ref raw_path;
    };

    struct idl_include {
        const id_ref raw_path;
    };

    struct module_definition {
        const id_ref id;
        const std::vector<module_subitem> items;
    };

    struct driver_definition {
        const id_ref id;
        const std::vector<module_import> imports;
    };

    struct driver_file {
        const driver_definition driver;
        const std::vector<idl_include> includes;
    };

    struct module_import {
        const id_ref id;
    };

    struct module_file {
        const std::vector<module_definition> modules;
    };

    template<typename derived>
    class ast_visitor {
    public:
        bool traverse_idl_file(idl_file& node)
        {
            const auto visitor = [this](auto&& subnode)
            {
                if constexpr (std::is_same_v<decltype(subnode), driver_file>) {
                    if (!self().traverse_driver_file(subnode))
                        return false;
                }
                else if constexpr (std::is_same_v<decltype(subnode), module_file>) {
                    if (!self().traverse_module_file(subnode))
                        return false;
                }
                else {
                    assert(false);
                }

                return true;
            };

            return std::visit(visitor, node);
        }

        bool traverse_driver_file(driver_file& node)
        {
            if (!self().traverse_driver_definition(node.driver))
                return false;

            for (auto& subnode : node.includes) {
                if (!self().traverse_idl_include(subnode))
                    return false;
            }

            return true;
        }

        bool traverse_driver_definition(driver_definition& node)
        {
            for (auto& subnode : node.imports) {
                if (!self().traverse_module_import(subnode))
                    return false;
            }

            return true;
        }

        bool traverse_idl_include(idl_include& node)
        {
            return true;
        }

        bool traverse_module_file(module_file& node)
        {
            for (auto& subnode : node.modules) {
                if (!self().traverse_module_definition(node))
                    return false;
            }

            return true;
        }

        bool traverse_module_definition(module_definition& node)
        {
            for (auto& subnode : node.items) {
                if (!self().traverse_module_subitem(node))
                    return false;
            }

            return true;
        }

        bool traverse_module_import(module_import& node)
        {
            return true;
        }

        bool traverse_module_subitem(module_subitem& node)
        {
            const auto visitor = [this](auto&& subnode) {
                if constexpr (std::is_same_v<decltype(subnode), header_import>) {
                    if (!self().traverse_header_import(subnode))
                        return false;
                }
                else if constexpr (std::is_same_v<decltype(subnode), rpc_definition>) {
                    if (!self().traverse_rpc_definition(subnode))
                        return false;
                }
                else if constexpr (std::is_same_v<decltype(subnode), struct_projection_definition>) {
                    if (!self().traverse_struct_projection_definition(subnode))
                        return false;
                }
                else if constexpr (std::is_same_v<decltype(subnode), union_projection_definition>) {
                    if (!self().traverse_union_projection_definition(subnode))
                        return false;
                }
                else {
                    assert(false);
                }

                return true;
            };

            return std::visit(visitor, node);
        }

        bool traverse_header_import(header_import& node)
        {
            return true;
        }

        bool traverse_rpc_definition(rpc_definition& node)
        {
            // TODO:
            return true;
        }

        bool traverse_struct_projection_definition(struct_projection_definition& node)
        {
            for (auto& subnode : node.fields) {
                if (!self().traverse_variable(subnode))
                    return false;
            }

            return true;
        }

        bool traverse_union_projection_definition(union_projection_definition& node)
        {
            for (auto& subnode : node.fields) {
                if (!self().traverse_variable(subnode))
                    return false;
            }

            return true;
        }

        bool traverse_variable(variable& node)
        {
            if (!self().traverse_type_name(node.type))
                return false;

            return true;
        }

        bool traverse_type_name(type_name& node)
        {
            if (!self().traverse_stem_type(node.stem))
                return false;

            return true;
        }

        bool traverse_stem_type(stem_type& node)
        {
            const auto visitor = [this](auto&& subnode) {
                if constexpr (std::is_same_v<decltype(subnode), void_type>) {
                    if (!traverse_void_type(subnode))
                        return false;
                }
                else if constexpr (std::is_same_v<decltype(subnode), rpc_type>) {
                    if (!traverse_rpc_type(subnode))
                        return false;
                }
                else if constexpr (std::is_same_v<decltype(subnode), primitive_type>) {
                    if (!traverse_primitive_type(subnode))
                        return false;
                }
                else if constexpr (std::is_same_v<decltype(subnode), projection_type>) {
                    if (!traverse_projection_type(subnode))
                        return false;
                }
                else if constexpr (std::is_same_v<decltype(subnode), string_type>) {
                    if (!traverse_string_type(subnode))
                        return false;
                }
                else {
                    assert(false);
                }
            };

            return std::visit(visitor, node);
        }

        bool traverse_void_type(void_type& node)
        {
            return true;
        }

        bool traverse_rpc_type(rpc_type& node)
        {
            return true;
        }

        bool traverse_primitive_type(primitive_type& node)
        {
            return true;
        }

        bool traverse_projection_type(projection_type& node)
        {
            return true;
        }

        bool traverse_string_type(string_type& node)
        {
            return true;
        }

    private:
        derived& self()
        {
            // This is 100% kosher CRTP
            return *reinterpret_cast<derived*>(this);
        }
    };

    class null_ast_walk : public ast_visitor<null_ast_walk> {

    };
}
