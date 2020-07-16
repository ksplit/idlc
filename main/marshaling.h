#ifndef _MARSHALING_H_
#define _MARSHALING_H_

#include "../parser/ast.h"

namespace idlc {
	struct marshal_unit {
		const signature* sig;
		std::string identifier;
	};

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
		send			//								=> send(<$rpc-id>, buffer);
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
			std::vector<send_data>&& send_data
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
			m_send {0}
		{
		}

		marshal_op get_next_op()
		{
			return m_ops[++m_op];
		}

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
	};

	class marshal_op_list_writer {
	public:
		void add_marshal(unsigned int source)
		{
			m_ops.push_back(marshal_op::marshal);
			m_marshal_data.push_back({source});
		}

		unsigned int add_unmarshal(const std::string& type_str)
		{
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
				std::move(m_send_data)
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
	};

	bool process_marshal_units(gsl::span<const marshal_unit> units);
}

#endif // !_MARSHALING_H_
