
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

	std::ostream& tab_over(unsigned int level, std::ostream& os)
	{
		for (unsigned int i {0}; i < level; ++i)
			os << "  ";

		return os;
	}

	void dump_type(unsigned int level, const type* type, std::ostream& os)
	{
		tab_over(level, os) << "type (\n";
		++level;

		tab_over(level, os) << "copy_type:\n";
		tab_over(level, os) << "attributes: TODO\n";
		tab_over(level, os) << "stars: " << type->stars() << "\n";

		--level;
		tab_over(level, os) << ")\n";
	}

	void dump_var_field(unsigned int level, const var_field* field, std::ostream& os)
	{
		tab_over(level, os) << "var_field (\n";
		++level;

		tab_over(level, os) << "identifier: " << field->identifier() << "\n";
		tab_over(level, os) << "type:\n";
		dump_type(level + 1, field->type(), os);

		--level;
		tab_over(level, os) << ")\n";
	}

	void dump_field(unsigned int level, const field* field, std::ostream& os)
	{
		tab_over(level, os) << "field (\n";
		++level;

		switch (field->kind()) {
		case field_kind::rpc:
			tab_over(level, os) << "kind: rpc\n";
			break;

		case field_kind::var:
			tab_over(level, os) << "kind: var\n";
			tab_over(level, os) << "_variant:\n";
			dump_var_field(level + 1, field->get<field_kind::var>().get(), os);
			break;
		}

		--level;
		tab_over(level, os) << ")\n";
	}

	void dump_projection(unsigned int level, const projection* proj, std::ostream& os)
	{
		tab_over(level, os) << "projection (\n";
		++level;

		tab_over(level, os) << "identifier: " << proj->identifier() << "\n";
		tab_over(level, os) << "real_type: " << proj->real_type() << "\n";
		tab_over(level, os) << "items: [\n";

		for (const auto& field : proj->fields()) {
			dump_field(level + 1, field.get(), os);
		}

		tab_over(level, os) << "]\n";

		--level;
		tab_over(level, os) << ")\n";
	}

	void dump_item(unsigned int level, const item* item, std::ostream& os)
	{
		tab_over(level, os) << "item (\n";
		++level;

		switch (item->kind()) {
		case item_kind::projection:
			tab_over(level, os) << "kind: projection\n";
			tab_over(level, os) << "_variant:\n";
			dump_projection(level + 1, item->get<item_kind::projection>(), os);
			break;

		case item_kind::rpc:
			tab_over(level, os) << "kind: rpc\n";
			break;
		}

		--level;
		tab_over(level, os) << ")\n";
	}

	void dump_module(unsigned int level, const module* mod, std::ostream& os)
	{
		tab_over(level, os) << "module (\n";
		++level;

		tab_over(level, os) << "identifier: " << mod->identifier() << "\n";
		tab_over(level, os) << "items: [\n";

		for (const auto& item : mod->items()) {
			dump_item(level + 1, item.get(), os);
		}

		tab_over(level, os) << "]\n";

		--level;
		tab_over(level, os) << ")\n";
	}

	void dump_file(unsigned int level, const file* file, std::ostream& os)
	{
		tab_over(level, os) << "file (\n";
		++level;

		tab_over(level, os) << "modules: [\n";

		for (const auto& mod : file->modules()) {
			dump_module(level + 1, mod.get(), os);
		}

		tab_over(level, os) << "]\n";

		--level;
		tab_over(level, os) << ")\n";
	}
}


int main(int argc, gsl::czstring<>* argv) {
	gsl::span args {argv, gsl::narrow_cast<std::size_t>(argc)};

	if (args.size() != 2 && args.size() != 3) {
		std::cout << "Usage: idlc <source-file> [<dump-file>]\n";
	}

	try {
		const auto top_node = std::unique_ptr<idlc::file> {(idlc::file*)Parser::parse(std::string(args[1]))};

		/*for (const auto& m : top_node->modules()) {
			std::cout << "module " << m->identifier() << "\n";
			idlc::walk_items(m->items());
		}*/

		if (args.size() == 3) {
			idlc::dump_file(0, top_node.get(), std::ofstream {args[2]});
		}

		return 0;
	}
	catch (const Parser::ParseException& e) {
		std::cerr << "\n\nALERT!!! - Caught parser exception" << std::endl;
		std::cerr << e.getReason() << std::endl;
		exit(0);
	}
}
