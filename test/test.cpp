#include "../parser/ast.h"
#include "../parser/parser.h"

#include <gsl/gsl>

#include <string>

int main()
{
    Parser::parse(std::string {"driver.idl"});
}
