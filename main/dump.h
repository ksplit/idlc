#ifndef _DUMP_H_
#define _DUMP_H_

#include <ostream>
#include "../parser/ast.h"

namespace idlc {
	std::ostream& tab_over(unsigned int level, std::ostream& os)
	{
		for (unsigned int i {0}; i < level; ++i)
			os << "  ";

		return os;
	}

	void dump(const primitive_type& pt, std::ostream& os, unsigned int level)
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

	void dump(const field& field, std::ostream& os, unsigned int level);

	void dump(const signature& sig, std::ostream& os, unsigned int level)
	{
		tab_over(level, os) << "signature (\n";
		++level;

		tab_over(level, os) << "return_field:\n";
		dump(sig.return_field(), os, level + 1);

		tab_over(level, os) << "arguments: [\n";
		++level;

		for (const auto& arg : sig.arguments()) {
			dump(*arg, os, level);
		}

		--level;
		tab_over(level, os) << "]\n";

		--level;
		tab_over(level, os) << ")\n";
	}

	void dump(const projection_type& pt, std::ostream& os, unsigned int level)
	{
		tab_over(level, os) << "projection_type (\n";
		++level;

		tab_over(level, os) << "identifier: " << pt.identifier() << "\n";

		--level;
		tab_over(level, os) << ")\n";
	}

	void dump(const copy_type& ct, std::ostream& os, unsigned int level)
	{
		tab_over(level, os) << "copy_type (\n";
		++level;

		switch (ct.kind()) {
		case copy_type_kind::primitive:
			tab_over(level, os) << "kind: primitive\n";
			tab_over(level, os) << "value:\n";
			dump(ct.get<copy_type_kind::primitive>(), os, level + 1);
			break;

		case copy_type_kind::projection:
			tab_over(level, os) << "kind: projection\n";
			tab_over(level, os) << "value:\n";
			dump(ct.get<copy_type_kind::projection>(), os, level + 1);
			break;
		}

		--level;
		tab_over(level, os) << ")\n";
	}

	void dump(const type& type, std::ostream& os, unsigned int level)
	{
		tab_over(level, os) << "type (\n";
		++level;

		if (type.get_copy_type()) {
			tab_over(level, os) << "copy_type:\n";
			dump(*type.get_copy_type(), os, level + 1);
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

	void dump(const var_field& field, std::ostream& os, unsigned int level)
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
		dump(field.get_type(), os, level + 1);

		--level;
		tab_over(level, os) << ")\n";
	}

	void dump(const rpc_field& field, std::ostream& os, unsigned int level)
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
		dump(field.get_signature(), os, level + 1);

		--level;
		tab_over(level, os) << ")\n";
	}

	void dump(const field& field, std::ostream& os, unsigned int level)
	{
		tab_over(level, os) << "field (\n";
		++level;

		switch (field.kind()) {
		case field_kind::rpc:
			tab_over(level, os) << "kind: rpc\n";
			tab_over(level, os) << "value:\n";
			dump(field.get<field_kind::rpc>(), os, level + 1);
			break;

		case field_kind::var:
			tab_over(level, os) << "kind: var\n";
			tab_over(level, os) << "value:\n";
			dump(field.get<field_kind::var>(), os, level + 1);
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
			dump(*field, os, level + 1);
		}

		tab_over(level, os) << "]\n";

		--level;
		tab_over(level, os) << ")\n";
	}

	void dump_rpc(const rpc& rpc, std::ostream& os, unsigned int level)
	{
		tab_over(level, os) << "rpc (\n";
		++level;

		tab_over(level, os) << "identifier: " << rpc.identifier() << "\n";
		tab_over(level, os) << "signature:\n";
		dump(rpc.get_signature(), os, level + 1);

		--level;
		tab_over(level, os) << ")\n";
	}

	void dump(const require& req, std::ostream& os, unsigned int level)
	{
		tab_over(level, os) << "require (\n";
		++level;

		tab_over(level, os) << "identifier: " << req.identifier() << "\n";

		--level;
		tab_over(level, os) << ")\n";
	}

	void dump(const module_item& item, std::ostream& os, unsigned int level)
	{
		tab_over(level, os) << "module_item (\n";
		++level;

		switch (item.kind()) {
		case module_item_kind::projection:
			tab_over(level, os) << "kind: projection\n";
			tab_over(level, os) << "value:\n";
			dump_projection(item.get<module_item_kind::projection>(), os, level + 1);
			break;

		case module_item_kind::rpc:
			tab_over(level, os) << "kind: rpc\n";
			tab_over(level, os) << "value:\n";
			dump_rpc(item.get<module_item_kind::rpc>(), os, level + 1);
			break;

		case module_item_kind::require:
			tab_over(level, os) << "kind: require\n";
			tab_over(level, os) << "value:\n";
			dump(item.get<module_item_kind::require>(), os, level + 1);
			break;
		}

		--level;
		tab_over(level, os) << ")\n";
	}

	void dump(const module& mod, std::ostream& os, unsigned int level)
	{
		tab_over(level, os) << "module (\n";
		++level;

		tab_over(level, os) << "identifier: " << mod.identifier() << "\n";
		tab_over(level, os) << "items: [\n";

		for (const auto& item : mod.items()) {
			dump(*item, os, level + 1);
		}

		tab_over(level, os) << "]\n";

		--level;
		tab_over(level, os) << ")\n";
	}

	void dump(const include& inc, std::ostream& os, unsigned int level)
	{
		tab_over(level, os) << "include (\n";
		++level;

		tab_over(level, os) << "path: " << inc.path() << "\n";

		--level;
		tab_over(level, os) << ")\n";
	}

	void dump(const file_item& fi, std::ostream& os, unsigned int level)
	{
		tab_over(level, os) << "file_item (\n";
		++level;

		switch (fi.kind()) {
		case file_item_kind::include:
			tab_over(level, os) << "kind: include\n";
			tab_over(level, os) << "_value:\n";
			dump(fi.get<file_item_kind::include>(), os, level + 1);
			break;

		case file_item_kind::module:
			tab_over(level, os) << "kind: module\n";
			tab_over(level, os) << "_value:\n";
			dump(fi.get<file_item_kind::module>(), os, level + 1);
			break;
		}

		--level;
		tab_over(level, os) << ")\n";
	}

	void dump(const file& file, std::ostream& os, unsigned int level = 0)
	{
		tab_over(level, os) << "file (\n";
		++level;

		tab_over(level, os) << "items: [\n";

		for (const auto& i : file.items()) {
			dump(*i, os, level + 1);
		}

		tab_over(level, os) << "]\n";

		--level;
		tab_over(level, os) << ")\n";
	}
}

#endif