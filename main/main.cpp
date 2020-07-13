#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sstream>

#include <fstream>

#include "node_map.h"
#include "dump.h"
#include "visit.h"
#include "generic_pass.h"
#include "module_import.h"
#include "marshaling.h"
#include "../parser/parser.h"

namespace fs = std::filesystem;

namespace idlc {
	class module_collection_pass : public generic_pass<module_collection_pass> {
	public:
		module_collection_pass(node_map<module>& modules) noexcept : m_modules {&modules}
		{
		}

		void visit_module(module& module)
		{
			if (!m_modules->insert(module)) {
				std::cout << "[error] encountered module redefinition: " << module.identifier() << "\n";
				throw std::exception {};
			}
		}

	private:
		node_map<module>* m_modules;
	};

	class include_file_pass : public generic_pass<include_file_pass> {
	public:
		include_file_pass(const fs::path& relative_to, node_map<module>& modules) :
			m_relative_to {relative_to},
			m_modules {modules}
		{
		}

		void visit_include(include& include)
		{
			const fs::path path {m_relative_to / include.path()};
			if (!fs::exists(path)) {
				std::cout << "[error] could not find include " << include.path().generic_string() << "\n";
				throw std::exception {};
			}

			std::cout << "[info] processing included file: " << path.generic_string() << "\n";

			auto& file = *const_cast<idlc::file*>(
				static_cast<const idlc::file*>(
					Parser::parse(path.generic_string())));

			include.parsed_file.reset(&file);
			std::cout << "[info] collecting modules in file\n";
			module_collection_pass tc_pass {m_modules};
			visit(tc_pass, file);
		}

	private:
		fs::path m_relative_to;
		node_map<module>& m_modules;
	};

	class verify_driver_idl_pass : public generic_pass<verify_driver_idl_pass> {
	public:
		void visit_rpc(const rpc& rpc)
		{
			std::cout << "[error] rpc definition " << rpc.identifier() << " illegal in driver module\n";
			throw std::exception {};
		}

		void visit_projection(const projection& projection) {
			std::cout << "[error] projection definition " << projection.identifier() << " illegal in driver module\n";
			throw std::exception {};
		}

		void visit_module(const module& module)
		{
			if (is_module_found) {
				std::cout << "[error] driver definition IDL may only define the driver module\n";
				throw std::exception {};
			}

			is_module_found = true;
		}

	private:
		bool is_module_found {false};
	};
}


int main(int argc, gsl::czstring<>* argv) {
	const gsl::span args {argv, gsl::narrow_cast<std::size_t>(argc)};

	if (args.size() != 2 && args.size() != 3) {
		std::cout << "Usage: idlc <source-file> [<dump-file>]\n";
		return 1;
	}

	const fs::path idl_path {gsl::at(args, 1)};

	try {
		auto top_node = std::unique_ptr<idlc::file> {
			const_cast<idlc::file*>(
				static_cast<const idlc::file*>(
					Parser::parse(idl_path.generic_string())))};

		auto& file = *top_node;

		std::cout << "[info] verified IDL syntax\n";

		if (args.size() == 3) {
			std::ofstream dump {gsl::at(args, 2)};
			std::cout << "[info] doing AST dump\n";
			idlc::dump(file, dump);
		}

		idlc::verify_driver_idl_pass vdi_pass;
		visit(vdi_pass, file);

		idlc::node_map<idlc::module> imports;
		idlc::include_file_pass if_pass {idl_path.parent_path(), imports};
		visit(if_pass, file);

		std::vector<idlc::marshal_unit> marshal_units;
		idlc::module_import_pass mi_pass {marshal_units, imports};
		visit(mi_pass, file);

		for (const auto& unit : marshal_units) {
			std::cout << "[info] marshal unit " << (unit.is_pointer ? "(function pointer) " : "") << unit.identifier << "\n";
		}

		return 0;
	}
	catch (const Parser::ParseException& e) {
		std::cerr << "[error] parsing failed\n" << std::endl;
		std::cerr << e.getReason() << std::endl;
		return 1;
	}
	catch (const std::exception&) {
		std::cout << "[error] compilation failed\n";
		return 1;
	}
}
