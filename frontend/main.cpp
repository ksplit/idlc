#include "../parser/ast.h"
#include "../parser/parser.h"

#include <gsl/gsl>

#include <string>
#include <iostream>

namespace idlc {
    namespace {
        std::unique_ptr<idlc::idl_file> parse_driver(gsl::czstring<> path)
        {
            try {
                const auto raw_ptr = Parser::parse(std::string {path});
                const auto ptr = const_cast<void*>(raw_ptr);
                return std::unique_ptr<idlc::idl_file> {static_cast<idlc::idl_file*>(ptr)};
            }
            catch (const Parser::ParseException& e) {
                std::cout << e.getReason() << std::endl;
                return nullptr;
            }
        }
    }
}

int main(int argc, char** argv)
{
    const gsl::span<gsl::zstring<>> args {argv, gsl::narrow<std::size_t>(argc)};
    if (argc != 2) {
        std::cout << "Usage: idlc <idl-file>" << std::endl;
        return 1;
    }

    const auto driver_idl = idlc::parse_driver(gsl::at(args, 1));
    if (!driver_idl) {
        return 1;
    }

    idlc::null_ast_walk null {};
    if (!null.traverse_idl_file(*driver_idl)) {
        std::cout << "Walk failed" << std::endl;
        return 1;
    }
}