#include "c_specifiers.h"

#include "../ast/pgraph.h"
#include "../ast/pgraph_walk.h"
#include "../frontend/analysis.h"
#include "generation.h"

namespace idlc {
	namespace {
		class c_specifier_walk : public pgraph_walk<c_specifier_walk> {
		public:
			const auto& get() const;
			bool visit_value(value& node);
			bool visit_projection(projection& node);
			bool visit_pointer(pointer& node);
			bool visit_rpc_ptr(rpc_ptr& node);
			bool visit_primitive(primitive node);
			bool visit_none(none);

		private:
			std::string m_specifier {};
		};

		// NOTE: this *does not* walk into projections, and should only be applied to
		// variable types (fields or arguments) and return types, as done in the lowering pass
		void populate_c_type_specifiers(value& node)
		{
			c_specifier_walk type_walk {};
			const auto succeeded = type_walk.visit_value(node);
			assert(succeeded);
		}
	}
}

void idlc::populate_c_type_specifiers(rpc_vec_view rpcs, projection_vec_view projections)
{
	for (const auto& node : rpcs) {
		if (node->ret_pgraph)
			populate_c_type_specifiers(*node->ret_pgraph);

		for (const auto& arg : node->arg_pgraphs)
			populate_c_type_specifiers(*arg);
	}

	for (const auto& projection : projections) {
		for (const auto& [name, field] : projection->fields)
			populate_c_type_specifiers(*field);
	}
}

const auto& idlc::c_specifier_walk::get() const { return m_specifier; }

bool idlc::c_specifier_walk::visit_value(value& node)
{
	if (!traverse(*this, node))
		return false;

	// DO NOT INCLUDE CONST HERE
	// Const-free specifiers are needed to be able to write through otherwise unwritable pointers in unmarshaling
	node.c_specifier = concat(node.is_volatile ? "volatile " : "", m_specifier);

	return true;
}

bool idlc::c_specifier_walk::visit_projection(projection& node)
{
	m_specifier = "struct ";
	m_specifier += node.real_name;
	return true;
}

bool idlc::c_specifier_walk::visit_pointer(pointer& node)
{
	if (!traverse(*this, node))
		return false;

	if (node.referent->is_const)
		m_specifier += " const";

	if (node.referent->is_volatile)
		m_specifier += " volatile";

	m_specifier += "*";

	return true;
}

bool idlc::c_specifier_walk::visit_rpc_ptr(rpc_ptr& node)
{
	m_specifier += node.definition->typedef_id;
	return true;
}

bool idlc::c_specifier_walk::visit_primitive(primitive node)
{
	switch (node) {
	case primitive::ty_bool:
		m_specifier = "bool";
		break;

	case primitive::ty_char:
		m_specifier = "char";
		break;

	case primitive::ty_schar:
		m_specifier = "signed char";
		break;

	case primitive::ty_uchar:
		m_specifier = "unsigned char";
		break;

	case primitive::ty_short:
		m_specifier = "short";
		break;

	case primitive::ty_ushort:
		m_specifier = "unsigned short";
		break;

	case primitive::ty_int:
		m_specifier = "int";
		break;

	case primitive::ty_uint:
		m_specifier = "unsigned int";
		break;

	case primitive::ty_long:
		m_specifier = "long";
		break;

	case primitive::ty_ulong:
		m_specifier = "unsigned long";
		break;

	case primitive::ty_llong:
		m_specifier = "long long";
		break;

	case primitive::ty_ullong:
		m_specifier = "unsigned long long";
		break;

	default:
		std::cout << "Debug: Unhandled primitive was " << static_cast<std::uintptr_t>(node) << "\n";
		std::terminate();
	}

	return true;
}

bool idlc::c_specifier_walk::visit_none(none)
{
	m_specifier = "void";
	return true;
}
