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
	field_marshal_kind find_marshal_kind(const type& ty);

	bool caller_marshal_signature(marshal_op_list_writer& writer, const signature& sig, std::vector<unsigned int>& out_ptrs);

	bool caller_unmarshal_projection_subfield(marshal_op_list_writer& writer, const var_field& field, unsigned int parent);
	bool caller_unmarshal_signature(marshal_op_list_writer& writer, const signature& sig, std::vector<unsigned int>& out_ptrs);

	unsigned int get_field(marshal_op_list_writer& writer, unsigned int parent, const var_field& field);

	bool marshal_projection_ptr(marshal_op_list_writer& writer, unsigned int id, projection& def);
	void marshal_value(marshal_op_list_writer& writer, unsigned int id);
	bool marshal_projection_ptr(marshal_op_list_writer& writer, unsigned int id, projection& def);
	void marshal_value(marshal_op_list_writer& writer, unsigned int id);

	void unmarshal_subfield(marshal_op_list_writer& writer, const var_field& field, unsigned int parent);

	class caller_marshal_subfield_pass : public generic_pass<caller_marshal_subfield_pass> {
	public:
		caller_marshal_subfield_pass(marshal_op_list_writer& writer, unsigned int parent) :
			m_writer {writer},
			m_parent {parent}
		{
		}

		bool visit_rpc_field(const rpc_field&)
		{
			log_warning("\tAn RPC field has been ignored during marshaling");
			log_debug("\t\tImplementation coming soon");
			return true;
		}

		bool visit_var_field(const var_field& field)
		{
			switch (find_marshal_kind(field.get_type())) {
			case field_marshal_kind::undefined:
				log_debug("\tMarshaling for this field is undefined");
				break;

			case field_marshal_kind::value: {
				const auto copy_dir = field.get_type().get_attributes()->get_value_copy_direction();
				if (copy_dir == copy_direction::in || copy_dir == copy_direction::both) {
					marshal_value(m_writer, get_field(m_writer, m_parent, field));
				}

				break;
			}

			case field_marshal_kind::projection_ptr:
				if (!marshal_projection_ptr(
					m_writer,
					get_field(m_writer, m_parent, field),
					field.get_type().get_copy_type()->get<copy_type_kind::projection>().definition()
				))
				{
					return false;
				}

				break;
			}

			return true;
		}

	private:
		marshal_op_list_writer& m_writer;
		unsigned int m_parent;
	};

	class caller_marshal_arguments_pass : public generic_pass<caller_marshal_arguments_pass> {
	public:
		caller_marshal_arguments_pass(marshal_op_list_writer& writer, std::vector<unsigned int>& out_ptrs) :
			m_writer {writer},
			m_out_ptrs {out_ptrs}
		{
		}

		bool visit_rpc_field(const rpc_field&)
		{
			log_warning("\tAn RPC field has been ignored during marshaling");
			log_debug("\t\tImplementation coming soon");
			return true;
		}

		bool visit_var_field(const var_field& field)
		{
			const auto type_str = get_type_string(field.get_type());
			const auto arg_id = m_writer.add_argument(type_str, field.identifier());
			log_debug("\t", arg_id, "\tparameter\t// ", field.identifier());

			switch (find_marshal_kind(field.get_type())) {
			case field_marshal_kind::undefined:
				log_debug("\tMarshaling for this field is undefined");
				break;

			case field_marshal_kind::value: {
				const auto copy_dir = field.get_type().get_attributes()->get_value_copy_direction();
				if (copy_dir == copy_direction::in || copy_dir == copy_direction::both) {
					marshal_value(m_writer, arg_id);
				}

				break;
			}

			case field_marshal_kind::projection_ptr:
				m_out_ptrs.push_back(arg_id);
				if (!marshal_projection_ptr(
					m_writer,
					arg_id,
					field.get_type().get_copy_type()->get<copy_type_kind::projection>().definition()
				))
				{
					return false;
				}

				break;
			}

			return true;
		}

	private:
		marshal_op_list_writer& m_writer;
		std::vector<unsigned int>& m_out_ptrs;
	};

	class caller_unmarshal_subfields_pass : public generic_pass<caller_unmarshal_subfields_pass> {
	public:
		caller_unmarshal_subfields_pass(marshal_op_list_writer& writer, unsigned int parent) :
			m_writer {writer},
			m_parent {parent}
		{
		}

		bool visit_rpc_field(const rpc_field&)
		{
			log_warning("\tAn RPC field has been ignored during marshaling");
			log_debug("\t\tImplementation coming soon");
			return true;
		}

		bool visit_var_field(const var_field& field)
		{
			switch (find_marshal_kind(field.get_type())) {
			case field_marshal_kind::undefined:
				log_debug("\tMarshaling for this field is undefined");
				break;

			case field_marshal_kind::value: {
				const auto copy_dir = field.get_type().get_attributes()->get_value_copy_direction();
				if (copy_dir == copy_direction::out || copy_dir == copy_direction::both) {
					unmarshal_subfield(m_writer, field, m_parent);
				}

				break;
			}

			case field_marshal_kind::projection_ptr:
				const auto copy_dir = field.get_type().get_attributes()->get_value_copy_direction();
				if (!caller_unmarshal_projection_subfield(m_writer, field, m_parent)) {
					return false;
				}

				break;
			}

			return true;
		}

	private:
		marshal_op_list_writer& m_writer;
		unsigned int m_parent;
	};

	class caller_unmarshal_arguments_pass : public generic_pass<caller_unmarshal_arguments_pass> {
	public:
		caller_unmarshal_arguments_pass(marshal_op_list_writer& writer, const std::vector<unsigned int>& out_ptrs) :
			m_next_ptr {0},
			m_writer {writer},
			m_out_ptrs {out_ptrs}
		{
		}

		bool visit_rpc_field(const rpc_field&)
		{
			log_warning("\tAn RPC field has been ignored during marshaling");
			log_debug("\t\tImplementation coming soon");
			return true;
		}

		bool visit_var_field(const var_field& field)
		{
			switch (find_marshal_kind(field.get_type())) {
			case field_marshal_kind::undefined:
				log_debug("\tMarshaling for this field is undefined");
				break;

			case field_marshal_kind::projection_ptr:
				caller_unmarshal_subfields_pass pass {m_writer, m_out_ptrs[m_next_ptr++]};
				projection& def {field.get_type().get_copy_type()->get<copy_type_kind::projection>().definition()};
				if (!visit(pass, def)) {
					return false;
				}

				break;
			}

			return true;
		}

	private:
		marshal_op_list_writer& m_writer;
		const std::vector<unsigned int>& m_out_ptrs;
		unsigned int m_next_ptr;
	};
}

idlc::field_marshal_kind idlc::find_marshal_kind(const type& ty)
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

bool idlc::marshal_projection_ptr(marshal_op_list_writer& writer, unsigned int id, projection& def)
{
	log_debug("\t\tmarshal ", id, "\t// marshal pointer");
	log_debug("\t\t\t\t// NOTE: pointer was assumed not null");
	writer.add_marshal(id);
	caller_marshal_subfield_pass pass {writer, id};
	return visit(pass, def);
}

void idlc::marshal_value(marshal_op_list_writer& writer, unsigned int id)
{
	log_debug("\t\tmarshal ", id);
	writer.add_marshal(id);
}

unsigned int idlc::get_field(marshal_op_list_writer& writer, unsigned int parent, const var_field& field)
{
	const auto type_str = get_type_string(field.get_type());
	const auto field_copy_id = writer.add_get(parent, field.identifier(), type_str);
	log_debug(
		"\t", field_copy_id,
		"\tget ", parent,
		" \"", field.identifier(), "\"",
		"\t// ", field.identifier()
	);

	return field_copy_id;
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

void idlc::unmarshal_subfield(marshal_op_list_writer& writer, const var_field& field, unsigned int parent)
{
	const auto value_id = writer.add_unmarshal(get_type_string(field.get_type()));
	log_debug(
		"\t",
		value_id,
		"\tunmarshal\t// pointer ",
		field.identifier());

	log_debug("\t\tset ", parent, ' ', field.identifier(), ' ', value_id);
	writer.add_set(parent, value_id, field.identifier());
}

bool idlc::caller_unmarshal_projection_subfield(marshal_op_list_writer& writer, const var_field& field, unsigned int parent)
{
	const auto ptr_id = writer.add_unmarshal(get_type_string(field.get_type()));
	log_debug(
		"\t",
		ptr_id,
		"\tunmarshal\t// pointer ",
		field.identifier());

	log_debug("\t\tset ", parent, ' ', field.identifier(), ' ', ptr_id);
	writer.add_set(parent, ptr_id, field.identifier());

	caller_unmarshal_subfields_pass pass {writer, ptr_id};
	projection& def {field.get_type().get_copy_type()->get<copy_type_kind::projection>().definition()};
	if (!visit(pass, def)) {
		return false;
	}

	return true;
}

bool idlc::process_marshal_units(gsl::span<const marshal_unit> units)
{
	marshal_op_list_writer writer;
	for (const auto& unit : units) {
		// To recall which pointers (which are not martialed back)
		// are used for output
		// Effectively remembering the temporaries used for
		// porijection_ptr args
		std::vector<unsigned int> out_ptrs;

		log_debug("RPC marshal unit ", unit.identifier);
		if (!caller_marshal_signature(writer, *unit.sig, out_ptrs)) {
			return false;
		}

		log_debug("\t\tsend\t\t", "// send(", unit.identifier, ", buffer)");
		writer.add_send(unit.identifier);

		if (!caller_unmarshal_signature(writer, *unit.sig, out_ptrs)) {
			return false;
		}
	}

	return true;
}

bool idlc::caller_marshal_signature(marshal_op_list_writer& writer, const signature& sig, std::vector<unsigned int>& out_ptrs)
{
	for (const auto& arg : sig.arguments()) {
		caller_marshal_arguments_pass pass {writer, out_ptrs};
		if (!visit(pass, *arg))
			return false;
	}

	return true;
}

bool idlc::caller_unmarshal_signature(marshal_op_list_writer& writer, const signature& sig, std::vector<unsigned int>& out_ptrs)
{
	for (const auto& arg : sig.arguments()) {
		caller_unmarshal_arguments_pass pass {writer, out_ptrs};
		if (!visit(pass, *arg))
			return false;
	}

	return true;
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