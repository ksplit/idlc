
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <fstream>

#include "node_map.h"
#include "dump.h"
#include "visit.h"
#include "../parser/parser.h"

namespace fs = std::filesystem;

namespace idlc
{
	// RPCs and RPC pointers, though different nodes, each have a signature, used to generate their marshaling info

	class type_collection_pass {
	public:
		void operator()(const file& file) noexcept {}
		void operator()(const include& include) noexcept {}
		void operator()(const rpc& rpc) noexcept {}
		void operator()(const require& require) noexcept {}
		void operator()(const var_field& field) noexcept {}
		void operator()(const rpc_field& field) noexcept {}
		void operator()(const projection_type& proj) noexcept {}
		void operator()(const primitive_type& prim) noexcept {}
		void operator()(const type& ty) noexcept {}
		void operator()(const signature& sig) noexcept {}
		void operator()(const attributes& attribs) noexcept {}

		void operator()(module& module) noexcept
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
		node_map<const projection>* m_types;
	};

	// TODO: would be nice to have an error context
	// NOTE: I don't think exceptions are terribly appropriate for this

	class type_resolve_pass {
	public:
		void operator()(const file& file) noexcept {}
		void operator()(const include& include) noexcept {}
		void operator()(const rpc& rpc) noexcept {}
		void operator()(const require& require) noexcept {}
		void operator()(const primitive_type& prim) noexcept {}
		void operator()(const var_field& field) noexcept {}
		void operator()(const rpc_field& field) noexcept {}
		void operator()(const type& ty) noexcept {}
		void operator()(const signature& sig) noexcept {}
		void operator()(const attributes& attribs) noexcept {}
		void operator()(const projection& projection) noexcept {}

		void operator()(module& module) noexcept
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
				std::cout << "Error: could not resolve projection: " << proj.identifier() << "\n";
				throw std::exception {};
			}
		}

	private:
		node_map<const projection>* m_types;
	};

	class module_collection_pass {
	public:
		module_collection_pass(node_map<module>& modules) noexcept : m_modules {&modules}
		{
		}

		void operator()(const include& include) noexcept {}
		void operator()(const rpc& rpc) noexcept {}
		void operator()(const require& require) noexcept {}
		void operator()(const projection& projection) noexcept {}
		void operator()(const primitive_type& prim) noexcept {}
		void operator()(const var_field& field) noexcept {}
		void operator()(const rpc_field& field) noexcept {}
		void operator()(const type& ty) noexcept {}
		void operator()(const signature& sig) noexcept {}
		void operator()(const attributes& attribs) noexcept {}
		void operator()(const projection_type& proj) noexcept {}
		void operator()(const file& file) noexcept {}

		void operator()(module& module)
		{
			if (!m_modules->insert(module)) {
				std::cout << "Error: encountered module redefinition: " << module.identifier() << "\n";
				throw std::exception {};
			}
		}

	private:
		node_map<module>* m_modules;
	};

	class include_file_pass {
	public:
		include_file_pass(const fs::path& relative_to) :
			m_relative_to {relative_to},
			m_modules {nullptr}
		{
		}

		void operator()(const rpc& rpc) noexcept {}
		void operator()(const require& require) noexcept {}
		void operator()(const projection& projection) noexcept {}
		void operator()(const primitive_type& prim) noexcept {}
		void operator()(const var_field& field) noexcept {}
		void operator()(const rpc_field& field) noexcept {}
		void operator()(const type& ty) noexcept {}
		void operator()(const signature& sig) noexcept {}
		void operator()(const attributes& attribs) noexcept {}
		void operator()(const projection_type& proj) noexcept {}
		void operator()(const module& module) noexcept {}

		void operator()(file& file) noexcept
		{
			m_modules = &file.included_modules;
		}

		// These are for the analysis tool
		[[gsl::suppress(type.3)]]
		void operator()(include& include)
		{
			const fs::path path {m_relative_to / include.path()};
			if (!fs::exists(path)) {
				std::cout << "Error: could not find include " << include.path().generic_string() << "\n";
				throw std::exception {};
			}

			std::cout << "Info: processing included file: " << path.generic_string() << "\n";

			auto& file = *const_cast<idlc::file*>(
				static_cast<const idlc::file*>(
					Parser::parse(path.generic_string())));

			include.parsed_file.reset(&file);
			std::cout << "Info: collecting modules in file\n";
			module_collection_pass tc_pass {*m_modules};
			visit(tc_pass, file);
		}

	private:
		fs::path m_relative_to;
		node_map<module>* m_modules;
	};

	class verify_driver_idl_pass {
	public:
		void operator()(const require& require) noexcept {}
		void operator()(const primitive_type& prim) noexcept {}
		void operator()(const var_field& field) noexcept {}
		void operator()(const rpc_field& field) noexcept {}
		void operator()(const type& ty) noexcept {}
		void operator()(const signature& sig) noexcept {}
		void operator()(const attributes& attribs) noexcept {}
		void operator()(const projection_type& proj) noexcept {}
		void operator()(const file& file) noexcept {}
		void operator()(const include& include) noexcept {}

		void operator()(const rpc& rpc)
		{
			std::cout << "Error: rpc definition " << rpc.identifier() << " illegal in driver module\n";
			throw std::exception {};
		}

		void operator()(const projection& projection) {
			std::cout << "Error: projection definition " << projection.identifier() << " illegal in driver module\n";
			throw std::exception {};
		}

		void operator()(const module& module)
		{
			if (is_module_found) {
				std::cout << "Error: driver definition IDL may only define the driver module\n";
				throw std::exception {};
			}

			is_module_found = true;
		}

	private:
		bool is_module_found {false};
	};

	class module_resolve_pass {
	public:
		void operator()(const include& include) noexcept {}
		void operator()(const rpc& rpc) noexcept {}
		void operator()(const projection& projection) noexcept {}
		void operator()(const primitive_type& prim) noexcept {}
		void operator()(const var_field& field) noexcept {}
		void operator()(const rpc_field& field) noexcept {}
		void operator()(const type& ty) noexcept {}
		void operator()(const signature& sig) noexcept {}
		void operator()(const attributes& attribs) noexcept {}
		void operator()(const projection_type& proj) noexcept {}
		void operator()(const module& module) noexcept {}

		void operator()(const file& file) noexcept
		{
			m_modules = &file.included_modules;
		}

		void operator()(const require& require)
		{
			module* const ptr {m_modules->get(require.identifier())};
			if (!ptr) {
				std::cout << "Error: could not resolve required module " << require.identifier() << "\n";
				throw std::exception {};
			}

			std::cout << "Info: processing required module " << require.identifier() << "\n";

			module& mod {*ptr};
			type_collection_pass tc;
			type_resolve_pass tr;
			visit(tc, mod);
			visit(tr, mod);
		}

	private:
		const node_map<module>* m_modules;
	};
}


int main(int argc, gsl::czstring<>* argv) {
	const gsl::span args {argv, gsl::narrow_cast<std::size_t>(argc)};

	if (args.size() != 2 && args.size() != 3) {
		std::cout << "Usage: idlc <source-file> [<dump-file>]\n";
		return 1;
	}

	try {
		auto top_node = std::unique_ptr<idlc::file> {
			const_cast<idlc::file*>(
				static_cast<const idlc::file*>(
					Parser::parse(std::string {gsl::at(args, 1)})))};

		auto& file = *top_node;

		std::cout << "Info: verified IDL syntax\n";

		if (args.size() == 3) {
			std::ofstream dump {gsl::at(args, 2)};
			std::cout << "Info: doing AST dump\n";
			idlc::dump(file, dump);
		}

		idlc::verify_driver_idl_pass vdi_pass;
		visit(vdi_pass, file);

		const auto base_path = fs::path {gsl::at(args, 1)}.parent_path();
		idlc::include_file_pass if_pass {base_path};
		visit(if_pass, file);

		idlc::module_resolve_pass mr_pass;
		visit(mr_pass, file);

		return 0;
	}
	catch (const Parser::ParseException& e) {
		std::cerr << "Error: parsing failed\n" << std::endl;
		std::cerr << e.getReason() << std::endl;
		return 1;
	}
	catch (const std::exception&) {
		std::cout << "Error: compilation failed\n";
		return 1;
	}
}
