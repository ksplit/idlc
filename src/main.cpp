#include <fstream>
#include <string>
#include <iostream>
#include <variant>

#include <gsl/gsl>

#include <absl/strings/string_view.h>

#include "parser/idl_parse.h"
#include "ast/pgraph_walk.h"
#include "frontend/name_binding.h"
#include "frontend/analysis.h"
#include "tab_over.h"

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

// NOTE: All of these are low-priority, but eventually needed (if it breaks nullnet, it's high-priority)
// TODO: sort out the somewhat hellish logging situation
// TODO: sort out const-ness handling (low-priority, work around in mean time)

// NOTE: all marshaling logic is side-independent, needing only a side-dependent send() primitive
// It's side-dependent where the dispatch loop gets hooked in, however
// and kernel functions must not conflict with the generated marshaling code
// it is the indirect RPCs that are truly side-independent

namespace idlc {
	namespace {
		// TODO: pre-collect all projections in the pgraph tree to allow iteration over them
		class visitor_walk : public pgraph_walk<visitor_walk> {
		public:
			visitor_walk(std::ostream& os) : os_ {os} {}

			bool visit_projection(projection& node)
			{
				os_ << "void " << node.visit_arg_marshal_name
					<< "(\n\tstruct fipc_message*,\n\tstruct " << node.real_name << "*);\n\n";

				os_ << "void " << node.visit_arg_unmarshal_name
					<< "(\n\tstruct fipc_message*,\n\tstruct " << node.real_name << "*);\n\n";

				os_ << "void " << node.visit_arg_remarshal_name
					<< "(\n\tstruct fipc_message*,\n\tstruct " << node.real_name << "*);\n\n";
					
				os_ << "void " << node.visit_arg_unremarshal_name
					<< "(\n\tstruct fipc_message*,\n\tstruct " << node.real_name << "*);\n\n";
					
				os_ << "void " << node.visit_ret_marshal_name
					<< "(\n\tstruct fipc_message*,\n\tstruct " << node.real_name << "*);\n\n";
					
				os_ << "void " << node.visit_ret_unmarshal_name
					<< "(\n\tstruct fipc_message*,\n\tstruct " << node.real_name << "*);\n\n";

				return true;
			}

		private:
			std::ostream& os_;
		};

		void generate_visitor_prototypes(std::ostream& file, gsl::span<const gsl::not_null<rpc_def*>> rpcs)
		{
			visitor_walk visit_walk {file};
			for (const auto& rpc : rpcs) {
				if (rpc->ret_pgraph) {
					const auto succeeded = visit_walk.visit_value(*rpc->ret_pgraph);
					assert(succeeded);
				}

				for (const auto& arg : rpc->arg_pgraphs) {
					const auto succeeded = visit_walk.visit_value(*arg);
					assert(succeeded);
				}
			}
		}

		void generate_common_header(gsl::span<const gsl::not_null<rpc_def*>> rpcs)
		{
			std::ofstream file {"common.h"};
			file.exceptions(file.badbit | file.failbit);

			file << "#ifndef COMMON_H\n#define COMMON_H\n\n";
			file << "#include <liblcd/trampoline.h>\n";
			file << "#include <libfipc.h>\n";
			file << "#include <asm/lcd_domains/libvmfunc.h>\n";
			file << "\n";
			file << "#define glue_marshal_shadow(msg, value) // TODO\n";
			file << "#define glue_marshal(msg, value) // TODO\n\n";
			file << "enum RPC_ID {\n";
			for (const auto& rpc : rpcs) {
				file << "\t" << rpc->enum_id << ",\n";
			}

			file << "};\n\n";

			generate_visitor_prototypes(file, rpcs);
			for (const auto& rpc : rpcs) {
				if (rpc->kind == rpc_def_kind::indirect) {
					file << "typedef " << rpc->ret_string << " (*" << rpc->typedef_id << ")("
						<< rpc->args_string << ");\n";

					file << "typedef " << rpc->ret_string << " (*" << rpc->impl_typedef_id << ")(" << rpc->typedef_id
						<< " target, " << rpc->args_string << ");\n\n";
				}
			}

			file << "\n#endif\n";
		}

		template<typename derived>
		class marshal_walk : public pgraph_walk<derived> {
		public:
			marshal_walk(std::ostream& os, absl::string_view holder, unsigned level) :
				os_ {os},
				marshaled_ptr_ {holder},
				indent_level_ {level}
			{}

		protected:		
			// identifier of the variable that points to what we're currently marshaling
			const auto& marshaled_variable()
			{
				return marshaled_ptr_;
			}

			template<typename node_type>
			bool marshal(std::string&& new_ptr, node_type& type)
			{
				const auto state = std::make_tuple(marshaled_ptr_, indent_level_);
				marshaled_ptr_ = new_ptr;
				++indent_level_;
				if (!this->traverse(this->self(), type))
					return false;

				std::forward_as_tuple(marshaled_ptr_, indent_level_) = state;

				return true;
			}

			std::ostream& stream()
			{
				return indent(os_, indent_level_);
			}

		private:
			std::ostream& os_;
			std::string marshaled_ptr_ {};
			unsigned indent_level_ {};
		};

		class arg_marshal_walk : public marshal_walk<arg_marshal_walk> {
		public:
			arg_marshal_walk(std::ostream& os, absl::string_view holder, unsigned level) :
				marshal_walk {os, holder, level}
			{}

			// TODO: lowering *needs* to compute a size-of-field expression for alloc support

			bool visit_pointer(pointer& node)
			{
				if (flags_set(node.pointer_annots, annotation::bind_caller))
					stream() << "glue_marshal_shadow(msg, *" << marshaled_variable() << ");\n";
				else
					stream() << "glue_marshal(msg, *" << marshaled_variable() << ");\n";

				stream() << "if (*" << marshaled_variable() << ") {\n";
				marshal("*" + marshaled_variable(), node);
				stream() << "}\n\n";

				return true;
			}

			bool visit_primitive(primitive node)
			{
				stream() << "glue_marshal(msg, *" << marshaled_variable() << ");\n";
				return true;
			}

			bool visit_rpc_ptr(rpc_ptr& node)
			{
				stream() << "glue_marshal(msg, *" << marshaled_variable() << ");\n";
				return true;
			}

			bool visit_projection(projection& node)
			{
				stream() << node.visit_arg_marshal_name << "(msg, " << marshaled_variable() << ");\n";
				return true;
			}

			bool visit_null_terminated_array(null_terminated_array& node)
			{
				stream() << "int i;\n";
				stream() << node.element->type_string << "* array = " << marshaled_variable() << ";\n";
				stream() << "for (i = 0; array[i]; ++i) {\n";
				marshal("array[i]", node);
				stream() << "}\n\n";
				return true;
			}

			bool visit_value(value& node)
			{
				if ((node.value_annots & annotation::in) == annotation::is_set) {
					std::cout << "Skipped non-in field\n";
					return true;
				}
				
				return traverse(*this, node);
			}
		};

		class arg_unmarshal_walk : public pgraph_walk<arg_unmarshal_walk> {};
		class arg_remarshal_walk : public pgraph_walk<arg_remarshal_walk> {};
		class arg_unremarshal_walk : public pgraph_walk<arg_unremarshal_walk> {};
		class ret_marshal_walk : public pgraph_walk<ret_marshal_walk> {};
		class ret_unmarshal_walk : public pgraph_walk<ret_unmarshal_walk> {};

		auto generate_roots(rpc_def& rpc, std::ostream& os)
		{
			const auto n_args = rpc.arg_pgraphs.size();
			std::vector<std::string> roots(n_args);
			for (gsl::index i {}; i < n_args; ++i) {
				const auto name = rpc.arguments->at(i)->name;
				const auto ts = rpc.arg_pgraphs.at(i)->type_string;
				std::string ptr_name {name};
				ptr_name += "_ptr";
				os << "\t" << ts << "* " << ptr_name << " = &" << name << ";\n";
				roots.at(i) = ptr_name;
			}

			os << "\t\n";

			return roots;
		}

		void generate_caller_glue(rpc_def& rpc, std::ostream& os)
		{
			os << "\tstruct fipc_message msg_buf = {0};\n";
			os << "\tstruct fipc_message *msg = &msg_buf;\n\n";

			// TODO: n_args is re-computed twice
			const auto n_args = rpc.arg_pgraphs.size();
			std::vector<std::string> roots = generate_roots(rpc, os);

			for (gsl::index i {}; i < n_args; ++i) {
				arg_marshal_walk arg_marshal {os, roots.at(i), 1}; // TODO: collect names
				arg_marshal.visit_value(*rpc.arg_pgraphs.at(i));
			}

			os << "\tvmfunc_wrapper(msg);\n\n";

			for (auto& arg : rpc.arg_pgraphs) {
				arg_unremarshal_walk arg_unremarshal {};
				arg_unremarshal.visit_value(*arg);
			}

			if (rpc.ret_pgraph) {
				ret_unmarshal_walk ret_unmarshal {};
				ret_unmarshal.visit_value(*rpc.ret_pgraph);
			}

			os << (rpc.ret_pgraph ? "\treturn 0;\n" : ""); 
		}

		void generate_callee_glue(rpc_def& rpc, std::ostream& os)
		{
			os << "\t// callee glue here\n";
		}

		void generate_indirect_rpc(rpc_def& rpc, std::ostream& os)
		{
			os << rpc.ret_string << " " << rpc.impl_id << "(" << rpc.typedef_id << " target, "
				<< rpc.args_string << ")\n{\n";

			generate_caller_glue(rpc, os);
			os << "}\n\n";

			os << "void " << rpc.callee_id << "(struct fipc_message* msg)\n{\n";
			generate_callee_glue(rpc, os);
			os << "}\n\n";

			os << "LCD_TRAMPOLINE_DATA(" << rpc.trmp_id << ")\n";
			os << rpc.ret_string;
			os << " LCD_TRAMPOLINE_LINKAGE(" << rpc.trmp_id << ") ";
			os << rpc.trmp_id << "(" << rpc.args_string << ")\n{\n";

			os << "\tvolatile " << rpc.impl_typedef_id << " impl;\n";
			os << "\t" << rpc.typedef_id << " target;\n";
			os << "\tLCD_TRAMPOLINE_PROLOGUE(target, " << rpc.trmp_id << ");\n";
			os << "\timpl = " << rpc.impl_id << ";\n";
			os << "\treturn impl(target, " << rpc.params_string << ");\n";

			os << "}\n\n";
		}

		void generate_caller(gsl::span<const gsl::not_null<rpc_def*>> rpcs)
		{
			std::ofstream file {"caller.c"};
			file.exceptions(file.badbit | file.failbit);

			file << "#include <lcd_config/pre_hook.h>\n\n";
			file << "#include \"common.h\"\n\n";
			file << "#include <lcd_config/post_hook.h>\n\n";
			for (const auto& rpc : rpcs) {
				if (rpc->kind == rpc_def_kind::direct) {
					file << rpc->ret_string << " " << rpc->name << "(" << rpc->args_string << ")\n{\n";
					generate_caller_glue(*rpc, file);
					file << "}\n\n";
				}
				else {
					generate_indirect_rpc(*rpc, file);
				}
			}
		}

		void generate_callee(gsl::span<const gsl::not_null<rpc_def*>> rpcs)
		{
			std::ofstream file {"callee.c"};
			file.exceptions(file.badbit | file.failbit);

			file << "#include <lcd_config/pre_hook.h>\n\n";
			file << "#include \"common.h\"\n\n";
			file << "#include <lcd_config/post_hook.h>\n\n";
			for (const auto& rpc : rpcs) {
				if (rpc->kind == rpc_def_kind::indirect) {
					generate_indirect_rpc(*rpc, file);
				}
				else {
					file << "void " << rpc->callee_id << "(struct fipc_message* msg)\n{\n";
					generate_callee_glue(*rpc, file);
					file << "}\n\n";
				}
			}
		}

		void generate_linker_script(gsl::span<const gsl::not_null<rpc_def*>> rpcs)
		{
			std::ofstream file {"trampolines.lds.S"};
			file.exceptions(file.badbit | file.failbit);

			file << "#include <liblcd/trampoline_link.h>\n\n";
			file << "SECTIONS{\n";

			for (const auto& rpc : rpcs) {
				if (rpc->kind == rpc_def_kind::indirect)
					file << "\tLCD_TRAMPOLINE_LINKER_SECTION(" << rpc->trmp_id << ")\n";
			}

			file << "}\n";
			file << "INSERT AFTER .text\n";
		}

		void create_alternate_names(gsl::span<const gsl::not_null<rpc_def*>> rpcs)
		{
			for (const auto& rpc : rpcs) {
				rpc->enum_id = "RPC_ID_";
				rpc->enum_id += rpc->name;
				rpc->callee_id = rpc->name;
				rpc->callee_id += "_callee";

				if (rpc->kind == rpc_def_kind::indirect) {
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

		void create_function_strings(gsl::span<const gsl::not_null<rpc_def*>> rpcs)
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

/*
	TODO:
	- split sema into the name-binding rpc-collection parts, vs the pgraph analysis parts
	- merge AST and pgraph into a single module, they're interdependent anyways
	- instead of extracting the pgraph owners, root them in their respective RPC nodes and do away with the unused table
	- move code generation into its own module
	- stages after the initial sema stage work solely on the table of RPCs (namely pgraph analysis and code generation)
	- move string processing to stringstream instead of concats (efficiency issue, heap reallocations, etc.)
*/

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

	const auto rpcs = idlc::get_rpcs(*file);
	if (!idlc::generate_pgraphs(rpcs)) {
		std::cout << "Error: pgraph generation failed\n";
		return 1;
	}

	idlc::create_function_strings(rpcs);
	idlc::generate_common_header(rpcs);
	idlc::generate_caller(rpcs);
	idlc::generate_callee(rpcs);
	idlc::generate_linker_script(rpcs);
}