#ifndef _LCDS_IDL_PARSER_IDL_PARSE_H_
#define _LCDS_IDL_PARSER_IDL_PARSE_H_

#include <any>
#include <iostream>

#include <gsl/gsl>

#include "ast.h"
#include "parser.h"
#include "parse_globals.h"
	
namespace idlc::parser {
	inline std::shared_ptr<file> parse_file(gsl::czstring<> path)
	{
		try {
			const auto raw = Parser::parse(std::string {path});
			const auto id = reinterpret_cast<std::size_t>(raw);
			const auto ref = std::any_cast<std::shared_ptr<file>>(gsl::at(parser_objs, id));
			
			// Now that we have the tree root we can GC all the unused nodes
			parser_objs.clear();

			return ref;
		}
		catch (const Parser::ParseException& e) {
			std::cout << e.getReason() << std::endl;
			return nullptr;
		}
		catch (const std::exception& e) {
			std::cout << e.what() << std::endl;
			return nullptr;
		}
	}
}

#endif
