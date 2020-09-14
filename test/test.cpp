#include "../parser/ast.h"
#include "../parser/parser.h"

#include <gsl/gsl>

#include <string>
#include <iostream>

int main(int argc, char** argv)
{
    const gsl::span<gsl::zstring<>> args {argv, gsl::narrow<std::size_t>(argc)};
    if (argc != 2) {
        std::cout << "Usage: idlc <idl-file>" << std::endl;
        return 1;
    }

    try {
        Parser::parse(std::string {gsl::at(args, 1)});
    }
    catch (const Parser::ParseException& e) {
        std::cout << e.getReason() << std::endl;
    }
}
