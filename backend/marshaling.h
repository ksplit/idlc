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
		get_remote,
		get_local,
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

	/*
		TODO: Considering all the subtly different kinds of strings in here,
		it may be useful to strongly type them
	*/

	struct marshal {
		std::string name;
	};

	struct unmarshal {
		std::string declaration;
		std::string type;
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

	// These ops always take remote pointers as opaque IDs
	// <void*>

	// Conceptually: <declaration> = fipc_create_shadow(<shadow-id>);
	struct create_shadow {
		std::string declaration;
		std::string remote_pointer;
	};

	// fipc_destroy_shadow(<remote-pointer>);
	struct destroy_shadow {
		std::string remote_pointer;
	};

	// <remote_declaration> = fipc_get_remote(<local-pointer>);
	struct get_remote {
		std::string remote_declaration;
		std::string local_pointer;
	};

	// <local_declaration> = fipc_get_local(<remote-pointer>);
	struct get_local {
		std::string local_declaration;
		std::string remote_pointer;
	};

	struct return_to_caller {
		std::string name;
	};

	// TODO: Switch this to the decl strings
	struct load_field_indirect {
		std::string declaration;
		std::string parent;
		std::string field;
	};

	struct store_field_indirect {
		std::string parent;
		std::string field;
		std::string name;
	};

	struct call_direct {
		std::string declaration;
		std::string function;
		std::string arguments_list;
	};

	struct call_indirect {
		std::string declaration;
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
		std::string mangled_name;
		std::string pointer;
	};

	using marshal_op = std::variant<
		marshal,
		unmarshal,
		marshal_field,
		unmarshal_field,
		create_shadow,
		destroy_shadow,
		get_remote,
		get_local,
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
		std::string header;
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
