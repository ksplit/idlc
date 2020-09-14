#ifndef _VISIT_H_
#define _VISIT_H_

#include "../parser/ast.h"

// TODO: write support for pre- and post- actions on nodes for traversal

namespace idlc {
	template<typename pass_type>
	bool visit(pass_type& pass, file& file)
	{
		if (!pass(file)) {
			return false;
		}

		for (const auto& item : file.items()) {
			if (!visit(pass, *item)) {
				return false;
			}
		}

		return true;
	}

	template<typename pass_type>
	bool visit(pass_type& pass, file_item& item)
	{
		switch (item.kind()) {
		case file_item_kind::include:
			return visit(pass, item.get<file_item_kind::include>());

		case file_item_kind::module:
			return visit(pass, item.get<file_item_kind::module>());

		default:
			Expects(false);
		}
	}

	template<typename pass_type>
	bool visit(pass_type& pass, include& include)
	{
		return pass(include);
	}

	template<typename pass_type>
	bool visit(pass_type& pass, module& module)
	{
		if (!pass(module)) {
			return false;
		}

		for (const auto& item : module.items()) {
			if (!visit(pass, *item)) {
				return false;
			}
		}

		return true;
	}

	// Is there really a use case for this one?
	// What we're really interested in is the content of the variant, after all

	template<typename pass_type>
	bool visit(pass_type& pass, module_item& item)
	{
		switch (item.kind()) {
		case module_item_kind::projection:
			return visit(pass, item.get<module_item_kind::projection>());

		case module_item_kind::rpc:
			return visit(pass, item.get<module_item_kind::rpc>());

		case module_item_kind::require:
			return visit(pass, item.get<module_item_kind::require>());

		case module_item_kind::header_import:
			return visit(pass, item.get<module_item_kind::header_import>());

		default:
			Expects(false);
		}
	}

	template<typename pass_type>
	bool visit(pass_type& pass, header_import& header_import)
	{
		return pass(header_import);
	}

	template<typename pass_type>
	bool visit(pass_type& pass, projection& projection)
	{
		if (!pass(projection)) {
			return false;
		}

		for (const auto& field : projection.fields()) {
			if (!visit(pass, *field)) {
				return false;
			}
		}

		return true;
	}

	template<typename pass_type>
	bool visit(pass_type& pass, field& field)
	{
		switch (field.kind()) {
		case field_kind::var:
			return visit(pass, field.get<field_kind::var>());

		case field_kind::rpc:
			return visit(pass, field.get<field_kind::rpc>());

		default:
			Expects(false);
		}
	}

	template<typename pass_type>
	bool visit(pass_type& pass, var_field& field)
	{
		if (!pass(field)) {
			return false;
		}

		return visit(pass, field.get_type());
	}

	template<typename pass_type>
	bool visit(pass_type& pass, rpc_field& field)
	{
		if (!pass(field)) {
			return false;
		}

		if (!visit(pass, field.get_signature())) {
			return false;
		}

		return true;
	}

	template<typename pass_type>
	bool visit(pass_type& pass, type& type)
	{
		if (!pass(type)) {
			return false;
		}

		if (type.get_copy_type()) {
			if (!visit(pass, *type.get_copy_type())) {
				return false;
			}
		}

		if (type.get_attributes()) {
			if (!visit(pass, *type.get_attributes())) {
				return false;
			}
		}

		return true;
	}

	template<typename pass_type>
	bool visit(pass_type& pass, copy_type& type)
	{
		switch (type.kind()) {
		case copy_type_kind::projection:
			return visit(pass, type.get<copy_type_kind::projection>());

		case copy_type_kind::primitive:
			return visit(pass, type.get<copy_type_kind::primitive>());

		default:
			Expects(false);
		}
	}

	template<typename pass_type>
	bool visit(pass_type& pass, projection_type& type)
	{
		return pass(type);
	}

	template<typename pass_type>
	bool visit(pass_type& pass, primitive_type& type)
	{
		return pass(type);
	}

	template<typename pass_type>
	bool visit(pass_type& pass, rpc& rpc)
	{
		if (!pass(rpc)) {
			return false;
		}

		if (!visit(pass, rpc.get_signature())) {
			return false;
		}

		return true;
	}

	template<typename pass_type>
	bool visit(pass_type& pass, require& require)
	{
		return pass(require);
	}

	template<typename pass_type>
	bool visit(pass_type& pass, signature& signature)
	{
		if (!pass(signature)) {
			return false;
		}

		if (!visit(pass, signature.return_field())) {
			return false;
		}

		for (const auto& arg : signature.arguments()) {
			if (!visit(pass, *arg)) {
				return false;
			}
		}

		return true;
	}

	template<typename pass_type>
	bool visit(pass_type& pass, attributes& attribs)
	{
		return pass(attribs);
	}
}

#endif