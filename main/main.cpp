
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <fstream>

#include "type_map.h"
#include "dump.h"
#include "../parser/parser.h"

namespace idlc
{
	// RPCs and RPC pointers, though different nodes, each have a signature, used to generate their marshaling info

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
			// TODO
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
	void visit(pass_type& pass, type& type)
	{
		pass(type);
		if (type.get_copy_type()) {
			visit(pass, *type.get_copy_type());
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
			// TODO
			break;
		}
	}

	template<typename pass_type>
	void visit(pass_type& pass, projection_type& type)
	{
		pass(type);
	}

	template<typename pass_type>
	void visit(pass_type& pass, rpc& rpc)
	{
		pass(rpc);
	}

	template<typename pass_type>
	void visit(pass_type& pass, require& require)
	{
		pass(require);
	}

	class type_collection_pass {
	public:
		void operator()(const file& file) {}
		void operator()(const include& include) {}
		void operator()(const rpc& rpc) {}
		void operator()(const require& require) {}
		void operator()(const var_field& field) {}
		void operator()(const projection_type& proj) {}
		void operator()(const type& ty) {}

		void operator()(module& module)
		{
			m_types = &module.types;
		}

		void operator()(const projection& projection)
		{
			if (!m_types->insert(projection)) {
				std::cout << "Encountered projection redefinition: " << projection.identifier() << "\n";
				throw std::exception {};
			}
		}

	private:
		type_map* m_types;
	};

	// TODO: would be nice to have an error context
	// NOTE: I don't think exceptions are terribly appropriate for this

	class type_resolve_pass {
	public:
		void operator()(const file& file) {}
		void operator()(const include& include) {}
		void operator()(const rpc& rpc) {}
		void operator()(const require& require) {}
		void operator()(const projection& projection) {}
		void operator()(const var_field& field) {}
		void operator()(const type& ty) {}

		void operator()(module& module)
		{
			m_types = &module.types;
		}

		void operator()(projection_type& proj)
		{
			const projection* def {m_types->get(proj.identifier())};
			if (def) {
				proj.definition(def);
			}
			else {
				std::cout << "Could not resolve projection: " << proj.identifier() << "\n";
			}
		}

	private:
		type_map* m_types;
	};
}


int main(int argc, gsl::czstring<>* argv) {
	gsl::span args {argv, gsl::narrow_cast<std::size_t>(argc)};

	if (args.size() != 2 && args.size() != 3) {
		std::cout << "Usage: idlc <source-file> [<dump-file>]\n";
	}

	try {
		auto top_node = std::unique_ptr<idlc::file> {(idlc::file*)Parser::parse(std::string {args[1]})};
		auto& file = *top_node;

		std::cout << "IDL file OK\n";

		if (args.size() == 3) {
			std::ofstream dump {args[2]};
			std::cout << "Doing AST dump...\n";
			idlc::dump(*top_node, dump);
		}

		std::cout << "Collecting types\n";
		idlc::type_collection_pass tc_pass;
		visit(tc_pass, file);

		std::cout << "Resolving types\n";
		idlc::type_resolve_pass tr_pass;
		visit(tr_pass, file);

		return 0;
	}
	catch (const Parser::ParseException& e) {
		std::cerr << "\n\nALERT!!! - Caught parser exception" << std::endl;
		std::cerr << e.getReason() << std::endl;
		return 1;
	}
	catch (const std::exception&) {
		std::cout << "Compilation failed\n";
		return 1;
	}
}
