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

	class marshal_op_writer {
	public:
		void add_marshal(unsigned int source)
		{
			log_debug("\t\tmarshal ", source);
			m_ops.push_back(marshal_op::marshal);
			m_marshal_data.push_back({source});
		}

		unsigned int add_unmarshal(const std::string& type_str)
		{
			log_debug("\t", m_next_var_id, "\tunmarshal");
			m_ops.push_back(marshal_op::unmarshal);
			m_unmarshal_data.push_back({type_str});
			return m_next_var_id++;
		}

		unsigned int add_create_cspace(unsigned int pointer, const std::string& type_str)
		{
			log_debug("\t", m_next_var_id, "\tcreate_cspace ", pointer);
			m_ops.push_back(marshal_op::create_cspace);
			m_create_cspace_data.push_back({pointer, type_str});
			return m_next_var_id++;
		}

		void add_destroy_cspace(unsigned int pointer)
		{
			log_debug("\t\tdestroy_cspace ", pointer);
			m_ops.push_back(marshal_op::destroy_cspace);
			m_destroy_cspace_data.push_back({pointer});
		}

		unsigned int add_find_cspace(unsigned int pointer)
		{
			log_debug("\t", m_next_var_id, "\tfind_cspace ", pointer);
			m_ops.push_back(marshal_op::find_cspace);
			m_find_cspace_data.push_back({pointer});
			return m_next_var_id++;
		}

		unsigned int add_get_cspace(unsigned int source, const std::string& type_str)
		{
			log_debug("\t", m_next_var_id, "\tget_cspace ", source);
			m_ops.push_back(marshal_op::get_cspace);
			m_get_cspace_data.push_back({source, type_str});
			return m_next_var_id++;
		}

		void add_parameter(unsigned int source, const std::string& type_str, const std::string& arg_str)
		{
			log_debug("\t\tparameter ", source);
			m_ops.push_back(marshal_op::parameter);
			m_parameter_data.push_back({source, type_str, arg_str});
		}

		unsigned int add_argument(const std::string& type_str, const std::string& param_str)
		{
			log_debug("\t", m_next_var_id, "\targument ", param_str);
			m_ops.push_back(marshal_op::argument);
			m_argument_data.push_back({type_str, param_str});
			return m_next_var_id++;
		}

		unsigned int add_get_return_value(const std::string& type_str)
		{
			log_debug("\t", m_next_var_id, "\tget_return_value");
			m_ops.push_back(marshal_op::get_return_value);
			m_get_return_value_data.push_back({type_str});
			return m_next_var_id++;
		}

		void add_return_var(unsigned int source)
		{
			log_debug("\t\treturn_var ", source);
			m_ops.push_back(marshal_op::get_return_value);
			m_return_var_data.push_back({source});
		}

		unsigned int add_get(unsigned int parent, const std::string& child_field_str, const std::string& type_str)
		{
			log_debug("\t", m_next_var_id, "\tget ", parent, " ", child_field_str);
			m_ops.push_back(marshal_op::get);
			m_get_data.push_back({parent, child_field_str, type_str});
			return m_next_var_id++;
		}

		void add_set(unsigned int parent, unsigned int source, const std::string& child_field_str)
		{
			log_debug("\t\tset ", parent, " ", child_field_str, " ", source);
			m_ops.push_back(marshal_op::set);
			m_set_data.push_back({parent, source, child_field_str});
		}

		void add_call(const std::string& return_type, const std::string& arguments_str)
		{
			log_debug("\t\tcall");
			m_ops.push_back(marshal_op::call);
			m_call_data.push_back({return_type, arguments_str});
		}

		void add_call_indirect(unsigned int source, const std::string& function_type, const std::string& return_type, const std::string& arguments_str)
		{
			log_debug("\t\tcall_indirect ", source);
			m_ops.push_back(marshal_op::call_indirect);
			m_call_indirect_data.push_back({source, function_type, return_type, arguments_str});
		}

		void add_send(const std::string& rpc_id_str)
		{
			log_debug("\t\tsend");
			m_ops.push_back(marshal_op::send);
			m_send_data.push_back({rpc_id_str});
		}

		void add_if_not_null(unsigned int pointer)
		{
			log_debug("\t\tif_not_null ", pointer);
			m_ops.push_back(marshal_op::if_not_null);
			m_if_not_null_data.push_back({pointer});
		}

		void add_end_if_not_null()
		{
			log_debug("\t\tend_if_not_null");
			m_ops.push_back(marshal_op::end_if_not_null);
		}

		unsigned int add_inject_trampoline(unsigned int ptr_id, gsl::czstring<> rpc_id)
		{
			log_debug("\t", m_next_var_id, "\tinject_trampoline ", ptr_id);
			m_ops.push_back(marshal_op::inject_trampoline);
			m_inject_trampoline_data.push_back({ptr_id, rpc_id});
			return m_next_var_id++;
		}

		marshal_op_list move_to_list()
		{
			return {
				std::move(m_ops),
				std::move(m_marshal_data),
				std::move(m_unmarshal_data),
				std::move(m_create_cspace_data),
				std::move(m_destroy_cspace_data),
				std::move(m_find_cspace_data),
				std::move(m_get_cspace_data),
				std::move(m_argument_data),
				std::move(m_parameter_data),
				std::move(m_get_return_value_data),
				std::move(m_return_var_data),
				std::move(m_get_data),
				std::move(m_set_data),
				std::move(m_call_data),
				std::move(m_call_indirect_data),
				std::move(m_send_data),
				std::move(m_if_not_null_data),
				std::move(m_inject_trampoline_data)
			};
		}

	private:
		unsigned int m_next_var_id {0};
		std::vector<marshal_op> m_ops;
		std::vector<marshal_data> m_marshal_data;
		std::vector<unmarshal_data> m_unmarshal_data;
		std::vector<create_cspace_data> m_create_cspace_data;
		std::vector<destroy_cspace_data> m_destroy_cspace_data;
		std::vector<find_cspace_data> m_find_cspace_data;
		std::vector<get_cspace_data> m_get_cspace_data;
		std::vector<argument_data> m_argument_data;
		std::vector<parameter_data> m_parameter_data;
		std::vector<get_return_value_data> m_get_return_value_data;
		std::vector<return_var_data> m_return_var_data;
		std::vector<get_data> m_get_data;
		std::vector<set_data> m_set_data;
		std::vector<call_data> m_call_data;
		std::vector<call_indirect_data> m_call_indirect_data;
		std::vector<send_data> m_send_data;
		std::vector<if_not_null_data> m_if_not_null_data;
		std::vector<inject_trampoline_data> m_inject_trampoline_data;
	};

	bool process_caller(marshal_op_writer& marshaling, const marshal_unit & unit, marshal_unit_kind kind);
	bool process_callee(marshal_op_writer& marshaling, const marshal_unit & unit, marshal_unit_kind kind);

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

	bool callee_insert_call(marshal_op_writer& marshaling, const signature& signature);
	bool callee_insert_call_indirect(marshal_op_writer& marshaling, const signature& signature, unsigned int source);

	bool caller_marshal_argument(marshal_op_writer& marshaling, const field& argument, std::vector<unsigned int>& caller_argument_save_ids);
	bool caller_marshal_argument_rpc(marshal_op_writer& marshaling, const rpc_field& rpc);
	bool caller_marshal_argument_var(marshal_op_writer& marshaling, const var_field& rpc, std::vector<unsigned int>& remarshal_ptr_ids);
	bool caller_marshal_argument_sub_rpc(marshal_op_writer& marshaling, const rpc_field& rpc, unsigned int parent_ptr_id);
	bool caller_marshal_argument_sub_var(marshal_op_writer& marshaling, const var_field& var, unsigned int parent_ptr_id);
	bool caller_marshal_argument_sub_field(marshal_op_writer& marshaling, const field& proj_field, unsigned int parent_ptr_id);

	bool caller_remarshal_argument(marshal_op_writer& marshaling, const field& argument, unsigned int save_id);
	bool caller_remarshal_argument_var(marshal_op_writer& marshaling, const var_field& rpc, unsigned int save_id);
	bool caller_remarshal_argument_sub_rpc(marshal_op_writer& marshaling, const rpc_field& rpc, unsigned int parent_ptr_id);
	bool caller_remarshal_argument_sub_var(marshal_op_writer& marshaling, const var_field& var, unsigned int parent_ptr_id);
	bool caller_remarshal_argument_sub_field(marshal_op_writer& marshaling, const field& proj_field, unsigned int parent_ptr_id);

	bool callee_unmarshal_argument(marshal_op_writer& marshaling, const field& argument, std::vector<unsigned int>& argument_save_ids);
	bool callee_unmarshal_argument_rpc(marshal_op_writer& marshaling, const rpc_field& rpc);
	bool callee_unmarshal_argument_var(marshal_op_writer& marshaling, const var_field& var, std::vector<unsigned int>& argument_save_ids);
	bool callee_unmarshal_argument_sub_rpc(marshal_op_writer& marshaling, const rpc_field& rpc, unsigned int parent_ptr_id);
	bool callee_unmarshal_argument_sub_var(marshal_op_writer& marshaling, const var_field& var, unsigned int parent_ptr_id);
	bool callee_unmarshal_argument_sub_field(marshal_op_writer& marshaling, const field& proj_field, unsigned int parent_ptr_id);

	bool callee_remarshal_argument(marshal_op_writer& marshaling, const field& argument, unsigned int save_id);
	bool callee_remarshal_argument_var(marshal_op_writer& marshaling, const var_field& var, unsigned int save_id);
	bool callee_remarshal_argument_sub_rpc(marshal_op_writer& marshaling, const rpc_field& rpc, unsigned int parent_ptr_id);
	bool callee_remarshal_argument_sub_var(marshal_op_writer& marshaling, const var_field& var, unsigned int parent_ptr_id);
	bool callee_remarshal_argument_sub_field(marshal_op_writer& marshaling, const field& proj_field, unsigned int parent_ptr_id);

	// No such thing as "remarshaling" a return value (hence the fact that an [in] field on a return value is meaningless)
	// NOTE: this is easily changed, it just uses the annotation to decide whether to skip the field or not
	// So we can trivially invert the annotations for return values

	bool caller_unmarshal_return(marshal_op_writer& marshaling, const field& return_field);
	bool caller_unmarshal_return_var(marshal_op_writer& marshaling, const var_field& return_field);
	bool caller_unmarshal_return_rpc(marshal_op_writer& marshaling, const rpc_field& return_field);
	bool caller_unmarshal_return_sub_field(marshal_op_writer& marshaling, const field& return_field, unsigned int parent_ptr_id);
	bool caller_unmarshal_return_sub_var(marshal_op_writer& marshaling, const var_field& return_field, unsigned int parent_ptr_id);
	bool caller_unmarshal_return_sub_rpc(marshal_op_writer& marshaling, const rpc_field& return_field, unsigned int parent_ptr_id);

	bool callee_marshal_return(marshal_op_writer& marshaling, const field& return_field);
	bool callee_marshal_return_var(marshal_op_writer& marshaling, const var_field& return_field);
	bool callee_marshal_return_rpc(marshal_op_writer& marshaling, const rpc_field& return_field);
	bool callee_marshal_return_sub_field(marshal_op_writer& marshaling, const field& return_field, unsigned int parent_ptr_id);
	bool callee_marshal_return_sub_var(marshal_op_writer& marshaling, const var_field& return_field, unsigned int parent_ptr_id);
	bool callee_marshal_return_sub_rpc(marshal_op_writer& marshaling, const rpc_field& return_field, unsigned int parent_ptr_id);
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

// TODO: add handling of indirect call for function pointers
bool idlc::process_marshal_units(gsl::span<const marshal_unit> units, marshal_unit_kind kind, std::vector<marshal_unit_lists>& unit_marshaling)
{
	unit_marshaling.clear();
	unit_marshaling.reserve(units.size());

	for (const marshal_unit& unit : units) {
		marshal_op_writer caller_marshaling;
		marshal_op_writer callee_marshaling;
		log_debug(unit.identifier, ":");
		process_caller(caller_marshaling, unit, kind);
		process_callee(callee_marshaling, unit, kind);
		unit_marshaling.push_back({unit.identifier, caller_marshaling.move_to_list(), callee_marshaling.move_to_list()});
	}

	return true;
}

bool idlc::process_caller(marshal_op_writer& marshaling, const marshal_unit& unit, marshal_unit_kind kind)
{
	log_debug("Caller-side:");

	const signature& signature {*unit.rpc_signature};

	if (kind == marshal_unit_kind::indirect) {
		// rpc pointer stubs take the real pointer as their first argument
		const unsigned int save_id {marshaling.add_argument("void*", "underlying")};
		marshaling.add_marshal(save_id);
	}

	// To keep track of projection pointers that need re-marshaling for [out] fields
	std::vector<unsigned int> caller_argument_save_ids(signature.arguments().size());
	caller_argument_save_ids.resize(0); // Keep the reserved space while allowing for push_back()

	for (const std::unique_ptr<field>& field : signature.arguments()) {
		if (!caller_marshal_argument(marshaling, *field, caller_argument_save_ids)) {
			return false;
		}
	}

	marshaling.add_send(unit.identifier);

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

bool idlc::process_callee(marshal_op_writer& marshaling, const marshal_unit & unit, marshal_unit_kind kind)
{
	log_debug("Callee-side:");

	const signature& signature {*unit.rpc_signature};

	unsigned int underlying_ptr_save_id {0};
	if (kind == marshal_unit_kind::indirect) {
		// rpc pointer stubs take the real pointer as their first argument
		underlying_ptr_save_id = marshaling.add_unmarshal("void*");
	}

	// To keep track of projection pointers that need re-marshaling for [out] fields
	std::vector<unsigned int> callee_argument_save_ids(signature.arguments().size());
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
		callee_insert_call(marshaling, signature);
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

bool idlc::callee_insert_call(marshal_op_writer& marshaling, const signature& signature)
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
		marshaling.add_call(get_type_string(return_field.get<field_kind::var>().get_type()), argument_string.str());
		break;

	case field_kind::rpc:
		marshaling.add_call("void*", argument_string.str());
		break;
	}

	return true;
}

bool idlc::callee_insert_call_indirect(marshal_op_writer& marshaling, const signature& signature, unsigned int source)
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
		marshaling.add_call_indirect(
			source,
			function_type,
			get_type_string(return_field.get<field_kind::var>().get_type()),
			argument_string.str()
		);

		break;

	case field_kind::rpc:
		marshaling.add_call_indirect(
			source,
			function_type,
			"void*",
			argument_string.str()
		);

		break;
	}

	return true;
}

bool idlc::caller_marshal_argument_rpc(marshal_op_writer& marshaling, const rpc_field& rpc)
{
	// NOTE: I think we can get away with the void* trick due to C's looser type system
	const unsigned int save_id {marshaling.add_argument("void*", rpc.identifier())};
	marshaling.add_marshal(save_id);
	return true;
}

bool idlc::caller_marshal_argument_var(marshal_op_writer& marshaling, const var_field& var, std::vector<unsigned int>& argument_save_ids)
{
	const type& type {var.get_type()};

	unsigned int save_id {marshaling.add_argument(get_type_string(type), var.identifier())};
	argument_save_ids.push_back(save_id);
	marshaling.add_marshal(save_id);

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

		marshaling.add_if_not_null(save_id);

		const projection& type_definition {type.get_copy_type()->get<copy_type_kind::projection>().definition()};
		for (const std::unique_ptr<field>& pr_field : type_definition.fields()) {
			if (!caller_marshal_argument_sub_field(marshaling, *pr_field, save_id)) {
				return false;
			}
		}

		marshaling.add_end_if_not_null();

		break;
	}
	}

	return true;
}

bool idlc::caller_marshal_argument_sub_rpc(marshal_op_writer& marshaling, const rpc_field& rpc, unsigned int parent_ptr_id)
{
	attributes attribs {get_attributes_with_argument_default(rpc.get_attributes())};
	const copy_direction field_copy_direction {attribs.get_value_copy_direction()};
	if (!(field_copy_direction == copy_direction::both
		|| field_copy_direction == copy_direction::in))
	{
		return true; // Do nothing, don't have to marshal
	}

	// NOTE: I think we can get away with the void* trick due to C's looser type system
	const unsigned int save_id {marshaling.add_get(parent_ptr_id, rpc.identifier(), "void*")};
	marshaling.add_marshal(save_id);
	return true;
}

bool idlc::caller_marshal_argument_sub_var(marshal_op_writer& marshaling, const var_field& var, unsigned int parent_ptr_id)
{
	const type& type {var.get_type()};

	attributes attribs {get_attributes_with_argument_default(type.get_attributes())};
	const copy_direction field_copy_direction {attribs.get_value_copy_direction()};
	if (!(field_copy_direction == copy_direction::both
		|| field_copy_direction == copy_direction::in))
	{
		return true; // Do nothing, don't have to marshal
	}

	unsigned int save_id {marshaling.add_get(parent_ptr_id, var.identifier(), get_type_string(type))};

	const field_marshal_kind marshal_kind {get_var_marshal_kind(type)};
	switch (marshal_kind) {
	case field_marshal_kind::undefined:
		log_error("\t", var.identifier(), " has undefined marshaling");
		return false;

	case field_marshal_kind::value:
		marshaling.add_marshal(save_id);
		break;

	case field_marshal_kind::projection_ptr: {
		// Recurse into projection fields
		// Needs save ID of the projection pointer and the projection definition
		marshaling.add_marshal(save_id);
		marshaling.add_if_not_null(save_id);

		const projection& type_definition {type.get_copy_type()->get<copy_type_kind::projection>().definition()};
		for (const std::unique_ptr<field>& pr_field : type_definition.fields()) {
			if (!caller_marshal_argument_sub_field(marshaling, *pr_field, save_id)) {
				return false;
			}
		}

		marshaling.add_end_if_not_null();

		break;
	}
	}

	return true;
}

bool idlc::caller_marshal_argument_sub_field(marshal_op_writer& marshaling, const field& proj_field, unsigned int parent_ptr_id)
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

bool idlc::caller_remarshal_argument(marshal_op_writer& marshaling, const field& argument, unsigned int save_id)
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

bool idlc::caller_remarshal_argument_var(marshal_op_writer& marshaling, const var_field& var, unsigned int save_id)
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

		marshaling.add_if_not_null(save_id);

		const projection& type_definition {type.get_copy_type()->get<copy_type_kind::projection>().definition()};
		for (const std::unique_ptr<field>& pr_field : type_definition.fields()) {
			if (!caller_remarshal_argument_sub_field(marshaling, *pr_field, save_id)) {
				return false;
			}
		}

		marshaling.add_end_if_not_null();

		break;
	}

	return true;
}

bool idlc::caller_remarshal_argument_sub_rpc(marshal_op_writer& marshaling, const rpc_field& rpc, unsigned int parent_ptr_id)
{
	attributes attribs {get_attributes_with_argument_default(rpc.get_attributes())};
	const copy_direction field_copy_direction {attribs.get_value_copy_direction()};
	if (!(field_copy_direction == copy_direction::both
		|| field_copy_direction == copy_direction::out))
	{
		return true; // Do nothing, don't have to marshal
	}

	// NOTE: I think we can get away with the void* trick due to C's looser type system
	const unsigned int save_id {marshaling.add_inject_trampoline(marshaling.add_unmarshal("void*"), rpc.mangled_signature)};
	marshaling.add_set(parent_ptr_id, save_id, rpc.identifier());
	return true;
}

bool idlc::caller_remarshal_argument_sub_var(marshal_op_writer& marshaling, const var_field& var, unsigned int parent_ptr_id)
{
	const type& type {var.get_type()};

	attributes attribs {get_attributes_with_argument_default(type.get_attributes())};
	const copy_direction field_copy_direction {attribs.get_value_copy_direction()};
	if (!(field_copy_direction == copy_direction::both
		|| field_copy_direction == copy_direction::out))
	{
		return true; // Do nothing, don't have to marshal
	}

	const unsigned int save_id {marshaling.add_unmarshal(get_type_string(type))};
	marshaling.add_set(parent_ptr_id, save_id, var.identifier());
	const field_marshal_kind marshal_kind {get_var_marshal_kind(type)};

	switch (marshal_kind) {
	case field_marshal_kind::undefined:
		log_error("\t", var.identifier(), " has undefined marshaling");
		return false;

	case field_marshal_kind::projection_ptr:
		// Recurse into projection fields
		// Needs save ID of the projection pointer and the projection definition

		marshaling.add_if_not_null(save_id);

		const projection& type_definition {type.get_copy_type()->get<copy_type_kind::projection>().definition()};
		for (const std::unique_ptr<field>& pr_field : type_definition.fields()) {
			if (!caller_remarshal_argument_sub_field(marshaling, *pr_field, save_id)) {
				return false;
			}
		}

		marshaling.add_end_if_not_null();

		break;
	}

	return true;
}

bool idlc::caller_remarshal_argument_sub_field(marshal_op_writer& marshaling, const field& proj_field, unsigned int parent_ptr_id)
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

bool idlc::caller_marshal_argument(marshal_op_writer& marshaling, const field& argument, std::vector<unsigned int>& caller_argument_save_ids)
{
	switch (argument.kind()) {
	case field_kind::rpc:
		// Placeholder, easily shows in generated code if improperly used
		// Anticipating need for other kinds of remarshaling
		caller_argument_save_ids.push_back(std::numeric_limits<unsigned int>::max());
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

bool idlc::callee_unmarshal_argument_rpc(marshal_op_writer& marshaling, const rpc_field& rpc)
{
	// NOTE: I think we can get away with the void* trick due to C's looser type system
	const unsigned int save_id {marshaling.add_inject_trampoline(marshaling.add_unmarshal("void*"), rpc.mangled_signature)};
	marshaling.add_parameter(save_id, "void*", rpc.identifier());
	return true;
}

bool idlc::callee_unmarshal_argument_var(marshal_op_writer& marshaling, const var_field& var, std::vector<unsigned int>& argument_save_ids)
{
	const type& type {var.get_type()};
	const std::string type_string {get_type_string(type)};

	const unsigned int save_id {marshaling.add_unmarshal(get_type_string(type))};
	argument_save_ids.push_back(save_id);
	marshaling.add_parameter(save_id, type_string, var.identifier());

	const field_marshal_kind marshal_kind {get_var_marshal_kind(type)};
	switch (marshal_kind) {
	case field_marshal_kind::undefined:
		log_error("\t", var.identifier(), " has undefined marshaling");
		return false;

	case field_marshal_kind::projection_ptr:
		// Recurse into projection fields
		// Needs save ID of the projection pointer and the projection definition

		marshaling.add_if_not_null(save_id);

		const projection& type_definition {type.get_copy_type()->get<copy_type_kind::projection>().definition()};
		for (const std::unique_ptr<field>& pr_field : type_definition.fields()) {
			if (!callee_unmarshal_argument_sub_field(marshaling, *pr_field, save_id)) {
				return false;
			}
		}

		marshaling.add_end_if_not_null();

		break;
	}

	return true;
}

bool idlc::callee_unmarshal_argument_sub_rpc(marshal_op_writer& marshaling, const rpc_field& rpc, unsigned int parent_ptr_id)
{
	attributes attribs {get_attributes_with_argument_default(rpc.get_attributes())};
	const copy_direction field_copy_direction {attribs.get_value_copy_direction()};
	if (!(field_copy_direction == copy_direction::both
		|| field_copy_direction == copy_direction::in))
	{
		return true; // Do nothing, don't have to marshal
	}

	// NOTE: I think we can get away with the void* trick due to C's looser type system
	const unsigned int save_id {marshaling.add_inject_trampoline(marshaling.add_unmarshal("void*"), rpc.mangled_signature)};
	marshaling.add_set(parent_ptr_id, save_id, rpc.identifier());
	return true;
}

bool idlc::callee_unmarshal_argument_sub_var(marshal_op_writer& marshaling, const var_field& var, unsigned int parent_ptr_id)
{
	const type& type {var.get_type()};

	attributes attribs {get_attributes_with_argument_default(type.get_attributes())};
	const copy_direction field_copy_direction {attribs.get_value_copy_direction()};
	if (!(field_copy_direction == copy_direction::both
		|| field_copy_direction == copy_direction::in))
	{
		return true; // Do nothing, don't have to marshal
	}

	const unsigned int save_id {marshaling.add_unmarshal(get_type_string(type))};
	marshaling.add_set(parent_ptr_id, save_id, var.identifier());
	const field_marshal_kind marshal_kind {get_var_marshal_kind(type)};

	switch (marshal_kind) {
	case field_marshal_kind::undefined:
		log_error("\t", var.identifier(), " has undefined marshaling");
		return false;

	case field_marshal_kind::projection_ptr:
		// Recurse into projection fields
		// Needs save ID of the projection pointer and the projection definition

		marshaling.add_if_not_null(save_id);

		const projection& type_definition {type.get_copy_type()->get<copy_type_kind::projection>().definition()};
		for (const std::unique_ptr<field>& pr_field : type_definition.fields()) {
			if (!callee_unmarshal_argument_sub_field(marshaling, *pr_field, save_id)) {
				return false;
			}
		}

		marshaling.add_end_if_not_null();

		break;
	}

	return true;
}

bool idlc::callee_unmarshal_argument_sub_field(marshal_op_writer& marshaling, const field& proj_field, unsigned int parent_ptr_id)
{
	switch (proj_field.kind()) {
	case field_kind::rpc:
		if (!callee_unmarshal_argument_sub_rpc(marshaling, proj_field.get<field_kind::rpc>(), parent_ptr_id)) {
			return false;
		}

		break;

	case field_kind::var:
		if (!callee_unmarshal_argument_sub_var(marshaling, proj_field.get<field_kind::var>(), parent_ptr_id)) {
			return false;
		}

		break;
	}

	return true;
}

bool idlc::callee_unmarshal_argument(marshal_op_writer& marshaling, const field& argument, std::vector<unsigned int>& argument_save_ids)
{
	switch (argument.kind()) {
	case field_kind::rpc:
		// Placeholder value
		argument_save_ids.push_back(std::numeric_limits<unsigned int>::max());
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

bool idlc::callee_remarshal_argument(marshal_op_writer& marshaling, const field& argument, unsigned int save_id)
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

bool idlc::callee_remarshal_argument_var(marshal_op_writer& marshaling, const var_field& var, unsigned int save_id)
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

		marshaling.add_if_not_null(save_id);

		const projection& type_definition {type.get_copy_type()->get<copy_type_kind::projection>().definition()};
		for (const std::unique_ptr<field>& pr_field : type_definition.fields()) {
			if (!callee_remarshal_argument_sub_field(marshaling, *pr_field, save_id)) {
				return false;
			}
		}

		marshaling.add_end_if_not_null();

		break;
	}
	}

	return true;
}

bool idlc::callee_remarshal_argument_sub_rpc(marshal_op_writer& marshaling, const rpc_field& rpc, unsigned int parent_ptr_id)
{
	attributes attribs {get_attributes_with_argument_default(rpc.get_attributes())};
	const copy_direction field_copy_direction {attribs.get_value_copy_direction()};
	if (!(field_copy_direction == copy_direction::both
		|| field_copy_direction == copy_direction::out))
	{
		return true; // Do nothing, don't have to marshal
	}

	// NOTE: I think we can get away with the void* trick due to C's looser type system
	const unsigned int save_id {marshaling.add_get(parent_ptr_id, rpc.identifier(), "void*")};
	marshaling.add_marshal(save_id);

	return true;
}

bool idlc::callee_remarshal_argument_sub_var(marshal_op_writer& marshaling, const var_field& var, unsigned int parent_ptr_id)
{
	const type& type {var.get_type()};

	attributes attribs {get_attributes_with_argument_default(type.get_attributes())};
	const copy_direction field_copy_direction {attribs.get_value_copy_direction()};
	if (!(field_copy_direction == copy_direction::both
		|| field_copy_direction == copy_direction::out))
	{
		return true; // Do nothing, don't have to marshal
	}

	unsigned int save_id {marshaling.add_get(parent_ptr_id, var.identifier(), get_type_string(type))};

	const field_marshal_kind marshal_kind {get_var_marshal_kind(type)};
	switch (marshal_kind) {
	case field_marshal_kind::undefined:
		log_error("\t", var.identifier(), " has undefined marshaling");
		return false;

	case field_marshal_kind::value:
		marshaling.add_marshal(save_id);
		break;

	case field_marshal_kind::projection_ptr: {
		// Recurse into projection fields
		// Needs save ID of the projection pointer and the projection definition
		marshaling.add_marshal(save_id);
		marshaling.add_if_not_null(save_id);

		const projection& type_definition {type.get_copy_type()->get<copy_type_kind::projection>().definition()};
		for (const std::unique_ptr<field>& pr_field : type_definition.fields()) {
			if (!callee_remarshal_argument_sub_field(marshaling, *pr_field, save_id)) {
				return false;
			}
		}

		marshaling.add_end_if_not_null();

		break;
	}
	}

	return true;
}

bool idlc::callee_remarshal_argument_sub_field(marshal_op_writer& marshaling, const field& proj_field, unsigned int parent_ptr_id)
{
	switch (proj_field.kind()) {
	case field_kind::rpc:
		if (!callee_remarshal_argument_sub_rpc(marshaling, proj_field.get<field_kind::rpc>(), parent_ptr_id)) {
			return false;
		}

		break;

	case field_kind::var:
		if (!callee_remarshal_argument_sub_var(marshaling, proj_field.get<field_kind::var>(), parent_ptr_id)) {
			return false;
		}

		break;
	}

	return true;
}

bool idlc::caller_unmarshal_return(marshal_op_writer& marshaling, const field& return_field)
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

bool idlc::caller_unmarshal_return_var(marshal_op_writer& marshaling, const var_field& return_field)
{
	const type& type {return_field.get_type()};
	if (!return_field.get_type().get_copy_type()) {
		return true; // type void, no marshaling needed
	}

	const unsigned int save_id {marshaling.add_unmarshal(get_type_string(type))};

	switch (get_var_marshal_kind(type)) {
	case field_marshal_kind::undefined:
		log_error("\t", return_field.identifier(), " has undefined marshaling");
		break;

	case field_marshal_kind::projection_ptr:
		// TODO: add unmarshaling of [out] fields of projection
		// TODO: is [in] fields on a returned projection an error case?
		marshaling.add_if_not_null(save_id);
		
		const projection& type_definition {type.get_copy_type()->get<copy_type_kind::projection>().definition()};
		for (const std::unique_ptr<field>& pr_field : type_definition.fields()) {
			if (!caller_unmarshal_return_sub_field(marshaling, *pr_field, save_id)) {
				return false;
			}
		}
		
		marshaling.add_end_if_not_null();

		break;
	}

	marshaling.add_return_var(save_id);

	return true;
}

bool idlc::caller_unmarshal_return_rpc(marshal_op_writer& marshaling, const rpc_field& return_field)
{
	unsigned int save_id {marshaling.add_unmarshal("void*")};
	save_id = marshaling.add_inject_trampoline(save_id, return_field.mangled_signature);
	marshaling.add_return_var(save_id);
	return true;
}

bool idlc::caller_unmarshal_return_sub_field(marshal_op_writer& marshaling, const field& return_field, unsigned int parent_ptr_id)
{
	switch (return_field.kind()) {
	case field_kind::rpc:
		if (!caller_unmarshal_return_sub_rpc(marshaling, return_field.get<field_kind::rpc>(), parent_ptr_id)) {
			return false;
		}

		break;

	case field_kind::var:
		if (!caller_unmarshal_return_sub_var(marshaling, return_field.get<field_kind::var>(), parent_ptr_id)) {
			return false;
		}

		break;
	}

	return true;
}

bool idlc::caller_unmarshal_return_sub_var(marshal_op_writer& marshaling, const var_field& return_field, unsigned int parent_ptr_id)
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

	const unsigned int save_id {marshaling.add_unmarshal(get_type_string(type))};
	marshaling.add_set(parent_ptr_id, save_id, return_field.identifier());

	switch (get_var_marshal_kind(type)) {
	case field_marshal_kind::undefined:
		log_error("\t", return_field.identifier(), " has undefined marshaling");
		break;

	case field_marshal_kind::projection_ptr:
		// TODO: add unmarshaling of [out] fields of projection
		// TODO: is [in] fields on a returned projection an error case?
		marshaling.add_if_not_null(save_id);

		const projection& type_definition {type.get_copy_type()->get<copy_type_kind::projection>().definition()};
		for (const std::unique_ptr<field>& pr_field : type_definition.fields()) {
			if (!caller_unmarshal_return_sub_field(marshaling, *pr_field, save_id)) {
				return false;
			}
		}

		marshaling.add_end_if_not_null();

		break;
	}

	return true;
}

bool idlc::caller_unmarshal_return_sub_rpc(marshal_op_writer& marshaling, const rpc_field& return_field, unsigned int parent_ptr_id)
{
	const attributes type_attribs {get_attributes_with_return_default(return_field.get_attributes())};
	const copy_direction copy_dir {type_attribs.get_value_copy_direction()};
	if (copy_dir == copy_direction::in || copy_dir == copy_direction::both) {
		log_warning("[in] annotations are meaningless for returned projections (", return_field.identifier(), ")");
	}

	if (!(copy_dir == copy_direction::both || copy_dir == copy_direction::out)) {
		return true; // No marshaling needed
	}

	unsigned int save_id {marshaling.add_unmarshal("void*")};
	save_id = marshaling.add_inject_trampoline(save_id, return_field.mangled_signature);
	marshaling.add_set(parent_ptr_id, save_id, return_field.identifier());
	return true;
}

bool idlc::callee_marshal_return(marshal_op_writer& marshaling, const field& return_field)
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

bool idlc::callee_marshal_return_var(marshal_op_writer& marshaling, const var_field& return_field)
{
	const type& type {return_field.get_type()};
	if (!return_field.get_type().get_copy_type()) {
		return true; // type void, no marshaling needed
	}

	const unsigned int save_id {marshaling.add_get_return_value(get_type_string(type))};
	marshaling.add_marshal(save_id);

	switch (get_var_marshal_kind(type)) {
	case field_marshal_kind::undefined:
		log_error("\t", return_field.identifier(), " has undefined marshaling");
		break;

	case field_marshal_kind::projection_ptr:
		// TODO: add unmarshaling of [out] fields of projection
		// TODO: is [in] fields on a returned projection an error case?
		marshaling.add_if_not_null(save_id);

		const projection& type_definition {type.get_copy_type()->get<copy_type_kind::projection>().definition()};
		for (const std::unique_ptr<field>& pr_field : type_definition.fields()) {
			if (!callee_marshal_return_sub_field(marshaling, *pr_field, save_id)) {
				return false;
			}
		}

		marshaling.add_end_if_not_null();

		break;
	}

	return true;
}

bool idlc::callee_marshal_return_rpc(marshal_op_writer& marshaling, const rpc_field& return_field)
{
	unsigned int save_id {marshaling.add_get_return_value("void*")};
	marshaling.add_marshal(save_id);
	return true;
}

bool idlc::callee_marshal_return_sub_field(marshal_op_writer& marshaling, const field& return_field, unsigned int parent_ptr_id)
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

bool idlc::callee_marshal_return_sub_var(marshal_op_writer& marshaling, const var_field& return_field, unsigned int parent_ptr_id)
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

	const unsigned int save_id {marshaling.add_get(parent_ptr_id, return_field.identifier(), get_type_string(type))};
	marshaling.add_marshal(save_id);

	switch (get_var_marshal_kind(type)) {
	case field_marshal_kind::undefined:
		log_error("\t", return_field.identifier(), " has undefined marshaling");
		break;

	case field_marshal_kind::projection_ptr:
		// TODO: add unmarshaling of [out] fields of projection
		// TODO: is [in] fields on a returned projection an error case?
		marshaling.add_if_not_null(save_id);

		const projection& type_definition {type.get_copy_type()->get<copy_type_kind::projection>().definition()};
		for (const std::unique_ptr<field>& pr_field : type_definition.fields()) {
			if (!callee_marshal_return_sub_field(marshaling, *pr_field, save_id)) {
				return false;
			}
		}

		marshaling.add_end_if_not_null();

		break;
	}

	return true;
}

bool idlc::callee_marshal_return_sub_rpc(marshal_op_writer& marshaling, const rpc_field& return_field, unsigned int parent_ptr_id)
{
	const attributes type_attribs {get_attributes_with_return_default(return_field.get_attributes())};
	const copy_direction copy_dir {type_attribs.get_value_copy_direction()};
	if (copy_dir == copy_direction::in || copy_dir == copy_direction::both) {
		log_warning("[in] annotations are meaningless for returned projections (", return_field.identifier(), ")");
	}

	if (!(copy_dir == copy_direction::both || copy_dir == copy_direction::out)) {
		return true; // No marshaling needed
	}

	unsigned int save_id {marshaling.add_get(parent_ptr_id, return_field.identifier(), "void*")};
	marshaling.add_marshal(save_id);
	return true;
}
