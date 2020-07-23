#ifndef _MARSHALING_H_
#define _MARSHALING_H_

#include "../parser/ast.h"

namespace idlc {
	struct marshal_unit {
		const signature* rpc_signature;
		gsl::czstring<> identifier;
	};

	enum class marshal_op_kind {
		marshal,
		unmarshal,
		marshal_field,
		unmarshal_field,
		create_shadow,
		destroy_shadow,
		find_shadow_id,
		find_shadow,
		return_to_caller,
		load_field_indirect,
		store_field_indirect,
		call_direct,
		call_indirect,
		send_rpc,
		block_if_not_null,
		end_block,
		inject_trampoline
	};

	struct marshal {
		std::string name;
	};

	struct unmarshal {
		std::string type;
		std::string name;
	};

	struct marshal_field {
		std::string parent;
		std::string field;
	};

	struct unmarshal_field {
		std::string type;
		std::string parent;
		std::string field;
	};

	struct create_shadow {
	};

	struct destroy_shadow {
	};

	struct find_shadow_id {
	};

	struct find_shadow {
	};

	struct return_to_caller {
		std::string name;
	};

	struct load_field_indirect {
		std::string type;
		std::string name;
		std::string parent;
		std::string field;
	};

	struct store_field_indirect {
		std::string parent;
		std::string field;
		std::string name;
	};

	struct call_direct {
		std::string type;
		std::string function;
		std::string arguments_list;
	};

	struct call_indirect {
		std::string type;
		std::string function_type;
		std::string pointer;
		std::string arguments_list;
	};

	struct send_rpc {
		std::string rpc;
	};

	struct block_if_not_null {
		std::string pointer;
	};

	struct end_block {
	};

	struct inject_trampoline {
		std::string declaration;
		std::string pointer;
	};

	using marshal_op = std::variant<
		marshal,
		unmarshal,
		marshal_field,
		unmarshal_field,
		create_shadow,
		destroy_shadow,
		find_shadow_id,
		find_shadow,
		return_to_caller,
		load_field_indirect,
		store_field_indirect,
		call_direct,
		call_indirect,
		send_rpc,
		block_if_not_null,
		end_block,
		inject_trampoline
	>;

	struct marshal_unit_lists {
		gsl::czstring<> identifier;
		std::vector<marshal_op> caller_ops;
		std::vector<marshal_op> callee_ops;
	};

	enum class marshal_unit_kind {
		direct,
		indirect
	};

	bool process_marshal_units(gsl::span<const marshal_unit> units, marshal_unit_kind kind, std::vector<marshal_unit_lists>& unit_marshaling);
}

#endif // !_MARSHALING_H_
