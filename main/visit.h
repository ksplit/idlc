#ifndef _VISIT_H_
#define _VISIT_H_

#include "../parser/ast.h"

namespace idlc {
	template<typename pass_type>
	void visit(pass_type& pass, file& file)
	{
		pass(file);
		for (const auto& item : file.items()) {
			visit(pass, *item);
		}
	}

	template<typename pass_type>
	void visit(pass_type& pass, file_item& item)
	{
		switch (item.kind()) {
		case file_item_kind::include:
			visit(pass, item.get<file_item_kind::include>());
			break;

		case file_item_kind::module:
			visit(pass, item.get<file_item_kind::module>());
			break;
		}
	}

	template<typename pass_type>
	void visit(pass_type& pass, include& include)
	{
		pass(include);
	}

	template<typename pass_type>
	void visit(pass_type& pass, module& module)
	{
		pass(module);
		for (const auto& item : module.items()) {
			visit(pass, *item);
		}
	}

	// Is there really a use case for this one?
	// What we're really interested in is the content of the variant, after all

	template<typename pass_type>
	void visit(pass_type& pass, module_item& item)
	{
		switch (item.kind()) {
		case module_item_kind::projection:
			visit(pass, item.get<module_item_kind::projection>());
			break;

		case module_item_kind::rpc:
			visit(pass, item.get<module_item_kind::rpc>());
			break;

		case module_item_kind::require:
			visit(pass, item.get<module_item_kind::require>());
			break;
		}
	}

	template<typename pass_type>
	void visit(pass_type& pass, projection& projection)
	{
		pass(projection);
		for (const auto& field : projection.fields()) {
			visit(pass, *field);
		}
	}

	template<typename pass_type>
	void visit(pass_type& pass, field& field)
	{
		switch (field.kind()) {
		case field_kind::var:
			visit(pass, field.get<field_kind::var>());
			break;

		case field_kind::rpc:
			visit(pass, field.get<field_kind::rpc>());
			break;
		}
	}

	template<typename pass_type>
	void visit(pass_type& pass, var_field& field)
	{
		pass(field);
		visit(pass, field.get_type());
	}

	template<typename pass_type>
	void visit(pass_type& pass, rpc_field& field)
	{
		pass(field);
		visit(pass, field.get_signature());
	}

	template<typename pass_type>
	void visit(pass_type& pass, type& type)
	{
		pass(type);
		if (type.get_copy_type()) {
			visit(pass, *type.get_copy_type());
		}

		if (type.get_attributes()) {
			visit(pass, *type.get_attributes());
		}
	}

	template<typename pass_type>
	void visit(pass_type& pass, copy_type& type)
	{
		switch (type.kind()) {
		case copy_type_kind::projection:
			visit(pass, type.get<copy_type_kind::projection>());
			break;

		case copy_type_kind::primitive:
			visit(pass, type.get<copy_type_kind::primitive>());
			break;
		}
	}

	template<typename pass_type>
	void visit(pass_type& pass, projection_type& type)
	{
		pass(type);
	}

	template<typename pass_type>
	void visit(pass_type& pass, primitive_type& type)
	{
		pass(type);
	}

	template<typename pass_type>
	void visit(pass_type& pass, rpc& rpc)
	{
		pass(rpc);
		visit(pass, rpc.get_signature());
	}

	template<typename pass_type>
	void visit(pass_type& pass, require& require)
	{
		pass(require);
	}

	template<typename pass_type>
	void visit(pass_type& pass, signature& signature)
	{
		pass(signature);
		visit(pass, signature.return_field());
		for (const auto& arg : signature.arguments()) {
			visit(pass, *arg);
		}
	}

	template<typename pass_type>
	void visit(pass_type& pass, attributes& attribs)
	{
		pass(attribs);
	}
}

#endif