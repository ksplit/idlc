#include "marshaling.h"

#include "generic_pass.h"
#include "visit.h"

#include <sstream>

namespace idlc {
	enum class field_marshal_kind {
		value,
		projection_ptr,
		undefined
	};

	gsl::czstring<> get_primitive_string(primitive_type_kind kind);
	std::string get_type_string(const type& ty);
	std::string get_rpc_string(const signature& signature);
	field_marshal_kind get_var_marshal_kind(const type& ty);
	attributes get_attributes_with_argument_default(const attributes* attribs);
	attributes get_attributes_with_return_default(const attributes* attribs);

	bool process_caller(std::vector<marshal_op>& marshaling, const marshal_unit& unit, marshal_unit_kind kind);
	bool process_callee(std::vector<marshal_op>& marshaling, const marshal_unit& unit, marshal_unit_kind kind);

	/*
		TODO: Invest in a more thought-out implementation of marshaling
			- Some sort of tree of fields?
			- Marked with copy direction, sharing op, etc.
			- Instead of the current dynamic computation of some important things (like type strings),
				or re-compuations
			- Visiting over the tree would likely be simpler than the nominally similar families of functions here

		Excuse the blatant code duplication, as it's still unclear exactly what is actually identical,
		and what is only superficially similar.
	*/

	// TODO: switch to string names, not numeric IDs

	bool callee_insert_call(std::vector<marshal_op>& marshaling, gsl::czstring<> rpc_name, const signature& signature);
	bool callee_insert_call_indirect(std::vector<marshal_op>& marshaling, const signature& signature, std::string_view pointer);

	bool caller_marshal_argument(std::vector<marshal_op>& marshaling, const field& argument, std::vector<std::string>& caller_argument_save_ids);
	bool caller_marshal_argument_rpc(std::vector<marshal_op>& marshaling, const rpc_field& rpc);
	bool caller_marshal_argument_var(std::vector<marshal_op>& marshaling, const var_field& rpc, std::vector<std::string>& argument_save_ids);
	bool caller_marshal_argument_sub_rpc(std::vector<marshal_op>& marshaling, const rpc_field& rpc, std::string_view parent);
	bool caller_marshal_argument_sub_var(std::vector<marshal_op>& marshaling, const var_field& var, std::string_view parent);
	bool caller_marshal_argument_sub_field(std::vector<marshal_op>& marshaling, const field& proj_field, std::string_view parent_ptr_id);

	bool caller_remarshal_argument(std::vector<marshal_op>& marshaling, const field& argument, std::string_view save_id);
	bool caller_remarshal_argument_var(std::vector<marshal_op>& marshaling, const var_field& rpc, std::string_view save_id);
	bool caller_remarshal_argument_sub_rpc(std::vector<marshal_op>& marshaling, const rpc_field& rpc, std::string_view parent);
	bool caller_remarshal_argument_sub_var(std::vector<marshal_op>& marshaling, const var_field& var, std::string_view parent_ptr_id);
	bool caller_remarshal_argument_sub_field(std::vector<marshal_op>& marshaling, const field& proj_field, std::string_view parent_ptr_id);

	bool callee_unmarshal_argument(std::vector<marshal_op>& marshaling, const field& argument, std::vector<std::string>& argument_save_ids);
	bool callee_unmarshal_argument_rpc(std::vector<marshal_op>& marshaling, const rpc_field& rpc);
	bool callee_unmarshal_argument_var(std::vector<marshal_op>& marshaling, const var_field& var, std::vector<std::string>& argument_save_ids);
	bool callee_unmarshal_argument_sub_rpc(std::vector<marshal_op>& marshaling, const rpc_field& rpc, std::string_view parent);
	bool callee_unmarshal_argument_sub_var(std::vector<marshal_op>& marshaling, const var_field& var, std::string_view parent);
	bool callee_unmarshal_argument_sub_field(std::vector<marshal_op>& marshaling, const field& proj_field, std::string_view parent);

	bool callee_remarshal_argument(std::vector<marshal_op>& marshaling, const field& argument, std::string save_id);
	bool callee_remarshal_argument_var(std::vector<marshal_op>& marshaling, const var_field& var, std::string_view save_id);
	bool callee_remarshal_argument_sub_rpc(std::vector<marshal_op>& marshaling, const rpc_field& rpc, std::string_view parent);
	bool callee_remarshal_argument_sub_var(std::vector<marshal_op>& marshaling, const var_field& var, std::string_view parent);
	bool callee_remarshal_argument_sub_field(std::vector<marshal_op>& marshaling, const field& proj_field, std::string_view parent);

	// No such thing as "remarshaling" a return value (hence the fact that an [in] field on a return value is meaningless)
	// NOTE: this is easily changed, it just uses the annotation to decide whether to skip the field or not
	// So we can trivially invert the annotations for return values

	bool caller_unmarshal_return(std::vector<marshal_op>& marshaling, const field& return_field);
	bool caller_unmarshal_return_var(std::vector<marshal_op>& marshaling, const var_field& return_field);
	bool caller_unmarshal_return_rpc(std::vector<marshal_op>& marshaling, const rpc_field& return_field);
	bool caller_unmarshal_return_sub_field(std::vector<marshal_op>& marshaling, const field& return_field, std::string_view parent);
	bool caller_unmarshal_return_sub_var(std::vector<marshal_op>& marshaling, const var_field& return_field, std::string_view parent);
	bool caller_unmarshal_return_sub_rpc(std::vector<marshal_op>& marshaling, const rpc_field& return_field, std::string_view parent);

	bool callee_marshal_return(std::vector<marshal_op>& marshaling, const field& return_field);
	bool callee_marshal_return_var(std::vector<marshal_op>& marshaling, const var_field& return_field);
	bool callee_marshal_return_rpc(std::vector<marshal_op>& marshaling, const rpc_field& return_field);
	bool callee_marshal_return_sub_field(std::vector<marshal_op>& marshaling, const field& return_field, std::string_view parent_ptr_id);
	bool callee_marshal_return_sub_var(std::vector<marshal_op>& marshaling, const var_field& return_field, std::string_view parent);
	bool callee_marshal_return_sub_rpc(std::vector<marshal_op>& marshaling, const rpc_field& return_field, std::string_view parent);
}

/*
	value -> a plain value type (scalar)
	projection_ptr -> a pointer to projection
	everything else is currently undefined marshaling
*/

idlc::field_marshal_kind idlc::get_var_marshal_kind(const type& ty)
{
	if (ty.stars() == 0) {
		return field_marshal_kind::value;
	}
	else if (ty.stars() == 1 && ty.get_copy_type()) {
		if (ty.get_copy_type()->kind() == copy_type_kind::projection) {
			return field_marshal_kind::projection_ptr;
		}
		else {
			return field_marshal_kind::undefined;
		}
	}
	else {
		return field_marshal_kind::undefined;
	}
}

// Not important ATM, but this should probably store the defaulted attributes into the type node
idlc::attributes idlc::get_attributes_with_argument_default(const attributes* attribs)
{
	if (!attribs) {
		return *attributes::make(
			{
				{rpc_side::callee, attribute_type::bind},	// bind(callee)
				{rpc_side::callee, attribute_type::copy}	// in
			}
		);
	}
	else {
		return *attribs;
	}
}

idlc::attributes idlc::get_attributes_with_return_default(const attributes* attribs)
{
	if (!attribs) {
		return *attributes::make(
			{
				{rpc_side::callee, attribute_type::bind},	// bind(callee)
				{rpc_side::caller, attribute_type::copy}	// out
			}
		);
	}
	else {
		return *attribs;
	}
}

std::string idlc::get_type_string(const type& ty)
{
	std::stringstream type_str;
	if (!ty.get_copy_type()) {
		type_str << "void";
	}
	else {
		const auto& ct = *ty.get_copy_type();
		switch (ct.kind()) {
		case copy_type_kind::primitive:
			type_str << get_primitive_string(ct.get<copy_type_kind::primitive>().kind());
			break;

		case copy_type_kind::projection:
			type_str << "struct " << ct.get<copy_type_kind::projection>().definition().real_type();
			break;
		}
	}

	for (unsigned int i {0}; i < ty.stars(); ++i) {
		type_str << "*";
	}

	return type_str.str();
}

// For use in the final cast in call_indirect
std::string idlc::get_rpc_string(const signature& signature)
{
	std::stringstream rpc_string;
	const field& return_field {signature.return_field()};
	switch (return_field.kind()) {
	case field_kind::rpc:
		rpc_string << get_rpc_string(return_field.get<field_kind::rpc>().get_signature());
		break;

	case field_kind::var:
		rpc_string << get_type_string(return_field.get<field_kind::var>().get_type());
		break;
	}

	rpc_string << " (*)(";
	bool use_comma {false};
	for (const std::unique_ptr<field>& argument : signature.arguments()) {
		if (use_comma) {
			rpc_string << ", ";
		}

		use_comma = true;

		switch (argument->kind()) {
		case field_kind::rpc:
			rpc_string << get_rpc_string(return_field.get<field_kind::rpc>().get_signature());
			break;

		case field_kind::var:
			rpc_string << get_type_string(return_field.get<field_kind::var>().get_type());
			break;
		}
	}

	return rpc_string.str();
}

gsl::czstring<> idlc::get_primitive_string(primitive_type_kind kind)
{
	switch (kind) {
	case primitive_type_kind::bool_k:
		return "bool";

	case primitive_type_kind::char_k:
		return "char";

	case primitive_type_kind::double_k:
		return "double";

	case primitive_type_kind::float_k:
		return "float";

	case primitive_type_kind::int_k:
		return "int";

	case primitive_type_kind::long_k:
		return "long";

	case primitive_type_kind::long_long_k:
		return "long long";

	case primitive_type_kind::short_k:
		return "short";

	case primitive_type_kind::unsigned_char_k:
		return "unsigned char";

	case primitive_type_kind::unsigned_int_k:
		return "unsigned int";

	case primitive_type_kind::unsigned_long_k:
		return "unsigned long";

	case primitive_type_kind::unsigned_long_long_k:
		return "unsigned long long";

	case primitive_type_kind::unsigned_short_k:
		return "unsigned short";

	default:
		Expects(false);
	}
}

// TODO: add handling of indirect call_direct for function pointers
bool idlc::process_marshal_units(gsl::span<const marshal_unit> units, marshal_unit_kind kind, std::vector<marshal_unit_lists>& unit_marshaling)
{
	unit_marshaling.clear();
	unit_marshaling.reserve(units.size());

	for (const marshal_unit& unit : units) {
		unit_marshaling.push_back({unit.identifier, {}, {}});
		marshal_unit_lists& lists {unit_marshaling.back()};
		process_caller(lists.caller_ops, unit, kind);
		process_callee(lists.callee_ops, unit, kind);
	}

	return true;
}

bool idlc::process_caller(std::vector<marshal_op>& marshaling, const marshal_unit& unit, marshal_unit_kind kind)
{
	const signature& signature {*unit.rpc_signature};

	if (kind == marshal_unit_kind::indirect) {
		// rpc pointer stubs take the real pointer as their first argument
		marshaling.push_back(marshal {"real_pointer"});
	}

	// To keep track of projection pointers that need re-marshaling for [out] fields
	std::vector<std::string> caller_argument_save_ids(signature.arguments().size());
	caller_argument_save_ids.resize(0); // Keep the reserved space while allowing for push_back()

	for (const std::unique_ptr<field>& field : signature.arguments()) {
		if (!caller_marshal_argument(marshaling, *field, caller_argument_save_ids)) {
			return false;
		}
	}

	marshaling.push_back(send_rpc {unit.identifier});

	auto current_save_id = begin(caller_argument_save_ids);
	for (const std::unique_ptr<field>& field : signature.arguments()) {
		// Need to know save-id of each argument
		if (!caller_remarshal_argument(marshaling, *field, *(current_save_id++))) {
			return false;
		}
	}

	if (!caller_unmarshal_return(marshaling, signature.return_field())) {
		return false;
	}

	return true;
}

bool idlc::process_callee(std::vector<marshal_op>& marshaling, const marshal_unit& unit, marshal_unit_kind kind)
{
	const signature& signature {*unit.rpc_signature};

	std::string underlying_ptr_save_id {"real_pointer"};
	if (kind == marshal_unit_kind::indirect) {
		marshaling.push_back(unmarshal {"void*", underlying_ptr_save_id});
	}

	// To keep track of projection pointers that need re-marshaling for [out] fields
	std::vector<std::string> callee_argument_save_ids(signature.arguments().size());
	callee_argument_save_ids.resize(0); // Keep the reserved space while allowing for push_back()

	for (const std::unique_ptr<field>& field : signature.arguments()) {
		if (!callee_unmarshal_argument(marshaling, *field, callee_argument_save_ids)) {
			return false;
		}
	}

	if (kind == marshal_unit_kind::indirect) {
		callee_insert_call_indirect(marshaling, signature, underlying_ptr_save_id);
	}
	else {
		callee_insert_call(marshaling, unit.identifier, signature);
	}

	auto current_save_id = begin(callee_argument_save_ids);
	for (const std::unique_ptr<field>& field : signature.arguments()) {
		if (!callee_remarshal_argument(marshaling, *field, *(current_save_id++))) {
			return false;
		}
	}

	if (!callee_marshal_return(marshaling, signature.return_field())) {
		return false;
	}

	return true;
}

bool idlc::callee_insert_call(std::vector<marshal_op>& marshaling, gsl::czstring<> rpc_name, const signature& signature)
{
	bool use_comma {false};
	std::stringstream argument_string;
	for (const std::unique_ptr<field>& field : signature.arguments()) {
		switch (field->kind()) {
		case field_kind::rpc:
			argument_string << (use_comma ? ", " : "") << field->get<field_kind::rpc>().identifier();
			break;

		case field_kind::var:
			argument_string << (use_comma ? ", " : "") << field->get<field_kind::var>().identifier();
			break;
		}

		use_comma = true;
	}

	const field& return_field {signature.return_field()};
	switch (return_field.kind()) {
	case field_kind::var:
		marshaling.push_back(
			call_direct {
				get_type_string(return_field.get<field_kind::var>().get_type()),
				rpc_name,
				argument_string.str()
			}
		);

		break;

	case field_kind::rpc:
		marshaling.push_back(
			call_direct {
				"void*",
				rpc_name,
				argument_string.str()
			}
		);

		break;
	}

	return true;
}

bool idlc::callee_insert_call_indirect(std::vector<marshal_op>& marshaling, const signature& signature, std::string_view pointer)
{
	bool use_comma {false};
	std::stringstream argument_string;
	for (const std::unique_ptr<field>& field : signature.arguments()) {
		switch (field->kind()) {
		case field_kind::rpc:
			argument_string << (use_comma ? ", " : "") << field->get<field_kind::rpc>().identifier();
			break;

		case field_kind::var:
			argument_string << (use_comma ? ", " : "") << field->get<field_kind::var>().identifier();
			break;
		}

		use_comma = true;
	}

	const std::string function_type {get_rpc_string(signature)};
	const field& return_field {signature.return_field()};
	switch (return_field.kind()) {
	case field_kind::var:
		marshaling.push_back(
			call_indirect {
				get_type_string(return_field.get<field_kind::var>().get_type()),
				function_type,
				{pointer.data(), pointer.length()},
				argument_string.str()
			}
		);

		break;

	case field_kind::rpc:
		marshaling.push_back(
			call_indirect {
				"void*",
				function_type,
				{pointer.data(), pointer.length()},
				argument_string.str()
			}
		);

		break;
	}

	return true;
}

bool idlc::caller_marshal_argument_rpc(std::vector<marshal_op>& marshaling, const rpc_field& rpc)
{
	marshaling.push_back(marshal {rpc.identifier()});
	return true;
}

bool idlc::caller_marshal_argument_var(std::vector<marshal_op>& marshaling, const var_field& var, std::vector<std::string>& argument_save_ids)
{
	const type& type {var.get_type()};

	argument_save_ids.push_back(var.identifier());
	marshaling.push_back(marshal {var.identifier()});

	const field_marshal_kind marshal_kind {get_var_marshal_kind(type)};
	switch (marshal_kind) {
	case field_marshal_kind::undefined:
		log_error("\t", var.identifier(), " has undefined marshaling");
		return false;

	case field_marshal_kind::value:
		break;

	case field_marshal_kind::projection_ptr: {
		// Recurse into projection fields
		// Needs save ID of the projection pointer and the projection definition

		marshaling.push_back(block_if_not_null {var.identifier()});

		const projection& type_definition {type.get_copy_type()->get<copy_type_kind::projection>().definition()};
		for (const std::unique_ptr<field>& pr_field : type_definition.fields()) {
			if (!caller_marshal_argument_sub_field(marshaling, *pr_field, var.identifier())) {
				return false;
			}
		}

		marshaling.push_back(end_block {});

		break;
	}
	}

	return true;
}

bool idlc::caller_marshal_argument_sub_rpc(std::vector<marshal_op>& marshaling, const rpc_field& rpc, std::string_view parent)
{
	attributes attribs {get_attributes_with_argument_default(rpc.get_attributes())};
	const copy_direction field_copy_direction {attribs.get_value_copy_direction()};
	if (!(field_copy_direction == copy_direction::both
		|| field_copy_direction == copy_direction::in))
	{
		return true; // Do nothing, don't have to marshal
	}

	// NOTE: I think we can get_field away with the void* trick due to C's looser type system
	marshaling.push_back(marshal_field {{parent.data(), parent.length()}, rpc.identifier()});

	return true;
}

bool idlc::caller_marshal_argument_sub_var(std::vector<marshal_op>& marshaling, const var_field& var, std::string_view parent)
{
	const type& type {var.get_type()};

	attributes attribs {get_attributes_with_argument_default(type.get_attributes())};
	const copy_direction field_copy_direction {attribs.get_value_copy_direction()};
	if (!(field_copy_direction == copy_direction::both
		|| field_copy_direction == copy_direction::in))
	{
		return true; // Do nothing, don't have to marshal
	}

	const field_marshal_kind marshal_kind {get_var_marshal_kind(type)};
	switch (marshal_kind) {
	case field_marshal_kind::undefined:
		log_error("\t", var.identifier(), " has undefined marshaling");
		return false;

	case field_marshal_kind::value:
		marshaling.push_back(marshal_field {{parent.data(), parent.length()}, var.identifier()});
		break;

	case field_marshal_kind::projection_ptr: {
		// Recurse into projection fields
		// Needs save ID of the projection pointer and the projection definition
		std::string save_id {parent.data(), parent.length()};
		save_id += "_";
		save_id += var.identifier();
		marshaling.push_back(load_field_indirect {get_type_string(type), save_id, {parent.data(), parent.length()}, var.identifier()});
		marshaling.push_back(marshal {save_id});
		marshaling.push_back(block_if_not_null {save_id});

		const projection& type_definition {type.get_copy_type()->get<copy_type_kind::projection>().definition()};
		for (const std::unique_ptr<field>& pr_field : type_definition.fields()) {
			if (!caller_marshal_argument_sub_field(marshaling, *pr_field, save_id)) {
				return false;
			}
		}

		marshaling.push_back(end_block {});

		break;
	}
	}

	return true;
}

bool idlc::caller_marshal_argument_sub_field(std::vector<marshal_op>& marshaling, const field& proj_field, std::string_view parent_ptr_id)
{
	switch (proj_field.kind()) {
	case field_kind::rpc:
		if (!caller_marshal_argument_sub_rpc(marshaling, proj_field.get<field_kind::rpc>(), parent_ptr_id)) {
			return false;
		}

		break;

	case field_kind::var:
		if (!caller_marshal_argument_sub_var(marshaling, proj_field.get<field_kind::var>(), parent_ptr_id)) {
			return false;
		}

		break;
	}

	return true;
}

bool idlc::caller_remarshal_argument(std::vector<marshal_op>& marshaling, const field& argument, std::string_view save_id)
{
	switch (argument.kind()) {
	case field_kind::rpc:
		return true; // Meaningless to re-marshal an RPC pointer

	case field_kind::var:
		if (!caller_remarshal_argument_var(marshaling, argument.get<field_kind::var>(), save_id)) {
			return false;
		}

		break;
	}

	return true;
}

bool idlc::caller_remarshal_argument_var(std::vector<marshal_op>& marshaling, const var_field& var, std::string_view save_id)
{
	const type& type {var.get_type()};
	const field_marshal_kind marshal_kind {get_var_marshal_kind(type)};

	switch (marshal_kind) {
	case field_marshal_kind::undefined:
		log_error("\t", var.identifier(), " has undefined marshaling");
		return false;

	case field_marshal_kind::projection_ptr:
		// Recurse into projection fields
		// Needs save ID of the projection pointer and the projection definition

		marshaling.push_back(block_if_not_null {{save_id.data(), save_id.size()}});

		const projection& type_definition {type.get_copy_type()->get<copy_type_kind::projection>().definition()};
		for (const std::unique_ptr<field>& pr_field : type_definition.fields()) {
			if (!caller_remarshal_argument_sub_field(marshaling, *pr_field, save_id)) {
				return false;
			}
		}

		marshaling.push_back(end_block {});

		break;
	}

	return true;
}

bool idlc::caller_remarshal_argument_sub_rpc(std::vector<marshal_op>& marshaling, const rpc_field& rpc, std::string_view parent)
{
	attributes attribs {get_attributes_with_argument_default(rpc.get_attributes())};
	const copy_direction field_copy_direction {attribs.get_value_copy_direction()};
	if (!(field_copy_direction == copy_direction::both
		|| field_copy_direction == copy_direction::out))
	{
		return true; // Do nothing, don't have to marshal
	}

	std::string store_name {};
	store_name += parent;
	store_name += "_";
	store_name += rpc.identifier();
	marshaling.push_back(unmarshal {"void*", store_name});
	std::string trampoline {store_name};
	trampoline += "_trampoline";
	marshaling.push_back(inject_trampoline {"void* " + trampoline, store_name});
	marshaling.push_back(store_field_indirect {{parent.data(), parent.size()}, rpc.identifier(), trampoline});

	return true;
}

bool idlc::caller_remarshal_argument_sub_var(std::vector<marshal_op>& marshaling, const var_field& var, std::string_view parent)
{
	const type& type {var.get_type()};

	attributes attribs {get_attributes_with_argument_default(type.get_attributes())};
	const copy_direction field_copy_direction {attribs.get_value_copy_direction()};
	if (!(field_copy_direction == copy_direction::both
		|| field_copy_direction == copy_direction::out))
	{
		return true; // Do nothing, don't have to marshal
	}
	const field_marshal_kind marshal_kind {get_var_marshal_kind(type)};

	switch (marshal_kind) {
	case field_marshal_kind::undefined:
		log_error("\t", var.identifier(), " has undefined marshaling");
		return false;

	case field_marshal_kind::value:
		marshaling.push_back(unmarshal_field {get_type_string(type), {parent.data(), parent.size()}, var.identifier()});
		break;

	case field_marshal_kind::projection_ptr:
		// Recurse into projection fields
		// Needs save ID of the projection pointer and the projection definition

		std::string save_id {parent.data(), parent.length()};
		save_id += "_";
		save_id += var.identifier();
		marshaling.push_back(unmarshal {get_type_string(type), save_id});
		marshaling.push_back(store_field_indirect {{parent.data(), parent.length()}, var.identifier(), save_id});
		marshaling.push_back(block_if_not_null {save_id});

		const projection& type_definition {type.get_copy_type()->get<copy_type_kind::projection>().definition()};
		for (const std::unique_ptr<field>& pr_field : type_definition.fields()) {
			if (!caller_remarshal_argument_sub_field(marshaling, *pr_field, save_id)) {
				return false;
			}
		}

		marshaling.push_back(end_block {});

		break;
	}

	return true;
}

bool idlc::caller_remarshal_argument_sub_field(std::vector<marshal_op>& marshaling, const field& proj_field, std::string_view parent_ptr_id)
{
	switch (proj_field.kind()) {
	case field_kind::rpc:
		if (!caller_remarshal_argument_sub_rpc(marshaling, proj_field.get<field_kind::rpc>(), parent_ptr_id)) {
			return false;
		}

		break;

	case field_kind::var:
		if (!caller_remarshal_argument_sub_var(marshaling, proj_field.get<field_kind::var>(), parent_ptr_id)) {
			return false;
		}

		break;
	}

	return true;
}

bool idlc::caller_marshal_argument(std::vector<marshal_op>& marshaling, const field& argument, std::vector<std::string>& caller_argument_save_ids)
{
	switch (argument.kind()) {
	case field_kind::rpc:
		// Placeholder, easily shows in generated code if improperly used
		// Anticipating need for other kinds of remarshaling
		caller_argument_save_ids.push_back("");
		if (!caller_marshal_argument_rpc(marshaling, argument.get<field_kind::rpc>())) {
			return false;
		}

		break;

	case field_kind::var:
		if (!caller_marshal_argument_var(marshaling, argument.get<field_kind::var>(), caller_argument_save_ids)) {
			return false;
		}

		break;
	}

	return true;
}

bool idlc::callee_unmarshal_argument_rpc(std::vector<marshal_op>& marshaling, const rpc_field& rpc)
{
	std::string store_name {rpc.identifier()};
	store_name += "_remote";
	marshaling.push_back(unmarshal {"void*", rpc.identifier()});
	marshaling.push_back(
		inject_trampoline {
			std::string {"void* "} + rpc.identifier(),
			store_name
		}
	);

	return true;
}

bool idlc::callee_unmarshal_argument_var(std::vector<marshal_op>& marshaling, const var_field& var, std::vector<std::string>& argument_save_ids)
{
	const type& type {var.get_type()};
	const std::string type_string {get_type_string(type)};

	marshaling.push_back(unmarshal {get_type_string(type), var.identifier()});
	argument_save_ids.push_back(var.identifier());

	const field_marshal_kind marshal_kind {get_var_marshal_kind(type)};
	switch (marshal_kind) {
	case field_marshal_kind::undefined:
		log_error("\t", var.identifier(), " has undefined marshaling");
		return false;

	case field_marshal_kind::projection_ptr:
		// Recurse into projection fields
		// Needs save ID of the projection pointer and the projection definition

		marshaling.push_back(block_if_not_null {var.identifier()});

		const projection& type_definition {type.get_copy_type()->get<copy_type_kind::projection>().definition()};
		for (const std::unique_ptr<field>& pr_field : type_definition.fields()) {
			if (!callee_unmarshal_argument_sub_field(marshaling, *pr_field, var.identifier())) {
				return false;
			}
		}

		marshaling.push_back(end_block {});

		break;
	}

	return true;
}

bool idlc::callee_unmarshal_argument_sub_rpc(std::vector<marshal_op>& marshaling, const rpc_field& rpc, std::string_view parent)
{
	attributes attribs {get_attributes_with_argument_default(rpc.get_attributes())};
	const copy_direction field_copy_direction {attribs.get_value_copy_direction()};
	if (!(field_copy_direction == copy_direction::both
		|| field_copy_direction == copy_direction::in))
	{
		return true; // Do nothing, don't have to marshal
	}

	std::string store_name {};
	store_name += parent;
	store_name += rpc.identifier();
	marshaling.push_back(unmarshal {"void*", store_name});
	std::string trampoline {store_name};
	trampoline += "_trampoline";
	marshaling.push_back(inject_trampoline {"void* " + trampoline, store_name});
	marshaling.push_back(store_field_indirect {{parent.data(), parent.size()}, rpc.identifier(), trampoline});

	return true;
}

bool idlc::callee_unmarshal_argument_sub_var(std::vector<marshal_op>& marshaling, const var_field& var, std::string_view parent)
{
	const type& type {var.get_type()};

	attributes attribs {get_attributes_with_argument_default(type.get_attributes())};
	const copy_direction field_copy_direction {attribs.get_value_copy_direction()};
	if (!(field_copy_direction == copy_direction::both
		|| field_copy_direction == copy_direction::in))
	{
		return true; // Do nothing, don't have to marshal
	}

	const field_marshal_kind marshal_kind {get_var_marshal_kind(type)};

	switch (marshal_kind) {
	case field_marshal_kind::undefined:
		log_error("\t", var.identifier(), " has undefined marshaling");
		return false;

	case field_marshal_kind::value:
		marshaling.push_back(unmarshal_field {get_type_string(type), {parent.data(), parent.size()}, var.identifier()});
		break;

	case field_marshal_kind::projection_ptr:
		// Recurse into projection fields
		// Needs save ID of the projection pointer and the projection definition

		std::string save_id {parent.data(), parent.length()};
		save_id += "_";
		save_id += var.identifier();
		marshaling.push_back(unmarshal {get_type_string(type), save_id});
		marshaling.push_back(store_field_indirect {{parent.data(), parent.length()}, var.identifier(), save_id});
		marshaling.push_back(block_if_not_null {save_id});

		const projection& type_definition {type.get_copy_type()->get<copy_type_kind::projection>().definition()};
		for (const std::unique_ptr<field>& pr_field : type_definition.fields()) {
			if (!callee_unmarshal_argument_sub_field(marshaling, *pr_field, save_id)) {
				return false;
			}
		}

		marshaling.push_back(end_block {});

		break;
	}

	return true;
}

bool idlc::callee_unmarshal_argument_sub_field(std::vector<marshal_op>& marshaling, const field& proj_field, std::string_view parent)
{
	switch (proj_field.kind()) {
	case field_kind::rpc:
		if (!callee_unmarshal_argument_sub_rpc(marshaling, proj_field.get<field_kind::rpc>(), parent)) {
			return false;
		}

		break;

	case field_kind::var:
		if (!callee_unmarshal_argument_sub_var(marshaling, proj_field.get<field_kind::var>(), parent)) {
			return false;
		}

		break;
	}

	return true;
}

bool idlc::callee_unmarshal_argument(std::vector<marshal_op>& marshaling, const field& argument, std::vector<std::string>& argument_save_ids)
{
	switch (argument.kind()) {
	case field_kind::rpc:
		// Placeholder value
		argument_save_ids.push_back("");
		if (!callee_unmarshal_argument_rpc(marshaling, argument.get<field_kind::rpc>())) {
			return false;
		}

		break;

	case field_kind::var:
		if (!callee_unmarshal_argument_var(marshaling, argument.get<field_kind::var>(), argument_save_ids)) {
			return false;
		}

		break;
	}

	return true;
}

bool idlc::callee_remarshal_argument(std::vector<marshal_op>& marshaling, const field& argument, std::string save_id)
{
	switch (argument.kind()) {
	case field_kind::rpc:
		return true; // Meaningless to re-marshal an RPC pointer

	case field_kind::var:
		if (!callee_remarshal_argument_var(marshaling, argument.get<field_kind::var>(), save_id)) {
			return false;
		}

		break;
	}

	return true;
}

bool idlc::callee_remarshal_argument_var(std::vector<marshal_op>& marshaling, const var_field& var, std::string_view save_id)
{
	const type& type {var.get_type()};

	const field_marshal_kind marshal_kind {get_var_marshal_kind(type)};
	switch (marshal_kind) {
	case field_marshal_kind::undefined:
		log_error("\t", var.identifier(), " has undefined marshaling");
		return false;

	case field_marshal_kind::value:
		break;

	case field_marshal_kind::projection_ptr: {
		// Recurse into projection fields
		// Needs save ID of the projection pointer and the projection definition

		marshaling.push_back(block_if_not_null {{save_id.data(), save_id.size()}});

		const projection& type_definition {type.get_copy_type()->get<copy_type_kind::projection>().definition()};
		for (const std::unique_ptr<field>& pr_field : type_definition.fields()) {
			if (!callee_remarshal_argument_sub_field(marshaling, *pr_field, save_id)) {
				return false;
			}
		}

		marshaling.push_back(end_block {});

		break;
	}
	}

	return true;
}

bool idlc::callee_remarshal_argument_sub_rpc(std::vector<marshal_op>& marshaling, const rpc_field& rpc, std::string_view parent)
{
	attributes attribs {get_attributes_with_argument_default(rpc.get_attributes())};
	const copy_direction field_copy_direction {attribs.get_value_copy_direction()};
	if (!(field_copy_direction == copy_direction::both
		|| field_copy_direction == copy_direction::out))
	{
		return true; // Do nothing, don't have to marshal
	}

	// NOTE: I think we can get_field away with the void* trick due to C's looser type system
	marshaling.push_back(marshal_field {{parent.data(), parent.length()}, rpc.identifier()});

	return true;
}

bool idlc::callee_remarshal_argument_sub_var(std::vector<marshal_op>& marshaling, const var_field& var, std::string_view parent)
{
	const type& type {var.get_type()};

	attributes attribs {get_attributes_with_argument_default(type.get_attributes())};
	const copy_direction field_copy_direction {attribs.get_value_copy_direction()};
	if (!(field_copy_direction == copy_direction::both
		|| field_copy_direction == copy_direction::out))
	{
		return true; // Do nothing, don't have to marshal
	}

	const field_marshal_kind marshal_kind {get_var_marshal_kind(type)};
	switch (marshal_kind) {
	case field_marshal_kind::undefined:
		log_error("\t", var.identifier(), " has undefined marshaling");
		return false;

	case field_marshal_kind::value:
		marshaling.push_back(marshal_field {{parent.data(), parent.length()}, var.identifier()});
		break;

	case field_marshal_kind::projection_ptr: {
		// Recurse into projection fields
		// Needs save ID of the projection pointer and the projection definition
		std::string save_id {parent.data(), parent.length()};
		save_id += "_";
		save_id += var.identifier();
		marshaling.push_back(load_field_indirect {get_type_string(type), save_id, {parent.data(), parent.length()}, var.identifier()});
		marshaling.push_back(marshal {save_id});
		marshaling.push_back(block_if_not_null {save_id});

		const projection& type_definition {type.get_copy_type()->get<copy_type_kind::projection>().definition()};
		for (const std::unique_ptr<field>& pr_field : type_definition.fields()) {
			if (!callee_remarshal_argument_sub_field(marshaling, *pr_field, save_id)) {
				return false;
			}
		}

		marshaling.push_back(end_block {});

		break;
	}
	}

	return true;
}

bool idlc::callee_remarshal_argument_sub_field(std::vector<marshal_op>& marshaling, const field& proj_field, std::string_view parent)
{
	switch (proj_field.kind()) {
	case field_kind::rpc:
		if (!callee_remarshal_argument_sub_rpc(marshaling, proj_field.get<field_kind::rpc>(), parent)) {
			return false;
		}

		break;

	case field_kind::var:
		if (!callee_remarshal_argument_sub_var(marshaling, proj_field.get<field_kind::var>(), parent)) {
			return false;
		}

		break;
	}

	return true;
}

bool idlc::caller_unmarshal_return(std::vector<marshal_op>& marshaling, const field& return_field)
{
	switch (return_field.kind()) {
	case field_kind::rpc:
		if (!caller_unmarshal_return_rpc(marshaling, return_field.get<field_kind::rpc>())) {
			return false;
		}

		break;

	case field_kind::var:
		if (!caller_unmarshal_return_var(marshaling, return_field.get<field_kind::var>())) {
			return false;
		}

		break;
	}

	return true;
}

bool idlc::caller_unmarshal_return_var(std::vector<marshal_op>& marshaling, const var_field& return_field)
{
	const type& type {return_field.get_type()};
	if (!return_field.get_type().get_copy_type()) {
		return true; // type void, no marshaling needed
	}

	marshaling.push_back(unmarshal {get_type_string(type), "return_value"});

	switch (get_var_marshal_kind(type)) {
	case field_marshal_kind::undefined:
		log_error("\t", return_field.identifier(), " has undefined marshaling");
		break;

	case field_marshal_kind::projection_ptr:
		// TODO: add unmarshaling of [out] fields of projection
		// TODO: is [in] fields on a returned projection an error case?
		marshaling.push_back(block_if_not_null {"return_value"});

		const projection& type_definition {type.get_copy_type()->get<copy_type_kind::projection>().definition()};
		for (const std::unique_ptr<field>& pr_field : type_definition.fields()) {
			if (!caller_unmarshal_return_sub_field(marshaling, *pr_field, "return_value")) {
				return false;
			}
		}

		marshaling.push_back(end_block {});

		break;
	}

	marshaling.push_back(return_to_caller {"return_value"});

	return true;
}

bool idlc::caller_unmarshal_return_rpc(std::vector<marshal_op>& marshaling, const rpc_field& return_field)
{
	marshaling.push_back(unmarshal {"void*", "return_value"});
	marshaling.push_back(inject_trampoline {"void* return_value_trampoline", "return_value"});
	marshaling.push_back(return_to_caller {"return_value_trampoline"});
	return true;
}

bool idlc::caller_unmarshal_return_sub_field(std::vector<marshal_op>& marshaling, const field& return_field, std::string_view parent)
{
	switch (return_field.kind()) {
	case field_kind::rpc:
		if (!caller_unmarshal_return_sub_rpc(marshaling, return_field.get<field_kind::rpc>(), parent)) {
			return false;
		}

		break;

	case field_kind::var:
		if (!caller_unmarshal_return_sub_var(marshaling, return_field.get<field_kind::var>(), parent)) {
			return false;
		}

		break;
	}

	return true;
}

bool idlc::caller_unmarshal_return_sub_var(std::vector<marshal_op>& marshaling, const var_field& return_field, std::string_view parent)
{
	const type& type {return_field.get_type()};
	if (!return_field.get_type().get_copy_type()) {
		return true; // type void, no marshaling needed
	}

	const attributes type_attribs {get_attributes_with_return_default(type.get_attributes())};
	const copy_direction copy_dir {type_attribs.get_value_copy_direction()};
	if (copy_dir == copy_direction::in || copy_dir == copy_direction::both) {
		log_warning("[in] annotations are meaningless for returned projections (", return_field.identifier(), ")");
	}

	if (!(copy_dir == copy_direction::both || copy_dir == copy_direction::out)) {
		return true; // No marshaling needed
	}

	switch (get_var_marshal_kind(type)) {
	case field_marshal_kind::undefined:
		log_error("\t", return_field.identifier(), " has undefined marshaling");
		break;

	case field_marshal_kind::value:
		marshaling.push_back(unmarshal_field {get_type_string(type), {parent.data(), parent.size()}, return_field.identifier()});
		break;

	case field_marshal_kind::projection_ptr:
		// TODO: add unmarshaling of [out] fields of projection
		// TODO: is [in] fields on a returned projection an error case?

		std::string save_id {parent.data(), parent.length()};
		save_id += "_";
		save_id += return_field.identifier();
		marshaling.push_back(unmarshal {get_type_string(type), save_id});
		marshaling.push_back(store_field_indirect {{parent.data(), parent.length()}, return_field.identifier(), save_id});
		marshaling.push_back(block_if_not_null {save_id});

		const projection& type_definition {type.get_copy_type()->get<copy_type_kind::projection>().definition()};
		for (const std::unique_ptr<field>& pr_field : type_definition.fields()) {
			if (!caller_unmarshal_return_sub_field(marshaling, *pr_field, save_id)) {
				return false;
			}
		}

		marshaling.push_back(end_block {});

		break;
	}

	return true;
}

bool idlc::caller_unmarshal_return_sub_rpc(std::vector<marshal_op>& marshaling, const rpc_field& return_field, std::string_view parent)
{
	const attributes type_attribs {get_attributes_with_return_default(return_field.get_attributes())};
	const copy_direction copy_dir {type_attribs.get_value_copy_direction()};
	if (copy_dir == copy_direction::in || copy_dir == copy_direction::both) {
		log_warning("[in] annotations are meaningless for returned projections (", return_field.identifier(), ")");
	}

	if (!(copy_dir == copy_direction::both || copy_dir == copy_direction::out)) {
		return true; // No marshaling needed
	}

	std::string store_name {};
	store_name += parent;
	store_name += "_";
	store_name += return_field.identifier();
	marshaling.push_back(unmarshal {"void*", store_name});
	std::string trampoline {store_name};
	trampoline += "_trampoline";
	marshaling.push_back(inject_trampoline {"void*" + trampoline, store_name});
	marshaling.push_back(store_field_indirect {{parent.data(), parent.size()}, return_field.identifier(), trampoline});

	return true;
}

bool idlc::callee_marshal_return(std::vector<marshal_op>& marshaling, const field& return_field)
{
	switch (return_field.kind()) {
	case field_kind::rpc:
		if (!callee_marshal_return_rpc(marshaling, return_field.get<field_kind::rpc>())) {
			return false;
		}

		break;

	case field_kind::var:
		if (!callee_marshal_return_var(marshaling, return_field.get<field_kind::var>())) {
			return false;
		}

		break;
	}

	return true;
}

bool idlc::callee_marshal_return_var(std::vector<marshal_op>& marshaling, const var_field& return_field)
{
	const type& type {return_field.get_type()};
	if (!return_field.get_type().get_copy_type()) {
		return true; // type void, no marshaling needed
	}

	marshaling.push_back(marshal {"return_value"});

	switch (get_var_marshal_kind(type)) {
	case field_marshal_kind::undefined:
		log_error("\t", return_field.identifier(), " has undefined marshaling");
		break;

	case field_marshal_kind::projection_ptr:
		// TODO: add unmarshaling of [out] fields of projection
		// TODO: is [in] fields on a returned projection an error case?
		marshaling.push_back(block_if_not_null {"return_value"});

		const projection& type_definition {type.get_copy_type()->get<copy_type_kind::projection>().definition()};
		for (const std::unique_ptr<field>& pr_field : type_definition.fields()) {
			if (!callee_marshal_return_sub_field(marshaling, *pr_field, "return_value")) {
				return false;
			}
		}

		marshaling.push_back(end_block {});

		break;
	}

	return true;
}

bool idlc::callee_marshal_return_rpc(std::vector<marshal_op>& marshaling, const rpc_field& return_field)
{
	marshaling.push_back(marshal {"return_value"});
	return true;
}

bool idlc::callee_marshal_return_sub_field(std::vector<marshal_op>& marshaling, const field& return_field, std::string_view parent_ptr_id)
{
	switch (return_field.kind()) {
	case field_kind::rpc:
		if (!callee_marshal_return_sub_rpc(marshaling, return_field.get<field_kind::rpc>(), parent_ptr_id)) {
			return false;
		}

		break;

	case field_kind::var:
		if (!callee_marshal_return_sub_var(marshaling, return_field.get<field_kind::var>(), parent_ptr_id)) {
			return false;
		}

		break;
	}

	return true;
}

bool idlc::callee_marshal_return_sub_var(std::vector<marshal_op>& marshaling, const var_field& return_field, std::string_view parent)
{
	const type& type {return_field.get_type()};
	if (!return_field.get_type().get_copy_type()) {
		return true; // type void, no marshaling needed
	}

	const attributes type_attribs {get_attributes_with_return_default(type.get_attributes())};
	const copy_direction copy_dir {type_attribs.get_value_copy_direction()};
	if (copy_dir == copy_direction::in || copy_dir == copy_direction::both) {
		log_warning("[in] annotations are meaningless for returned projections (", return_field.identifier(), ")");
	}

	if (!(copy_dir == copy_direction::both || copy_dir == copy_direction::out)) {
		return true; // No marshaling needed
	}

	switch (get_var_marshal_kind(type)) {
	case field_marshal_kind::undefined:
		log_error("\t", return_field.identifier(), " has undefined marshaling");
		break;

	case field_marshal_kind::value:
		marshaling.push_back(marshal_field {{parent.data(), parent.length()}, return_field.identifier()});
		break;

	case field_marshal_kind::projection_ptr:
		// TODO: add unmarshaling of [out] fields of projection
		// TODO: is [in] fields on a returned projection an error case?

		std::string save_id {parent.data(), parent.length()};
		save_id += "_";
		save_id += return_field.identifier();
		marshaling.push_back(load_field_indirect {get_type_string(type), save_id, {parent.data(), parent.length()}, return_field.identifier()});
		marshaling.push_back(marshal {save_id});
		marshaling.push_back(block_if_not_null {save_id});

		const projection& type_definition {type.get_copy_type()->get<copy_type_kind::projection>().definition()};
		for (const std::unique_ptr<field>& pr_field : type_definition.fields()) {
			if (!callee_marshal_return_sub_field(marshaling, *pr_field, save_id)) {
				return false;
			}
		}

		marshaling.push_back(end_block {});

		break;
	}

	return true;
}

bool idlc::callee_marshal_return_sub_rpc(std::vector<marshal_op>& marshaling, const rpc_field& return_field, std::string_view parent)
{
	const attributes type_attribs {get_attributes_with_return_default(return_field.get_attributes())};
	const copy_direction copy_dir {type_attribs.get_value_copy_direction()};
	if (copy_dir == copy_direction::in || copy_dir == copy_direction::both) {
		log_warning("[in] annotations are meaningless for returned projections (", return_field.identifier(), ")");
	}

	if (!(copy_dir == copy_direction::both || copy_dir == copy_direction::out)) {
		return true; // No marshaling needed
	}

	marshaling.push_back(marshal_field {{parent.data(), parent.length()}, return_field.identifier()});

	return true;
}
