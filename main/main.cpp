
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <fstream>

#include "dump.h"
#include "../parser/parser.h"
#include "../parser/ast.h"

namespace idlc
{
	// RPCs and RPC pointers, though different nodes, each have a signature, used to generate their marshaling info

	template<typename pass_type>
	void visit(pass_type& pass, const file& file)
	{
		pass(file);
		for (const auto& item : file.items()) {
			visit(pass, *item);
		}
	}

	template<typename pass_type>
	void visit(pass_type& pass, const file_item& item)
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
	void visit(pass_type& pass, const include& include)
	{
		pass(include);
	}

	template<typename pass_type>
	void visit(pass_type& pass, const module& module)
	{
		pass(module);
		for (const auto& item : module.items()) {
			visit(pass, *item);
		}
	}
	
	// Is there really a use case for this one?
	// What we're really interested in is the content of the variant, after all

	template<typename pass_type>
	void visit(pass_type& pass, const module_item& item)
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
	void visit(pass_type& pass, const projection& projection)
	{
		pass(projection);
	}

	template<typename pass_type>
	void visit(pass_type& pass, const rpc& rpc)
	{
		pass(rpc);
	}

	template<typename pass_type>
	void visit(pass_type& pass, const require& require)
	{
		pass(require);
	}

	class type_map {
	public:
		const projection* get(gsl::czstring<> identifier) const
		{
			const auto first = begin(m_keys);
			const auto last = end(m_keys);
			const auto find_it = std::find(first, last, identifier);

			if (find_it == last) {
				return nullptr;
			}
			else {
				return m_types.at(std::distance(first, find_it));
			}
		}

		// Returns false if we're trying to add an existing type
		bool insert(const projection& projection)
		{
			const auto identifier = projection.identifier();
			const auto last = end(m_keys);
			const auto find_it = std::find(begin(m_keys), last, identifier);

			if (find_it == last) {
				m_keys.push_back(identifier);
				m_types.push_back(&projection);
				return true;
			}
			else {
				return false;
			}
		}

		void debug_dump()
		{
			for (const auto id : m_keys) {
				std::cout << id << "\n";
			}
		}

	private:
		std::vector<gsl::czstring<>> m_keys;
		std::vector<const projection*> m_types;
	};

	class type_collection_pass {
	public:
		type_collection_pass(type_map& types) : m_types {&types}
		{
		}

		void operator()(const file& file)
		{
		}

		void operator()(const include& include)
		{
		}

		void operator()(const module& module)
		{
		}

		void operator()(const projection& projection)
		{
			m_types->insert(projection);
		}

		void operator()(const rpc& rpc)
		{
		}

		void operator()(const require& require)
		{
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
		const auto top_node = std::unique_ptr<idlc::file> {(idlc::file*)Parser::parse(std::string {args[1]})};
		const auto& file = *top_node;

		/*for (const auto& m : top_node->modules()) {
			std::cout << "module " << m->identifier() << "\n";
			idlc::walk_items(m->items());
		}*/

		std::cout << "IDL file OK\n";

		if (args.size() == 3) {
			std::ofstream dump {args[2]};
			std::cout << "Doing AST dump...\n";
			idlc::dump(*top_node, dump);
		}

		std::cout << "Collecting types\n";
		idlc::type_map types;
		idlc::type_collection_pass tc_pass {types};
		visit(tc_pass, file);
		types.debug_dump();

		return 0;
	}
	catch (const Parser::ParseException& e) {
		std::cerr << "\n\nALERT!!! - Caught parser exception" << std::endl;
		std::cerr << e.getReason() << std::endl;
		return 1;
	}
	catch (const std::exception& e) {
		std::cout << "Compilation failed\n";
		return 1;
	}
}
