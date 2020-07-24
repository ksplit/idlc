#include "dump.h"

void idlc::dump(const primitive_type& pt, std::ostream& os, unsigned int level)
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

void idlc::dump(const signature& sig, std::ostream& os, unsigned int level)
{
	tab_over(level, os) << "rpc_signature (\n";
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

void idlc::dump(const projection_type& pt, std::ostream& os, unsigned int level)
{
	tab_over(level, os) << "projection_type (\n";
	++level;

	tab_over(level, os) << "identifier: " << pt.identifier() << "\n";

	--level;
	tab_over(level, os) << ")\n";
}

void idlc::dump(const copy_type& ct, std::ostream& os, unsigned int level)
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

gsl::czstring<> idlc::to_string(rpc_side side) noexcept
{
	switch (side) {
	case rpc_side::none:
		return "none";

	case rpc_side::caller:
		return "caller";

	case rpc_side::callee:
		return "callee";

	case rpc_side::both:
		return "both";

	default:
		Expects(false);
	}
}

void idlc::dump(const attributes& attrs, std::ostream& os, unsigned int level)
{
	tab_over(level, os) << "attributes (\n";
	++level;

	switch (attrs.get_value_copy_direction()) {
	case copy_direction::none:
		tab_over(level, os) << "value_copy_direction: none\n";
		break;

	case copy_direction::in:
		tab_over(level, os) << "value_copy_direction: in\n";
		break;

	case copy_direction::out:
		tab_over(level, os) << "value_copy_direction: out\n";
		break;

	case copy_direction::both:
		tab_over(level, os) << "value_copy_direction: both\n";
		break;
	}

	const auto share_side = attrs.get_sharing_op_side();
	if (share_side == rpc_side::none) {
		tab_over(level, os) << "sharing_op_side: none\n";
		tab_over(level, os) << "sharing_op: null\n";
	}
	else {
		switch (share_side) {
		case rpc_side::caller:
			tab_over(level, os) << "sharing_op_side: caller\n";
			break;

		case rpc_side::callee:
			tab_over(level, os) << "sharing_op_side: callee\n";
			break;

		case rpc_side::both:
			tab_over(level, os) << "sharing_op_side: both\n";
			break;
		}

		switch (attrs.get_sharing_op()) {
		case sharing_op::alloc:
			tab_over(level, os) << "sharing_op: alloc\n";
			break;

		case sharing_op::dealloc:
			tab_over(level, os) << "sharing_op: dealloc\n";
			break;

		case sharing_op::bind:
			tab_over(level, os) << "sharing_op: bind\n";
			break;
		}
	}

	--level;
	tab_over(level, os) << ")\n";
}

void idlc::dump(const type& type, std::ostream& os, unsigned int level)
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
		dump(*type.get_attributes(), os, level + 1);
	}
	else {
		tab_over(level, os) << "attributes: null\n";
	}

	tab_over(level, os) << "stars: " << type.stars() << "\n";

	--level;
	tab_over(level, os) << ")\n";
}

void idlc::dump(const var_field& field, std::ostream& os, unsigned int level)
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

void idlc::dump(const rpc_field& field, std::ostream& os, unsigned int level)
{
	tab_over(level, os) << "rpc_field (\n";
	++level;

	if (field.identifier()) {
		tab_over(level, os) << "identifier: " << field.identifier() << "\n";
	}
	else {
		tab_over(level, os) << "identifier: null\n";
	}

	tab_over(level, os) << "rpc_signature:\n";
	dump(field.get_signature(), os, level + 1);

	if (field.get_attributes()) {
		tab_over(level, os) << "attributes:\n";
		dump(*field.get_attributes(), os, level + 1);
	}
	else {
		tab_over(level, os) << "attributes: null\n";
	}

	--level;
	tab_over(level, os) << ")\n";
}

void idlc::dump(const field& field, std::ostream& os, unsigned int level)
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

void idlc::dump(const projection& proj, std::ostream& os, unsigned int level)
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

void idlc::dump(const rpc& rpc, std::ostream& os, unsigned int level)
{
	tab_over(level, os) << "rpc (\n";
	++level;

	tab_over(level, os) << "identifier: " << rpc.identifier() << "\n";
	tab_over(level, os) << "rpc_signature:\n";
	dump(rpc.get_signature(), os, level + 1);

	--level;
	tab_over(level, os) << ")\n";
}

void idlc::dump(const require& req, std::ostream& os, unsigned int level)
{
	tab_over(level, os) << "require (\n";
	++level;

	tab_over(level, os) << "identifier: " << req.identifier() << "\n";

	--level;
	tab_over(level, os) << ")\n";
}

void idlc::dump(const module_item& item, std::ostream& os, unsigned int level)
{
	tab_over(level, os) << "module_item (\n";
	++level;

	switch (item.kind()) {
	case module_item_kind::projection:
		tab_over(level, os) << "kind: projection\n";
		tab_over(level, os) << "value:\n";
		dump(item.get<module_item_kind::projection>(), os, level + 1);
		break;

	case module_item_kind::rpc:
		tab_over(level, os) << "kind: rpc\n";
		tab_over(level, os) << "value:\n";
		dump(item.get<module_item_kind::rpc>(), os, level + 1);
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

void idlc::dump(const module& mod, std::ostream& os, unsigned int level)
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

void idlc::dump(const include& inc, std::ostream& os, unsigned int level)
{
	tab_over(level, os) << "include (\n";
	++level;

	tab_over(level, os) << "path: " << inc.path() << "\n";

	--level;
	tab_over(level, os) << ")\n";
}

void idlc::dump(const file_item& fi, std::ostream& os, unsigned int level)
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

void idlc::dump(const file& file, std::ostream& os, unsigned int level)
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