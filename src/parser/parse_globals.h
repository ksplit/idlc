#ifndef _LCDS_IDL_PARSE_GLOBALS_H_
#define _LCDS_IDL_PARSE_GLOBALS_H_

#include <vector>
#include <any>

#include "string_heap.h"

namespace idlc::parser
{
	extern std::vector<std::any> parser_objs; // Dummy object at index 0 to catch sentinel values
	extern idlc::string_heap idents;
}


#endif
