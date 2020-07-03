
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

	std::ostream& tab_over(unsigned int level, std::ostream& os)
	{
		for (unsigned int i {0}; i < level; ++i)
			os << " ";

		return os;
	}

	void dump_primitive_type(const primitive_type& pt, std::ostream& os, unsigned int level)
	{
		tab_over(level, os) << "primitive_type (\n";
		++level;

		switch (pt.kind()) {
		case primitive_type_kind::bool_k:
			tab_over(level, os) << "kind: bool\n";
			break;

		case primitive_type_kind::char_k:
			tab_over(level, os) << "kind: char\n";
			break;

		case primitive_type_kind::double_k:
			tab_over(level, os) << "kind: double\n";
			break;

		case primitive_type_kind::float_k:
			tab_over(level, os) << "kind: float\n";
			break;

		case primitive_type_kind::int_k:
			tab_over(level, os) << "kind: int\n";
			break;

		case primitive_type_kind::long_k:
			tab_over(level, os) << "kind: long\n";
			break;

		case primitive_type_kind::long_long_k:
			tab_over(level, os) << "kind: long long\n";
			break;

		case primitive_type_kind::short_k:
			tab_over(level, os) << "kind: short\n";
			break;

		case primitive_type_kind::unsigned_char_k:
			tab_over(level, os) << "kind: unsigned char\n";
			break;

		case primitive_type_kind::unsigned_int_k:
			tab_over(level, os) << "kind: unsigned int\n";
			break;

		case primitive_type_kind::unsigned_long_k:
			tab_over(level, os) << "kind: unsigned long\n";
			break;

		case primitive_type_kind::unsigned_long_long_k:
			tab_over(level, os) << "kind: usnigned long long\n";
			break;

		case primitive_type_kind::unsigned_short_k:
			tab_over(level, os) << "kind: unsigned short\n";
			break;
		}

		--level;
		tab_over(level, os) << ")\n";
	}

	void dump_field(const field& field, std::ostream& os, unsigned int level);

	void dump_signature(const signature& sig, std::ostream& os, unsigned int level)
	{
		tab_over(level, os) << "signature (\n";
		++level;

		tab_over(level, os) << "return_field:\n";
		dump_field(sig.return_field(), os, level + 1);

		tab_over(level, os) << "arguments: [\n";
		++level;

		for (const auto& arg : sig.arguments()) {
			dump_field(*arg, os, level);
		}

		--level;
		tab_over(level, os) << "]\n";

		--level;
		tab_over(level, os) << ")\n";
	}

	void dump_projection_type(const projection_type& pt, std::ostream& os, unsigned int level)
	{
		tab_over(level, os) << "projection_type (\n";
		++level;

		tab_over(level, os) << "identifier: " << pt.identifier() << "\n";

		--level;
		tab_over(level, os) << ")\n";
	}

	void dump_copy_type(const copy_type& ct, std::ostream& os, unsigned int level)
	{
		tab_over(level, os) << "copy_type (\n";
		++level;

		switch (ct.kind()) {
		case copy_type_kind::primitive:
			tab_over(level, os) << "kind: primitive\n";
			tab_over(level, os) << "_variant:\n";
			dump_primitive_type(ct.get<copy_type_kind::primitive>(), os, level + 1);
			break;

		case copy_type_kind::projection:
			tab_over(level, os) << "kind: projection\n";
			tab_over(level, os) << "_variant:\n";
			dump_projection_type(ct.get<copy_type_kind::projection>(), os, level + 1);
			break;
		}

		--level;
		tab_over(level, os) << ")\n";
	}

	void dump_type(const type& type, std::ostream& os, unsigned int level)
	{
		tab_over(level, os) << "type (\n";
		++level;

		if (type.get_copy_type()) {
			tab_over(level, os) << "copy_type:\n";
			dump_copy_type(*type.get_copy_type(), os, level + 1);
		}
		else {
			tab_over(level, os) << "copy_type: null\n";
		}

		if (type.get_attributes()) {
			tab_over(level, os) << "attributes:\n";
			// TODO
		}
		else {
			tab_over(level, os) << "attributes: null\n";
		}

		tab_over(level, os) << "stars: " << type.stars() << "\n";

		--level;
		tab_over(level, os) << ")\n";
	}

	void dump_var_field(const var_field& field, std::ostream& os, unsigned int level)
	{
		tab_over(level, os) << "var_field (\n";
		++level;

		if (field.identifier()) {
			tab_over(level, os) << "identifier: " << field.identifier() << "\n";
		}
		else {
			tab_over(level, os) << "identifier: null\n";
		}

		tab_over(level, os) << "type:\n";
		dump_type(field.get_type(), os, level + 1);

		--level;
		tab_over(level, os) << ")\n";
	}

	void dump_rpc_field(const rpc_field& field, std::ostream& os, unsigned int level)
	{
		tab_over(level, os) << "rpc_field (\n";
		++level;

		if (field.identifier()) {
			tab_over(level, os) << "identifier: " << field.identifier() << "\n";
		}
		else {
			tab_over(level, os) << "identifier: null\n";
		}

		tab_over(level, os) << "signature:\n";
		dump_signature(field.get_signature(), os, level + 1);

		--level;
		tab_over(level, os) << ")\n";
	}

	void dump_field(const field& field, std::ostream& os, unsigned int level)
	{
		tab_over(level, os) << "field (\n";
		++level;

		switch (field.kind()) {
		case field_kind::rpc:
			tab_over(level, os) << "kind: rpc\n";
			tab_over(level, os) << "_variant:\n";
			dump_rpc_field(field.get<field_kind::rpc>(), os, level + 1);
			break;

		case field_kind::var:
			tab_over(level, os) << "kind: var\n";
			tab_over(level, os) << "_variant:\n";
			dump_var_field(field.get<field_kind::var>(), os, level + 1);
			break;
		}

		--level;
		tab_over(level, os) << ")\n";
	}

	void dump_projection(const projection& proj, std::ostream& os, unsigned int level)
	{
		tab_over(level, os) << "projection (\n";
		++level;

		tab_over(level, os) << "identifier: " << proj.identifier() << "\n";
		tab_over(level, os) << "real_type: " << proj.real_type() << "\n";
		tab_over(level, os) << "items: [\n";

		for (const auto& field : proj.fields()) {
			dump_field(*field, os, level + 1);
		}

		tab_over(level, os) << "]\n";

		--level;
		tab_over(level, os) << ")\n";
	}

	void dump_rpc(const rpc& rpc, std::ostream& os, unsigned int level)
	{
		tab_over(level, os) << "rpc_field (\n";
		++level;

		tab_over(level, os) << "identifier: " << rpc.identifier() << "\n";
		tab_over(level, os) << "signature:\n";
		dump_signature(rpc.get_signature(), os, level + 1);

		--level;
		tab_over(level, os) << ")\n";
	}

	void dump_item(const item& item, std::ostream& os, unsigned int level)
	{
		tab_over(level, os) << "item (\n";
		++level;

		switch (item.kind()) {
		case item_kind::projection:
			tab_over(level, os) << "kind: projection\n";
			tab_over(level, os) << "_variant:\n";
			dump_projection(item.get<item_kind::projection>(), os, level + 1);
			break;

		case item_kind::rpc:
			tab_over(level, os) << "kind: rpc\n";
			tab_over(level, os) << "_variant:\n";
			dump_rpc(item.get<item_kind::rpc>(), os, level + 1);
			break;
		}

		--level;
		tab_over(level, os) << ")\n";
	}

	void dump_module(const module* mod, std::ostream& os, unsigned int level)
	{
		tab_over(level, os) << "module (\n";
		++level;

		tab_over(level, os) << "identifier: " << mod->identifier() << "\n";
		tab_over(level, os) << "items: [\n";

		for (const auto& item : mod->items()) {
			dump_item(*item, os, level + 1);
		}

		tab_over(level, os) << "]\n";

		--level;
		tab_over(level, os) << ")\n";
	}

	void dump_file(const file& file, std::ostream& os, unsigned int level = 0)
	{
		tab_over(level, os) << "file (\n";
		++level;

		tab_over(level, os) << "modules: [\n";

		for (const auto& mod : file.modules()) {
			dump_module(mod.get(), os, level + 1);
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
			std::ofstream dump {args[2]};
			idlc::dump_file(*top_node, dump);
		}

		return 0;
	}
	catch (const Parser::ParseException& e) {
		std::cerr << "\n\nALERT!!! - Caught parser exception" << std::endl;
		std::cerr << e.getReason() << std::endl;
		exit(0);
	}
}
