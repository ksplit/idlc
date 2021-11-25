#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <variant>

#include <gsl/gsl>

#include <absl/strings/string_view.h>

#include "ast/ast_dump.h"
#include "ast/pgraph_dump.h"
#include "ast/pgraph_walk.h"
#include "backend/generation.h"
#include "frontend/analysis.h"
#include "frontend/name_binding.h"
#include "parser/idl_parse.h"
#include "utility.h"

// NOTE: we keep the identifier heap around for basically the entire life of the compiler
// NOTE: Currently we work at the scale of a single file. All modules within a file are treated as implicitly imported.
// TODO: Support merging ASTs via import / use keywords.
// TODO: ndo_start_xmit uncovered an issue with enum promotion not being applicable to function pointers, i.e. the IDL
// needs a concept of enums (so does the pgraph, etc.)

// TODO: @Vikram says: "do you remember the discussion we had before on accessing kernel exported rpc_ptrs from the
// driver (need trampolines on driver end). by any chance you remember which driver it was?"

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

		template <bool trampolines_allowed>
		class trampoline_sanity_walk : public pgraph_walk<trampoline_sanity_walk<trampolines_allowed>> {
		public:
			bool visit_rpc_ptr(rpc_ptr& node)
			{
				if (!node.is_static) {
					if (is_driver_import || !trampolines_allowed) {
						std::cout << "error: trampoline of type " << node.definition->name << " not allowed here.\n";
						return false;
					}
				}
				else if (trampolines_allowed && !is_driver_import) {
					std::cout << "warning: trampoline can be used instead of static rpc pointer here\n";
				}

				return this->traverse(*this, node);
			}

			bool visit_value(value& node)
			{
				const auto old = is_driver_import;
				is_driver_import = flags_set(node.value_annots, annotation_kind::out);
				if (!this->traverse(*this, node))
					return false;

				is_driver_import = old;

				return true;
			}

		private:
			bool is_driver_import {};
		};

		template <bool trampolines_allowed>
		bool ensure_trampoline_sanity(rpc_def& rpc)
		{
			if (rpc.ret_pgraph) {
				trampoline_sanity_walk<trampolines_allowed> walk {};
				if (!walk.visit_value(*rpc.ret_pgraph)) {
					std::cout << "note: in return value of rpc " << rpc.name << "\n";
					return false;
				}
			}

			auto count = 0;
			for (const auto& argument : rpc.arg_pgraphs) {
				trampoline_sanity_walk<trampolines_allowed> walk {};
				if (!walk.visit_value(*argument)) {
					std::cout << "note in argument " << rpc.arguments->at(count)->name << " of rpc " << rpc.name
							  << "\n";

					return false;
				}

				++count;
			}

			return true;
		}

		bool ensure_trampoline_sanity(rpc_vec_view rpcs)
		{
			trampoline_sanity_walk<true> trmp_exports_only {};
			trampoline_sanity_walk<false> no_trmps {};
			for (const auto& rpc : rpcs) {
				switch (rpc->kind) {
				case rpc_def_kind::direct:
					if (!ensure_trampoline_sanity<true>(*rpc))
						return false;

					break;

				default:
					if (!ensure_trampoline_sanity<false>(*rpc))
						return false;

					break;
				}
			}

			return true;
		}
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

	if (!idlc::ensure_trampoline_sanity(*rpcs)) {
		std::cout << "error: trampoline sanity rules violation\n";
		return 1;
	}

	idlc::generate(*rpcs);
}