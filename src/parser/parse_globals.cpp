#include "parse_globals.h"

#include <any>
#include <vector>

#include "string_heap.h"

std::vector<std::any> idlc::parser::parser_objs(1); // Dummy object at index 0 to catch sentinel values
idlc::string_heap idlc::parser::idents {};
