#include "dump.h"

void idlc::dump(const primitive_type& pt, std::ostream& os, unsigned int level)
{
	tab_over(os, level) << "primitive_type (\n";
	++level;

	switch (pt.kind()) {
	case primitive_type_kind::bool_k:
		tab_over(os, level) << "kind: bool\n";
		break;

	case primitive_type_kind::char_k:
		tab_over(os, level) << "kind: char\n";
		break;

	case primitive_type_kind::double_k:
		tab_over(os, level) << "kind: double\n";
		break;

	case primitive_type_kind::float_k:
		tab_over(os, level) << "kind: float\n";
		break;

	case primitive_type_kind::int_k:
		tab_over(os, level) << "kind: int\n";
		break;

	case primitive_type_kind::long_k:
		tab_over(os, level) << "kind: long\n";
		break;

	case primitive_type_kind::long_long_k:
		tab_over(os, level) << "kind: long long\n";
		break;

	case primitive_type_kind::short_k:
		tab_over(os, level) << "kind: short\n";
		break;

	case primitive_type_kind::unsigned_char_k:
		tab_over(os, level) << "kind: unsigned char\n";
		break;

	case primitive_type_kind::unsigned_int_k:
		tab_over(os, level) << "kind: unsigned int\n";
		break;

	case primitive_type_kind::unsigned_long_k:
		tab_over(os, level) << "kind: unsigned long\n";
		break;

	case primitive_type_kind::unsigned_long_long_k:
		tab_over(os, level) << "kind: usnigned long long\n";
		break;

	case primitive_type_kind::unsigned_short_k:
		tab_over(os, level) << "kind: unsigned short\n";
		break;
	}

	--level;
	tab_over(os, level) << ")\n";
}

void idlc::dump(const signature& sig, std::ostream& os, unsigned int level)
{
	tab_over(os, level) << "rpc_signature (\n";
	++level;

	tab_over(os, level) << "return_field:\n";
	dump(sig.return_field(), os, level + 1);

	tab_over(os, level) << "arguments: [\n";
	++level;

	for (const auto& arg : sig.arguments()) {
		dump(*arg, os, level);
	}

	--level;
	tab_over(os, level) << "]\n";

	--level;
	tab_over(os, level) << ")\n";
}

void idlc::dump(const projection_type& pt, std::ostream& os, unsigned int level)
{
	tab_over(os, level) << "projection_type (\n";
	++level;

	tab_over(os, level) << "identifier: " << pt.identifier() << "\n";

	--level;
	tab_over(os, level) << ")\n";
}

void idlc::dump(const copy_type& ct, std::ostream& os, unsigned int level)
{
	tab_over(os, level) << "copy_type (\n";
	++level;

	switch (ct.kind()) {
	case copy_type_kind::primitive:
		tab_over(os, level) << "kind: primitive\n";
		tab_over(os, level) << "value:\n";
		dump(ct.get<copy_type_kind::primitive>(), os, level + 1);
		break;

	case copy_type_kind::projection:
		tab_over(os, level) << "kind: projection\n";
		tab_over(os, level) << "value:\n";
		dump(ct.get<copy_type_kind::projection>(), os, level + 1);
		break;
	}

	--level;
	tab_over(os, level) << ")\n";
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
	tab_over(os, level) << "attributes (\n";
	++level;

	switch (attrs.get_value_copy_direction()) {
	case copy_direction::none:
		tab_over(os, level) << "value_copy_direction: none\n";
		break;

	case copy_direction::in:
		tab_over(os, level) << "value_copy_direction: in\n";
		break;

	case copy_direction::out:
		tab_over(os, level) << "value_copy_direction: out\n";
		break;

	case copy_direction::both:
		tab_over(os, level) << "value_copy_direction: both\n";
		break;
	}

	const auto share_side = attrs.get_sharing_op_side();
	if (share_side == rpc_side::none) {
		tab_over(os, level) << "sharing_op_side: none\n";
		tab_over(os, level) << "sharing_op: null\n";
	}
	else {
		switch (share_side) {
		case rpc_side::caller:
			tab_over(os, level) << "sharing_op_side: caller\n";
			break;

		case rpc_side::callee:
			tab_over(os, level) << "sharing_op_side: callee\n";
			break;

		case rpc_side::both:
			tab_over(os, level) << "sharing_op_side: both\n";
			break;
		}

		switch (attrs.get_sharing_op()) {
		case sharing_op::alloc:
			tab_over(os, level) << "sharing_op: alloc\n";
			break;

		case sharing_op::dealloc:
			tab_over(os, level) << "sharing_op: dealloc\n";
			break;

		case sharing_op::bind:
			tab_over(os, level) << "sharing_op: bind\n";
			break;
		}
	}

	--level;
	tab_over(os, level) << ")\n";
}

void idlc::dump(const type& type, std::ostream& os, unsigned int level)
{
	tab_over(os, level) << "type (\n";
	++level;

	if (type.get_copy_type()) {
		tab_over(os, level) << "copy_type:\n";
		dump(*type.get_copy_type(), os, level + 1);
	}
	else {
		tab_over(os, level) << "copy_type: null\n";
	}

	if (type.get_attributes()) {
		tab_over(os, level) << "attributes:\n";
		dump(*type.get_attributes(), os, level + 1);
	}
	else {
		tab_over(os, level) << "attributes: null\n";
	}

	tab_over(os, level) << "stars: " << type.stars() << "\n";

	--level;
	tab_over(os, level) << ")\n";
}

void idlc::dump(const var_field& field, std::ostream& os, unsigned int level)
{
	tab_over(os, level) << "var_field (\n";
	++level;

	if (field.identifier()) {
		tab_over(os, level) << "identifier: " << field.identifier() << "\n";
	}
	else {
		tab_over(os, level) << "identifier: null\n";
	}

	tab_over(os, level) << "type:\n";
	dump(field.get_type(), os, level + 1);

	--level;
	tab_over(os, level) << ")\n";
}

void idlc::dump(const rpc_field& field, std::ostream& os, unsigned int level)
{
	tab_over(os, level) << "rpc_field (\n";
	++level;

	if (field.identifier()) {
		tab_over(os, level) << "identifier: " << field.identifier() << "\n";
	}
	else {
		tab_over(os, level) << "identifier: null\n";
	}

	tab_over(os, level) << "rpc_signature:\n";
	dump(field.get_signature(), os, level + 1);

	if (field.get_attributes()) {
		tab_over(os, level) << "attributes:\n";
		dump(*field.get_attributes(), os, level + 1);
	}
	else {
		tab_over(os, level) << "attributes: null\n";
	}

	--level;
	tab_over(os, level) << ")\n";
}

void idlc::dump(const field& field, std::ostream& os, unsigned int level)
{
	tab_over(os, level) << "field (\n";
	++level;

	switch (field.kind()) {
	case field_kind::rpc:
		tab_over(os, level) << "kind: rpc\n";
		tab_over(os, level) << "value:\n";
		dump(field.get<field_kind::rpc>(), os, level + 1);
		break;

	case field_kind::var:
		tab_over(os, level) << "kind: var\n";
		tab_over(os, level) << "value:\n";
		dump(field.get<field_kind::var>(), os, level + 1);
		break;
	}

	--level;
	tab_over(os, level) << ")\n";
}

void idlc::dump(const projection& proj, std::ostream& os, unsigned int level)
{
	tab_over(os, level) << "projection (\n";
	++level;

	tab_over(os, level) << "identifier: " << proj.identifier() << "\n";
	tab_over(os, level) << "real_type: " << proj.real_type() << "\n";
	tab_over(os, level) << "items: [\n";

	for (const auto& field : proj.fields()) {
		dump(*field, os, level + 1);
	}

	tab_over(os, level) << "]\n";

	--level;
	tab_over(os, level) << ")\n";
}

void idlc::dump(const rpc& rpc, std::ostream& os, unsigned int level)
{
	tab_over(os, level) << "rpc (\n";
	++level;

	tab_over(os, level) << "identifier: " << rpc.identifier() << "\n";
	tab_over(os, level) << "rpc_signature:\n";
	dump(rpc.get_signature(), os, level + 1);

	--level;
	tab_over(os, level) << ")\n";
}

void idlc::dump(const require& req, std::ostream& os, unsigned int level)
{
	tab_over(os, level) << "require (\n";
	++level;

	tab_over(os, level) << "identifier: " << req.identifier() << "\n";

	--level;
	tab_over(os, level) << ")\n";
}

void idlc::dump(const module_item& item, std::ostream& os, unsigned int level)
{
	tab_over(os, level) << "module_item (\n";
	++level;

	switch (item.kind()) {
	case module_item_kind::projection:
		tab_over(os, level) << "kind: projection\n";
		tab_over(os, level) << "value:\n";
		dump(item.get<module_item_kind::projection>(), os, level + 1);
		break;

	case module_item_kind::rpc:
		tab_over(os, level) << "kind: rpc\n";
		tab_over(os, level) << "value:\n";
		dump(item.get<module_item_kind::rpc>(), os, level + 1);
		break;

	case module_item_kind::require:
		tab_over(os, level) << "kind: require\n";
		tab_over(os, level) << "value:\n";
		dump(item.get<module_item_kind::require>(), os, level + 1);
		break;
	}

	--level;
	tab_over(os, level) << ")\n";
}

void idlc::dump(const module& mod, std::ostream& os, unsigned int level)
{
	tab_over(os, level) << "module (\n";
	++level;

	tab_over(os, level) << "identifier: " << mod.identifier() << "\n";
	tab_over(os, level) << "items: [\n";

	for (const auto& item : mod.items()) {
		dump(*item, os, level + 1);
	}

	tab_over(os, level) << "]\n";

	--level;
	tab_over(os, level) << ")\n";
}

void idlc::dump(const include& inc, std::ostream& os, unsigned int level)
{
	tab_over(os, level) << "include (\n";
	++level;

	tab_over(os, level) << "path: " << inc.path() << "\n";

	--level;
	tab_over(os, level) << ")\n";
}

void idlc::dump(const file_item& fi, std::ostream& os, unsigned int level)
{
	tab_over(os, level) << "file_item (\n";
	++level;

	switch (fi.kind()) {
	case file_item_kind::include:
		tab_over(os, level) << "kind: include\n";
		tab_over(os, level) << "_value:\n";
		dump(fi.get<file_item_kind::include>(), os, level + 1);
		break;

	case file_item_kind::module:
		tab_over(os, level) << "kind: module\n";
		tab_over(os, level) << "_value:\n";
		dump(fi.get<file_item_kind::module>(), os, level + 1);
		break;
	}

	--level;
	tab_over(os, level) << ")\n";
}

void idlc::dump(const file& file, std::ostream& os, unsigned int level)
{
	tab_over(os, level) << "file (\n";
	++level;

	tab_over(os, level) << "items: [\n";

	for (const auto& i : file.items()) {
		dump(*i, os, level + 1);
	}

	tab_over(os, level) << "]\n";

	--level;
	tab_over(os, level) << ")\n";
}