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
	field_marshal_kind get_var_marshal_kind(const type& ty);

	enum class marshal_op {
		// <$*> indicates a template field that is inferred somehow
		// <$id>, for instance, is monotonically increasing for each
		// marshal_op with a result
		// <$type> is inferred from type info
		// <$child-field> from the projection definition
		// etc.

		//				arguments						generated code

		marshal,		// <source>						=> buffer[++slot] = var_<source>;
		unmarshal,		//								=> <$type> var_<$id> = buffer[++slot];
		create_cspace,	// <pointer>					=> <$type>* var_<$id> = create_cspace(<$type>, var_<pointer>);
		destroy_cspace,	// <pointer>					=> destroy_cspace(var_<pointer>);
		get_cspace,		// <pointer>					=> <$type>* var_<$id> = get_cspace(var_<pointer>);
		parameter,		// <source>						=> <$type> <$parameter> = var_<source>;
		argument,		//								=> <$type> var_<$id> = <$argument>;
		return_value,	//								=> <$type> var_<$id> = var_ret_val;
		get,			// <parent> <child>				=> <$type> var_<$id> = var_<parent>-><$child-field>;
		set,			// <parent> <child> <source>	=> var_<parent>-><$child-field> = var_<source>;
		call,			//								=> [<$type> var_<$id> =] <real-function>(<$arguments>); slot = 0;
		send,			//								=> send(<$rpc-id>, buffer);
		if_not_null,	// <pointer>					=> if (var_<pointer>) {
		end_if_not_null //								=> }
	};

	struct marshal_data {
		unsigned int source;
	};

	struct unmarshal_data {
		std::string type;
	};

	struct create_cspace_data {
		unsigned int pointer;
		std::string type;
	};

	struct destroy_cspace_data {
		unsigned int pointer;
	};

	struct get_cspace_data {
		unsigned int pointer;
		std::string type;
	};

	struct argument_data {
		std::string type;
		std::string argument;
	};

	struct parameter_data {
		unsigned int source;
		std::string type;
		std::string argument;
	};

	struct return_value_data {
		std::string type;
	};

	struct get_data {
		unsigned int parent;
		std::string child_field;
		std::string type;
	};

	struct set_data {
		unsigned int parent;
		unsigned int source;
		std::string child_field;
	};

	struct call_data {
		std::string arguments;
	};

	struct send_data {
		std::string rpc_id;
	};

	struct if_not_null_data {
		unsigned int pointer;
	};

	class marshal_op_list {
	public:
		marshal_op_list(
			std::vector<marshal_op>&& ops,
			std::vector<marshal_data>&& marshal_data,
			std::vector<unmarshal_data>&& unmarshal_data,
			std::vector<create_cspace_data>&& create_cspace_data,
			std::vector<destroy_cspace_data>&& destroy_cspace_data,
			std::vector<get_cspace_data>&& get_cspace_data,
			std::vector<argument_data>&& argument_data,
			std::vector<parameter_data>&& parameter_data,
			std::vector<return_value_data>&& return_value_data,
			std::vector<get_data>&& get_data,
			std::vector<set_data>&& set_data,
			std::vector<call_data>&& call_data,
			std::vector<send_data>&& send_data,
			std::vector<if_not_null_data>&& if_not_null_data
		) :
			m_ops {std::move(ops)},
			m_marshal_data {std::move(marshal_data)},
			m_unmarshal_data {std::move(unmarshal_data)},
			m_create_cspace_data {std::move(create_cspace_data)},
			m_destroy_cspace_data {std::move(destroy_cspace_data)},
			m_get_cspace_data {std::move(get_cspace_data)},
			m_argument_data {std::move(argument_data)},
			m_parameter_data {std::move(parameter_data)},
			m_return_value_data {std::move(return_value_data)},
			m_get_data {std::move(get_data)},
			m_set_data {std::move(set_data)},
			m_call_data {std::move(call_data)},
			m_send_data {std::move(send_data)},
			m_if_not_null_data {std::move(if_not_null_data)},
			m_op {0},
			m_marshal {0},
			m_unmarshal {0},
			m_create_cspace {0},
			m_destroy_cspace {0},
			m_get_cspace {0},
			m_argument {0},
			m_parameter {0},
			m_return_value {0},
			m_get {0},
			m_set {0},
			m_call {0},
			m_send {0},
			m_if_not_null {0}
		{
		}

		marshal_op get_next_op()
		{
			return m_ops[++m_op];
		}

		// TODO: getters for the argument data

	private:
		std::vector<marshal_op> m_ops;
		std::vector<marshal_data> m_marshal_data;
		std::vector<unmarshal_data> m_unmarshal_data;
		std::vector<create_cspace_data> m_create_cspace_data;
		std::vector<destroy_cspace_data> m_destroy_cspace_data;
		std::vector<get_cspace_data> m_get_cspace_data;
		std::vector<argument_data> m_argument_data;
		std::vector<parameter_data> m_parameter_data;
		std::vector<return_value_data> m_return_value_data;
		std::vector<get_data> m_get_data;
		std::vector<set_data> m_set_data;
		std::vector<call_data> m_call_data;
		std::vector<send_data> m_send_data;
		std::vector<if_not_null_data> m_if_not_null_data;

		unsigned int m_op;
		unsigned int m_marshal;
		unsigned int m_unmarshal;
		unsigned int m_create_cspace;
		unsigned int m_destroy_cspace;
		unsigned int m_get_cspace;
		unsigned int m_argument;
		unsigned int m_parameter;
		unsigned int m_return_value;
		unsigned int m_get;
		unsigned int m_set;
		unsigned int m_call;
		unsigned int m_send;
		unsigned int m_if_not_null;
	};

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
			log_debug("\t", m_next_var_id + 1, "\tunmarshal");
			m_ops.push_back(marshal_op::unmarshal);
			m_unmarshal_data.push_back({type_str});
			return m_next_var_id++;
		}

		unsigned int add_create_cspace(unsigned int pointer, const std::string& type_str)
		{
			m_ops.push_back(marshal_op::create_cspace);
			m_create_cspace_data.push_back({pointer, type_str});
			return m_next_var_id++;
		}

		void add_destroy_cspace(unsigned int pointer)
		{
			m_ops.push_back(marshal_op::destroy_cspace);
			m_destroy_cspace_data.push_back({pointer});
		}

		unsigned int add_get_cspace(unsigned int source, const std::string& type_str)
		{
			m_ops.push_back(marshal_op::get_cspace);
			m_get_cspace_data.push_back({source, type_str});
			return m_next_var_id++;
		}


		void add_parameter(unsigned int source, const std::string& type_str, const std::string& arg_str)
		{
			m_ops.push_back(marshal_op::parameter);
			m_parameter_data.push_back({source, type_str, arg_str});
		}

		unsigned int add_argument(const std::string& type_str, const std::string& param_str)
		{
			log_debug("\t", m_next_var_id + 1, "\targument ", param_str);
			m_ops.push_back(marshal_op::argument);
			m_argument_data.push_back({type_str, param_str});
			return m_next_var_id++;
		}

		unsigned int add_return_value(const std::string& type_str)
		{
			m_ops.push_back(marshal_op::return_value);
			m_return_value_data.push_back({type_str});
			return m_next_var_id++;
		}


		unsigned int add_get(unsigned int parent, const std::string& child_field_str, const std::string& type_str)
		{
			m_ops.push_back(marshal_op::get);
			m_get_data.push_back({parent, child_field_str, type_str});
			return m_next_var_id++;
		}


		void add_set(unsigned int parent, unsigned int source, const std::string& child_field_str)
		{
			m_ops.push_back(marshal_op::set);
			m_set_data.push_back({parent, source, child_field_str});
		}

		unsigned int add_call(const std::string& arguments_str)
		{
			m_ops.push_back(marshal_op::call);
			m_call_data.push_back({arguments_str});
			return m_next_var_id++;
		}

		void add_send(const std::string& rpc_id_str)
		{
			m_ops.push_back(marshal_op::send);
			m_send_data.push_back({rpc_id_str});
		}

		void add_if_not_null(unsigned int pointer)
		{
			m_ops.push_back(marshal_op::if_not_null);
			m_if_not_null_data.push_back({pointer});
		}

		void add_end_if_not_null()
		{
			m_ops.push_back(marshal_op::end_if_not_null);
		}

		marshal_op_list move_to_list()
		{
			return {
				std::move(m_ops),
				std::move(m_marshal_data),
				std::move(m_unmarshal_data),
				std::move(m_create_cspace_data),
				std::move(m_destroy_cspace_data),
				std::move(m_get_cspace_data),
				std::move(m_argument_data),
				std::move(m_parameter_data),
				std::move(m_return_value_data),
				std::move(m_get_data),
				std::move(m_set_data),
				std::move(m_call_data),
				std::move(m_send_data),
				std::move(m_if_not_null_data)
			};
		}

	private:
		unsigned int m_next_var_id {0};
		std::vector<marshal_op> m_ops;
		std::vector<marshal_data> m_marshal_data;
		std::vector<unmarshal_data> m_unmarshal_data;
		std::vector<create_cspace_data> m_create_cspace_data;
		std::vector<destroy_cspace_data> m_destroy_cspace_data;
		std::vector<get_cspace_data> m_get_cspace_data;
		std::vector<argument_data> m_argument_data;
		std::vector<parameter_data> m_parameter_data;
		std::vector<return_value_data> m_return_value_data;
		std::vector<get_data> m_get_data;
		std::vector<set_data> m_set_data;
		std::vector<call_data> m_call_data;
		std::vector<send_data> m_send_data;
		std::vector<if_not_null_data> m_if_not_null_data;
	};

	bool caller_marshal_argument(marshal_op_writer& marshaling, const field& argument);
	bool caller_marshal_rpc(marshal_op_writer& marshaling, const rpc_field& rpc);

	bool callee_unmarshal_argument(marshal_op_writer& marshaling, const field& argument);
	bool callee_unmarshal_rpc(marshal_op_writer& marshaling, const rpc_field& rpc);
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

bool idlc::process_marshal_units(gsl::span<const marshal_unit> units)
{
	marshal_op_writer caller_marshaling;
	marshal_op_writer callee_marshaling;

	for (const marshal_unit& unit : units) {
		log_debug(unit.identifier, ":");
		const signature& signature {*unit.signature};
		
		log_debug("Caller-side:");

		// Caller marshaling
		for (const std::unique_ptr<field>& field : signature.arguments()) {
			caller_marshal_argument(caller_marshaling, *field);
		}

		log_debug("Callee-side:");

		// Callee marshaling
		for (const std::unique_ptr<field>& field : signature.arguments()) {
			callee_unmarshal_argument(callee_marshaling, *field);
		}
	}

	return true;
}

bool idlc::caller_marshal_rpc(marshal_op_writer& marshaling, const rpc_field& rpc)
{
	// NOTE: I think we can get away with the void* trick due to C's looser type system
	const unsigned int save_id {marshaling.add_argument("void*", rpc.identifier())};
	marshaling.add_marshal(save_id);
	log_warning("RPC pointers not implemented yet, marshaling raw pointer value instead");
	return true;
}

bool idlc::caller_marshal_argument(marshal_op_writer& marshaling, const field& argument)
{
	switch (argument.kind()) {
	case field_kind::rpc:
		caller_marshal_rpc(marshaling, argument.get<field_kind::rpc>());
		break;

	case field_kind::var:
		const var_field& var {argument.get<field_kind::var>()};

		const type& type {var.get_type()};
		const unsigned save_id {marshaling.add_argument(get_type_string(type), var.identifier())};
		marshaling.add_marshal(save_id);

		const field_marshal_kind marshal_kind {get_var_marshal_kind(type)};
		switch (marshal_kind) {
		case field_marshal_kind::undefined:
			log_error("\t", var.identifier(), " has undefined marshaling");
			return false;

		case field_marshal_kind::projection_ptr:
			// Recurse into projection fields
			// Needs save ID of the projection pointer and the projection definition

			break;
		}
	}

	return true;
}


bool idlc::callee_unmarshal_rpc(marshal_op_writer& marshaling, const rpc_field& rpc)
{
	// NOTE: I think we can get away with the void* trick due to C's looser type system
	const unsigned int save_id {marshaling.add_unmarshal("void*")};
	log_warning("RPC pointers not implemented yet, unmarshaling raw pointer value instead");
	return true;
}

bool idlc::callee_unmarshal_argument(marshal_op_writer& marshaling, const field& argument)
{
	switch (argument.kind()) {
	case field_kind::rpc:
		callee_unmarshal_rpc(marshaling, argument.get<field_kind::rpc>());
		break;

	case field_kind::var:
		const var_field& var {argument.get<field_kind::var>()};

		const type& type {var.get_type()};
		const unsigned int save_id {marshaling.add_unmarshal(get_type_string(type))};
		const field_marshal_kind marshal_kind {get_var_marshal_kind(type)};

		switch (marshal_kind) {
		case field_marshal_kind::undefined:
			log_error("\t", var.identifier(), " has undefined marshaling");
			return false;

		case field_marshal_kind::projection_ptr:
			// Recurse into projection fields
			// Needs save ID of the projection pointer and the projection definition
			break;
		}
	}

	return true;
}
