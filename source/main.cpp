#include <algorithm>
#include <fstream>
#include <string>
#include <iostream>
#include <variant>

#include <gsl/gsl>

#include <absl/strings/string_view.h>

#include "parser/idl_parse.h"
#include "ast/ast_dump.h"
#include "ast/pgraph_dump.h"
#include "ast/pgraph_walk.h"
#include "frontend/name_binding.h"
#include "frontend/analysis.h"
#include "backend/generation.h"
#include "utility.h"

// NOTE: we keep the identifier heap around for basically the entire life of the compiler
// NOTE: Currently we work at the scale of a single file. All modules within a file are treated as implicitly imported.
// TODO: Support merging ASTs via import / use keywords.
// TODO: ndo_start_xmit uncovered an issue with enum promotion not being applicable to function pointers, i.e. the IDL
// needs a concept of enums (so does the pgraph, etc.)

// TODO: @Vikram says: "do you remember the discussion we had before on accessing kernel exported rpc_ptrs from the driver (need trampolines on driver end). by any chance you remember which driver it was?"

namespace idlc {
	namespace {
		class string_const_walk : public ast_walk<string_const_walk> {
		public:
			bool visit_type_spec(type_spec& node)
			{
				if (std::get_if<type_string>(node.stem.get().get()))
					node.is_const = true;

				return true;
			}
		};
	}
}

int main(int argc, char** argv)
{
    const gsl::span<gsl::zstring<>> args {argv, gsl::narrow<std::size_t>(argc)};
    if (argc != 2) {
        std::cout << "Usage: idlc <idl-file>" << std::endl;
        return 1;
    }

    const auto file = idlc::parser::parse_file(gsl::at(args, 1));
    if (!file)
        return 1;

	idlc::dump_ast(*file);
	if (!idlc::bind_all_names(*file)) {
		std::cout << "Error: Not all names were bound\n";
		return 1;
	}

	idlc::string_const_walk string_walk {};
	if (!string_walk.visit_file(*file))
		std::terminate();

	const auto rpcs = idlc::generate_rpc_pgraphs(*file);
	if (!rpcs) {
		std::cout << "Error: pgraph generation failed\n";
		return 1;
	}

	for (const auto& rpc : *rpcs) {
		std::cout << rpc->name << "::__return\n";
		idlc::dump_pgraph(*rpc->ret_pgraph);		
		std::size_t index {};
		for (const auto& arg : rpc->arg_pgraphs) {
			std::cout << rpc->name << "::" << rpc->arguments->at(index)->name << "\n";
			idlc::dump_pgraph(*arg);
			++index;
		}
	}

	idlc::generate(*rpcs);
}