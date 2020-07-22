#ifndef _MARSHALING_H_
#define _MARSHALING_H_

#include "../parser/ast.h"

namespace idlc {
	struct marshal_unit {
		const signature* rpc_signature;
		gsl::czstring<> identifier;
	};

	enum class marshal_op {
		// <$*> indicates a template field that is inferred somehow
		// <$id>, for instance, is monotonically increasing for each
		// marshal_op with a result
		// <$type> is inferred from type info
		// <$child-field> from the projection definition
		// etc.

		//					arguments						generated code

		marshal,			// <source>						=> buffer[++slot] = var_<source>;
		unmarshal,			//								=> <$type> var_<$id> = buffer[++slot];
		create_cspace,		// <pointer>					=> <$type>* var_<$id> = create_cspace(<$type>, var_<pointer>);
		destroy_cspace,		// <pointer>					=> destroy_cspace(var_<pointer>);
		find_cspace,		// <pointer>					=> find_cspace(var_<pointer>);
		get_cspace,			// <pointer>					=> <$type>* var_<$id> = get_cspace(var_<pointer>);
		parameter,			// <source>						=> <$type> <$parameter> = var_<source>;
		argument,			//								=> <$type> var_<$id> = <$argument>;
		get_return_value,	//								=> <$type> var_<$id> = var_ret_val;
		return_var,			// <source>						=> return var_<source>;
		get,				// <parent> <child>				=> <$type> var_<$id> = var_<parent>-><$child-field>;
		set,				// <parent> <child> <source>	=> var_<parent>-><$child-field> = var_<source>;
		call,				//								=> [<$type> var_ret_val =] <real-function>(<$arguments>); slot = 0;
		call_indirect,		// <source>						=> [<$type> var_ret_val =] ((<$type>)var_<source>)(<$arguments>); slot = 0;
		send,				//								=> send(<$rpc-id>, buffer);
		if_not_null,		// <pointer>					=> if (var_<pointer>) {
		end_if_not_null,	//								=> }
		inject_trampoline	// <fptr>						=> void* var_<$id> = inject_trampoline(var_<fptr>, <$rpc-id>);
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

	struct find_cspace_data {
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

	struct get_return_value_data {
		std::string type;
	};

	struct return_var_data {
		unsigned int source;
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
		std::string return_type;
		std::string arguments;
	};

	struct call_indirect_data {
		unsigned int source;
		std::string function_type;
		std::string return_type;
		std::string arguments;
	};

	struct send_data {
		std::string rpc_id;
	};

	struct if_not_null_data {
		unsigned int pointer;
	};

	struct inject_trampoline_data {
		unsigned int fptr;
		gsl::czstring<> rpc_id;
	};

	class marshal_op_list {
	public:
		marshal_op_list(
			std::vector<marshal_op>&& ops,
			std::vector<marshal_data>&& marshal_data,
			std::vector<unmarshal_data>&& unmarshal_data,
			std::vector<create_cspace_data>&& create_cspace_data,
			std::vector<destroy_cspace_data>&& destroy_cspace_data,
			std::vector<find_cspace_data>&& find_cspace_data,
			std::vector<get_cspace_data>&& get_cspace_data,
			std::vector<argument_data>&& argument_data,
			std::vector<parameter_data>&& parameter_data,
			std::vector<get_return_value_data>&& get_return_value_data,
			std::vector<return_var_data>&& return_var_data,
			std::vector<get_data>&& get_data,
			std::vector<set_data>&& set_data,
			std::vector<call_data>&& call_data,
			std::vector<call_indirect_data>&& call_indirect_data,
			std::vector<send_data>&& send_data,
			std::vector<if_not_null_data>&& if_not_null_data,
			std::vector<inject_trampoline_data>&& inject_trampoline_data
		) :
			m_ops {std::move(ops)},
			m_marshal_data {std::move(marshal_data)},
			m_unmarshal_data {std::move(unmarshal_data)},
			m_create_cspace_data {std::move(create_cspace_data)},
			m_destroy_cspace_data {std::move(destroy_cspace_data)},
			m_find_cspace_data {std::move(find_cspace_data)},
			m_get_cspace_data {std::move(get_cspace_data)},
			m_argument_data {std::move(argument_data)},
			m_parameter_data {std::move(parameter_data)},
			m_get_return_value_data {std::move(get_return_value_data)},
			m_return_var_data {std::move(return_var_data)},
			m_get_data {std::move(get_data)},
			m_set_data {std::move(set_data)},
			m_call_data {std::move(call_data)},
			m_call_indirect_data {std::move(call_indirect_data)},
			m_send_data {std::move(send_data)},
			m_if_not_null_data {std::move(if_not_null_data)},
			m_inject_trampoline_data {std::move(inject_trampoline_data)},
			m_op {0},
			m_marshal {0},
			m_unmarshal {0},
			m_create_cspace {0},
			m_destroy_cspace {0},
			m_get_cspace {0},
			m_argument {0},
			m_parameter {0},
			m_get_return_value {0},
			m_return_var {0},
			m_get {0},
			m_set {0},
			m_call {0},
			m_call_indirect {0},
			m_send {0},
			m_if_not_null {0},
			m_inject_trampoline {0}
		{
		}

		bool finished()
		{
			return m_ops.size() == m_op;
		}

		marshal_op get_next_op()
		{
			return m_ops[m_op++];
		}

		unmarshal_data& get_next_unmarshal()
		{
			return m_unmarshal_data[m_unmarshal++];
		}

		marshal_data& get_next_marshal()
		{
			return m_marshal_data[m_marshal++];
		}

		parameter_data& get_next_parameter()
		{
			return m_parameter_data[m_parameter++];
		}

		if_not_null_data& get_next_if_not_null()
		{
			return m_if_not_null_data[m_if_not_null++];
		}

		set_data& get_next_set()
		{
			return m_set_data[m_set++];
		}

		get_data& get_next_get()
		{
			return m_get_data[m_get++];
		}

		call_data& get_next_call()
		{
			return m_call_data[m_call++];
		}

		inject_trampoline_data& get_next_inject_trampoline()
		{
			return m_inject_trampoline_data[m_inject_trampoline++];
		}

		get_return_value_data& get_next_get_return_value()
		{
			return m_get_return_value_data[m_get_return_value++];
		}

		// TODO: getters for the proj_field data

	private:
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

		unsigned int m_op;
		unsigned int m_marshal;
		unsigned int m_unmarshal;
		unsigned int m_create_cspace;
		unsigned int m_destroy_cspace;
		unsigned int m_find_cspace;
		unsigned int m_get_cspace;
		unsigned int m_inject_trampoline;
		unsigned int m_argument;
		unsigned int m_parameter;
		unsigned int m_get_return_value;
		unsigned int m_return_var;
		unsigned int m_get;
		unsigned int m_set;
		unsigned int m_call;
		unsigned int m_call_indirect;
		unsigned int m_send;
		unsigned int m_if_not_null;
	};

	struct marshal_unit_lists {
		gsl::czstring<> identifier;
		marshal_op_list caller_ops;
		marshal_op_list callee_ops;
	};

	enum class marshal_unit_kind {
		direct,
		indirect
	};

	bool process_marshal_units(gsl::span<const marshal_unit> units, marshal_unit_kind kind, std::vector<marshal_unit_lists>& unit_marshaling);
}

#endif // !_MARSHALING_H_
