
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <fstream>

#include "../parser/parser.h"
#include "../parser/ast.h"

namespace idlc
{
	// RPCs and RPC pointers, though different nodes, each have a signature, used to generate their marshaling info

	void walk_items(gsl::span<const std::unique_ptr<item>> items)
	{
		for (const auto& item : items) {
			switch (item->kind()) {
			case idlc::item_kind::rpc:
				std::cout << "rpc ";
				std::cout << item->get<idlc::item_kind::rpc>()->identifier();
				std::cout << "\n";
				break;

			case idlc::item_kind::projection:
				const auto& proj = item->get<idlc::item_kind::projection>();
				std::cout << "projection ";
				std::cout << proj->identifier();
				std::cout << ", struct ";
				std::cout << proj->real_type();
				std::cout << "\n";
				break;
			}
		}
	}
} // identifier()space idlc


int main(int argc, char **argv) {
	char *file;
	bool test_mode;
	file = argv[1];
	test_mode = false;

	try {
		const auto top_node = std::unique_ptr<idlc::file> {(idlc::file*)Parser::parse(std::string(file))};
		
		for (const auto& m : top_node->modules()) {
			std::cout << "module " << m->identifier() << "\n";
			idlc::walk_items(m->items());
		}

		return 0;
	} catch (const Parser::ParseException &e) {
		std::cerr << "\n\nALERT!!! - Caught parser exception" << std::endl;
		std::cerr << e.getReason() << std::endl;
		exit(0);
	}
}
