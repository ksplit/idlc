#include <fstream>
#include <string>
#include <iostream>
#include <variant>

#include <gsl/gsl>

#include "parser/idl_parse.h"
#include "ast/ast_dump.h"
#include "sema/name_binding.h"
#include "sema/pgraph_dump.h"
#include "sema/pgraph_generation.h"
#include "sema/lowering.h"

// NOTE: we keep the identifier heap around for basically the entire life of the compiler
// NOTE: Currently we work at the scale of a single file. All modules within a file are treated as implicitly imported.
// TODO: Support merging ASTs via import / use keywords.

/*
	Marshaling logic gets outputted as a per-passing-tree list of "blocks" that form a graph structure.
	These are essentially visitors, which visit the fields at runtime to correctly pack them into the message buffer.
	The goal is to encode enough information here that we could later do optimization passes over it. Such passes
	would only be justified if it's places where the compiler cannot feasibly optimize it for us, or places where
	said optimization improves debuggability. The major candidates are trampoline elision, static assignment of IPC
	registers, (??)
*/

/*
	NOTE: Marshaling

	As for outptutting C code for marshaling instructions: we've run into the issue before of having to wrap function
	pointer types in awkward ways. IIRC, we needed to know the pointer types of RPC pointers in a very "full" way,
	both the specifier form and the actual type name. Check lvd_linux's generated drivers for more info.
	
	These can be handled similar to variants: pre-computed strings that we can generate in batches. We could generate
	the type "forms" on a per-node basis in the passing trees, and simply cache that.
	
	These type specifiers / declarators and the identifier variants could be done during the C output stage, or as part
	of marshalling generation: temporary variable names could be decided during IR generation, or perhaps the IR itself
	encodes enough information to generate that later.
	
	Note that each IR "block" will end up generating a marshal_* function.
	Each block is per-projection, and RPC-scoped projections will get names correspondingly prepended with the RPC name
	
	Package as much as possible into helper functions, this greatly simplifies code generation
	- you can package bind / alloc / dealloc into helpers
*/

/*
	NOTE: front-end passes are irrelevant rn, work on:
	- marshal IR
	- 6 IR generation passes (!!)
	- code generation
*/

/*
	NOTE: Vikram notes:
	- initial pass recording IDs and types of every object in the graph
	- serialize to flattened array of registers, pointers replaced by IDs
	- unrealistic to build right now
	- focus on subset of features
		- static arrays
		- pointers (non-cyclic?)
		- unions
		- structs
		- minimum viable: focus on nullnet
*/

// Note that Roslyn appears to render the AST completely immutable, forcing node properties to be stored
// by association (hash table, red node, etc.)

// NOTE: Default to a walk, re-write later as needed (premature optimization, etc.)
// NOTE: All of these are low-priority, but eventually needed (if it breaks nullnet, it's high-priority)
// TODO: sort out the somewhat hellish logging situation
// TODO: sort out const-ness handling (low-priority, work around in mean time)

namespace idlc {
	namespace {
		void generate_common_header(gsl::span<const gsl::not_null<ast::rpc_def*>> rpcs)
		{
			std::ofstream file {"common.h"};
			file.exceptions(file.badbit | file.failbit);

			file << "#ifndef COMMON_H\n#define COMMON_H\n\n";
			file << "#include <liblcd/trampoline.h>\n";
			file << "\n";
			file << "enum RPC_ID {\n";
			for (const auto& rpc : rpcs) {
				file << "\t" << rpc->enum_id << ",\n";
			}

			file << "};\n\n";
			for (const auto& rpc : rpcs) {
				if (rpc->kind == ast::rpc_def_kind::indirect) {
					file << "typedef " << rpc->ret_string << " (*" << rpc->typedef_id << ")("
						<< rpc->args_string << ");\n";

					file << "typedef " << rpc->ret_string << " (*" << rpc->impl_typedef_id << ")(" << rpc->typedef_id
						<< " target, " << rpc->args_string << ");\n\n";
				}
			}

			file << "\n#endif\n";
		}

		void generate_indirect_rpc(ast::rpc_def& rpc, std::ostream& os)
		{
			os << rpc.ret_string << " " << rpc.impl_id << "(" << rpc.typedef_id << " target, "
				<< rpc.args_string << ");\n\n";

			os << "LCD_TRAMPOLINE_DATA(" << rpc.trmp_id << ")\n";
			os << rpc.ret_string;
			os << " LCD_TRAMPOLINE_LINKAGE(" << rpc.trmp_id << ") ";
			os << rpc.trmp_id << "(" << rpc.args_string << ") {\n";

			os << "\tvolatile " << rpc.impl_typedef_id << " impl;\n";
			os << "\t" << rpc.typedef_id << " target;\n";
			os << "\tLCD_TRAMPOLINE_PROLOGUE(target, " << rpc.trmp_id << ");\n";
			os << "\timpl = " << rpc.impl_id << ";\n";
			os << "\treturn impl(target, " << rpc.params_string << ");\n";

			os << "}\n\n";
		}

		void generate_caller(gsl::span<const gsl::not_null<ast::rpc_def*>> rpcs)
		{
			std::ofstream file {"caller.c"};
			file.exceptions(file.badbit | file.failbit);

			file << "#include <lcd_config/pre_hook.h>\n\n";
			file << "#include \"common.h\"\n\n";
			file << "#include <lcd_config/post_hook.h>\n\n";
			for (const auto& rpc : rpcs) {
				if (rpc->kind == ast::rpc_def_kind::direct) {
					file << rpc->ret_string << " " << rpc->name << "(" << rpc->args_string << ");\n\n";
				}
				else {
					generate_indirect_rpc(*rpc, file);
				}
			}
		}

		void generate_callee(gsl::span<const gsl::not_null<ast::rpc_def*>> rpcs)
		{
			std::ofstream file {"callee.c"};
			file.exceptions(file.badbit | file.failbit);

			file << "#include <lcd_config/pre_hook.h>\n\n";
			file << "#include \"common.h\"\n\n";
			file << "#include <lcd_config/post_hook.h>\n\n";
			for (const auto& rpc : rpcs) {
				if (rpc->kind == ast::rpc_def_kind::indirect) {
					generate_indirect_rpc(*rpc, file);
				}
			}
		}

		void generate_linker_script(gsl::span<const gsl::not_null<ast::rpc_def*>> rpcs)
		{
			std::ofstream file {"trampolines.lds.S"};
			file.exceptions(file.badbit | file.failbit);

			file << "#include <liblcd/trampoline_link.h>\n\n";
			file << "SECTIONS{\n";

			for (const auto& rpc : rpcs) {
				if (rpc->kind == ast::rpc_def_kind::indirect)
					file << "\tLCD_TRAMPOLINE_LINKER_SECTION(" << rpc->trmp_id << ")\n";
			}

			file << "}\n";
			file << "INSERT AFTER .text\n";
		}

		void create_alternate_names(gsl::span<const gsl::not_null<ast::rpc_def*>> rpcs)
		{
			for (const auto& rpc : rpcs) {
				rpc->enum_id = "RPC_ID_";
				rpc->enum_id += rpc->name;
				rpc->callee_id = rpc->name;
				rpc->callee_id += "_callee";

				if (rpc->kind == ast::rpc_def_kind::indirect) {
					rpc->trmp_id = "trmp_";
					rpc->trmp_id += rpc->name;
					rpc->impl_id = "trmp_impl_";
					rpc->impl_id += rpc->name;
					rpc->typedef_id = "fptr_";
					rpc->typedef_id += rpc->name;
					rpc->impl_typedef_id = "fptr_impl_";
					rpc->impl_typedef_id += rpc->name;
				}
			}
		}

		void create_function_strings(gsl::span<const gsl::not_null<ast::rpc_def*>> rpcs)
		{
			for (auto& rpc : rpcs) {
				if (rpc->ret_type)
					rpc->ret_string = rpc->ret_pgraph->type_string;
				else
					rpc->ret_string = "void";

				if (rpc->arguments) {
					bool is_first {true};
					for (gsl::index i {}; i < rpc->arguments->size(); ++i) {
						const auto& arg = rpc->arg_pgraphs.at(i);
						const auto name = rpc->arguments->at(i)->name;
						if (!is_first) {
							rpc->args_string += ", ";
							rpc->params_string += ", ";
						}

						rpc->args_string += arg->type_string;
						rpc->args_string += " ";
						rpc->args_string += name;

						rpc->params_string += name;

						is_first = false;
					}
				}
				else {
					rpc->args_string = "void";
				}
			}
		}
	}
}

// NOTE: it is possible to implement type naming as a walk over all var_decls

int main(int argc, char** argv)
{
	std::set_terminate([] {
		std::cout.flush();
		std::abort();
	});

    const gsl::span<gsl::zstring<>> args {argv, gsl::narrow<std::size_t>(argc)};
    if (argc != 2) {
        std::cout << "Usage: idlc <idl-file>" << std::endl;
        return 1;
    }

    const auto driver_idl = idlc::parser::parse_file(gsl::at(args, 1));
    if (!driver_idl)
        return 1;

	auto& file = *driver_idl;
	if (!idlc::sema::bind_all(file)) {
		std::cout << "Error: Not all names were bound\n";
		return 1;
	}

	const auto rpcs = idlc::sema::get_rpcs(file);
	const auto data_fields = idlc::sema::generate_pgraphs(rpcs);
	idlc::create_alternate_names(rpcs); // Note: must occur before type-naming step
	if (!idlc::sema::lower(rpcs)) {
		std::cout << "Error: pgraph lowering failed\n";
		return 1;
	}

	// Need to generate all the walk names for each projection
	// Need to somehow walk all reachable projections (easy)
	// - for each rpc, walk all pgraph roots with same walk object, only look at projections

	idlc::create_function_strings(rpcs);
	idlc::generate_common_header(rpcs);
	idlc::generate_caller(rpcs);
	idlc::generate_callee(rpcs);
	idlc::generate_linker_script(rpcs);
}