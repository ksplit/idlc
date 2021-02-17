#include <algorithm>
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
#include "utility.h"

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
		using projection_vec = std::vector<gsl::not_null<projection*>>;
		using projection_vec_view = gsl::span<const gsl::not_null<projection*>>;

		class visitor_walk : public pgraph_walk<visitor_walk> {
		public:
			visitor_walk(std::ostream& os) : m_stream {os} {}

			bool visit_projection(projection& node)
			{
				m_stream << "void " << node.arg_marshal_visitor
					<< "(\n\tstruct fipc_message*,\n\tstruct " << node.real_name << "*);\n\n";

				m_stream << "void " << node.arg_unmarshal_visitor
					<< "(\n\tstruct fipc_message*,\n\tstruct " << node.real_name << "*);\n\n";

				m_stream << "void " << node.arg_remarshal_visitor
					<< "(\n\tstruct fipc_message*,\n\tstruct " << node.real_name << "*);\n\n";
					
				m_stream << "void " << node.arg_unremarshal_visitor
					<< "(\n\tstruct fipc_message*,\n\tstruct " << node.real_name << "*);\n\n";
					
				m_stream << "void " << node.ret_marshal_visitor
					<< "(\n\tstruct fipc_message*,\n\tstruct " << node.real_name << "*);\n\n";
					
				m_stream << "void " << node.ret_unmarshal_visitor
					<< "(\n\tstruct fipc_message*,\n\tstruct " << node.real_name << "*);\n\n";

				return true;
			}

		private:
			std::ostream& m_stream;
		};

		void generate_visitor_prototypes(std::ostream& file, projection_vec_view projections)
		{
			visitor_walk visit_walk {file};
			for (const auto& proj : projections)
				visit_walk.visit_projection(*proj);
		}

		void generate_common_header(rpc_vec_view rpcs, projection_vec_view projections)
		{
			std::ofstream file {"common.h"};
			file.exceptions(file.badbit | file.failbit);

			file << "#ifndef COMMON_H\n#define COMMON_H\n\n";
			file << "#include <liblcd/trampoline.h>\n";
			file << "#include <libfipc.h>\n";
			file << "#include <asm/lcd_domains/libvmfunc.h>\n";
			file << "\n";
			file << "#define glue_marshal_shadow(msg, value) // TODO\n";
			file << "#define glue_marshal(msg, value) // TODO\n";
			file << "#define glue_unmarshal(msg, type) (type)0xdeadbeef // TODO\n";
			file << "#define glue_unmarshal_rpc_ptr(msg, trmp_id) 0 // TODO\n\n";
			file << "enum RPC_ID {\n";
			for (const auto& rpc : rpcs) {
				file << "\t" << rpc->enum_id << ",\n";
			}

			file << "};\n\n";

			generate_visitor_prototypes(file, projections);
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
				m_stream {os},
				m_marshaled_ptr {holder},
				m_marshaled_type_specifier {},
				m_indent_level {level}
			{}

		protected:		
			// identifier of the variable that points to what we're currently marshaling
			const auto& marshaled_variable()
			{
				return m_marshaled_ptr;
			}

			const auto& c_specifier()
			{
				return m_marshaled_type_specifier;
			}

			template<typename node_type>
			bool marshal(std::string&& new_ptr, node_type& type)
			{
				const auto state = std::make_tuple(m_marshaled_ptr, m_indent_level);
				m_marshaled_ptr = new_ptr;
				++m_indent_level;
				if (!this->traverse(this->self(), type))
					return false;

				std::forward_as_tuple(m_marshaled_ptr, m_indent_level) = state;

				return true;
			}

			std::ostream& stream()
			{
				return indent(m_stream, m_indent_level);
			}

		private:
			std::ostream& m_stream;
			std::string m_marshaled_ptr {};
			std::string m_marshaled_type_specifier {};
			unsigned m_indent_level {};
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
				stream() << node.arg_marshal_visitor << "(msg, " << marshaled_variable() << ");\n";
				return true;
			}

			bool visit_null_terminated_array(null_terminated_array& node)
			{
				stream() << "int i;\n";
				stream() << node.element->c_specifier << "* array = " << marshaled_variable() << ";\n";
				stream() << "for (i = 0; array[i]; ++i) {\n";
				stream() << "\t" << node.element->c_specifier << "* element = &array[i];\n";
				if (!marshal("element", node))
					return false;

				stream() << "}\n\n";

				return true;
			}

			bool visit_value(value& node)
			{
				if (flags_set(node.value_annots, annotation::in)) {
					std::cout << "Marshaling input argument\n";
					return traverse(*this, node);
				}

				std::cout << "Skipped non-input argument for marshaling\n";

				return true;				
			}
		};

		class arg_unmarshal_walk : public marshal_walk<arg_unmarshal_walk> {
		public:
			arg_unmarshal_walk(std::ostream& os, absl::string_view holder, unsigned level) :
				marshal_walk {os, holder, level}
			{}

			bool visit_value(value& node)
			{
				if (flags_set(node.value_annots, annotation::in)) {
					std::cout << "Unmarshaling input argument\n";
					const auto old = m_c_specifier;
					m_c_specifier = node.c_specifier;
					if (!traverse(*this, node))
						return false;

					m_c_specifier = old;
				}
				
				std::cout << "Skipped non-input argument for unmarshaling\n";

				return true;
			}

			bool visit_primitive(primitive node)
			{
				stream() << "*" << marshaled_variable() << " = glue_unmarshal(msg, " << m_c_specifier << ");\n";
				return true;
			}

			bool visit_pointer(pointer& node)
			{
				stream() << "*" << marshaled_variable() << " = glue_unmarshal(msg, " << m_c_specifier << ");\n";
				stream() << "if (*" << marshaled_variable() << ") {\n";
				if (!marshal("*" + marshaled_variable(), node))
					return false;

				stream() << "}\n\n";

				return true;
			}

			bool visit_null_terminated_array(null_terminated_array& node)
			{
				stream() << "int i;\n";
				stream() << node.element->c_specifier << "* array = " << marshaled_variable() << ";\n";
				stream() << "for (i = 0; array[i]; ++i) {\n";
				stream() << "\t" << node.element->c_specifier << "* element = &array[i];\n";
				if (!marshal("element", node))
					return false;

				stream() << "}\n\n";

				return true;
			}

			bool visit_rpc_ptr(rpc_ptr& node)
			{
				stream() << "*" << marshaled_variable() << " = glue_unmarshal_rpc_ptr(msg, " << node.definition->trmp_id
					<< ");\n";

				return true;
			}

			bool visit_projection(projection& node)
			{
				stream() << node.arg_unmarshal_visitor << "(msg, " << marshaled_variable() << ");\n";
				return true;
			}

		private:
			absl::string_view m_c_specifier {};
		};

		class arg_remarshal_walk : public marshal_walk<arg_remarshal_walk> {
		public:
			arg_remarshal_walk(std::ostream& os, absl::string_view holder, unsigned level) :
				marshal_walk {os, holder, level}
			{}
		};

		class arg_unremarshal_walk : public marshal_walk<arg_unremarshal_walk> {
		public:
			arg_unremarshal_walk(std::ostream& os, absl::string_view holder, unsigned level) :
				marshal_walk {os, holder, level}
			{}
		};

		class ret_marshal_walk : public marshal_walk<ret_marshal_walk> {
		public:
			ret_marshal_walk(std::ostream& os, absl::string_view holder, unsigned level) :
				marshal_walk {os, holder, level}
			{}
		};

		class ret_unmarshal_walk : public marshal_walk<ret_unmarshal_walk> {
		public:
			ret_unmarshal_walk(std::ostream& os, absl::string_view holder, unsigned level) :
				marshal_walk {os, holder, level}
			{}
		};

		/*
			Some explanation: C usually has two different ways of accessing variables, depnding on if they're a value
			or a pointer, i.e. foo_ptr->a_field vs. foo_ptr.a_field. To avoid unnecessary copying, and also because in
			come cases (arrays) copying the variable would be more complex than an assignment, we'd rather work with
			pointers in marshaling. But, arguments to functions start as values. I didn't want to maintain code paths
			to correctly track what "kind" of variable we're marshaling to access it correctly, so this function
			creates pointers to all the arguments of the RPC. That way the marshaling system can be written to only
			deal with pointers.
		*/
		auto generate_root_ptrs(rpc_def& rpc, std::ostream& os)
		{
			const auto n_args = rpc.arg_pgraphs.size();
			std::vector<std::string> roots(n_args);
			for (gsl::index i {}; i < n_args; ++i) {
				const auto name = rpc.arguments->at(i)->name;
				const auto specifier = rpc.arg_pgraphs.at(i)->c_specifier;
				const auto ptr_name = concat(name, "_ptr");
				os << "\t" << specifier << "* " << ptr_name << " = &" << name << ";\n";
				roots.at(i) = ptr_name;
			}

			// TODO: somehow return actual retval pointer name
			if (rpc.ret_pgraph) {
				os << "\t" << rpc.ret_pgraph->c_specifier << " ret;\n";
				os << "\t" << rpc.ret_pgraph->c_specifier << "* ret_ptr = &ret;\n";
			}

			os << "\t\n";

			return std::make_tuple(roots, "ret_ptr");
		}

		void generate_caller_glue(rpc_def& rpc, std::ostream& os)
		{
			os << "\tstruct fipc_message msg_buf = {0};\n";
			os << "\tstruct fipc_message *msg = &msg_buf;\n\n";

			const auto n_args = rpc.arg_pgraphs.size();
			const auto [roots, ret_root] = generate_root_ptrs(rpc, os);

			for (gsl::index i {}; i < n_args; ++i) {
				arg_marshal_walk arg_marshal {os, roots.at(i), 1}; // TODO: collect names
				arg_marshal.visit_value(*rpc.arg_pgraphs.at(i));
			}

			os << "\tvmfunc_wrapper(msg);\n\n";

			for (gsl::index i {}; i < n_args; ++i) {
				arg_unremarshal_walk arg_unremarshal {os, roots.at(i), 1};
				arg_unremarshal.visit_value(*rpc.arg_pgraphs.at(i));
			}

			if (rpc.ret_pgraph) {
				ret_unmarshal_walk ret_unmarshal {os, ret_root, 1};
				ret_unmarshal.visit_value(*rpc.ret_pgraph);
			}

			os << (rpc.ret_pgraph ? "\treturn 0;\n" : ""); 
		}

		void generate_callee_glue(rpc_def& rpc, std::ostream& os)
		{
			const auto n_args = rpc.arg_pgraphs.size();
			for (gsl::index i {}; i < n_args; ++i) {
				const auto& type = rpc.arg_pgraphs.at(i)->c_specifier;
				const auto name = rpc.arguments->at(i)->name;
				os << "\t" << type << " " << name << " = 0;\n";
			}

			const auto [roots, ret_root] = generate_root_ptrs(rpc, os);

			for (gsl::index i {}; i < n_args; ++i) {
				arg_unmarshal_walk arg_unmarshal {os, roots.at(i), 1};
				arg_unmarshal.visit_value(*rpc.arg_pgraphs.at(i));
			}

			if (rpc.kind == rpc_def_kind::direct) {
				os << "\t";
				if (rpc.ret_pgraph)
					os << "ret = ";

				os << rpc.name << "(" << rpc.params_string << ");\n\n";
			}

			for (gsl::index i {}; i < n_args; ++i) {
				arg_remarshal_walk arg_remarshal {os, roots.at(i), 1};
				arg_remarshal.visit_value(*rpc.arg_pgraphs.at(i));
			}

			if (rpc.ret_pgraph) {
				ret_marshal_walk ret_marshal {os, ret_root, 1};
				ret_marshal.visit_value(*rpc.ret_pgraph);
			}
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

		void generate_caller(rpc_vec_view rpcs)
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

		void generate_callee(rpc_vec_view rpcs)
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

		void generate_linker_script(rpc_vec_view rpcs)
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

		void create_function_strings(rpc_vec_view rpcs)
		{
			for (auto& rpc : rpcs) {
				if (rpc->ret_type)
					rpc->ret_string = rpc->ret_pgraph->c_specifier;
				else
					rpc->ret_string = "void";

				if (rpc->arguments) {
					bool is_first {true};
					auto& args_str = rpc->args_string;
					auto& param_str = rpc->params_string;
					for (gsl::index i {}; i < rpc->arguments->size(); ++i) {
						const auto& arg = rpc->arg_pgraphs.at(i);
						const auto name = rpc->arguments->at(i)->name;
						if (!is_first) {
							args_str += ", ";
							param_str += ", ";
						}

						append(args_str, arg->c_specifier, " ", name);
						param_str += name;
						is_first = false;
					}
				}
				else {
					rpc->args_string = "void"; // GCC doesn't like it if we just leave the arguments list empty
				}
			}
		}

		void generate_arg_marshal_visitor(std::ostream& file, projection& node)
		{
			file << "void " << node.arg_marshal_visitor
					<< "(\n\tstruct fipc_message* msg,\n\tstruct " << node.real_name << "* ptr)\n{\n";

				
			file << "}\n\n";
		}

		void generate_arg_unmarshal_visitor(std::ostream& file, projection& node)
		{
			file << "void " << node.arg_unmarshal_visitor
					<< "(\n\tstruct fipc_message* msg,\n\tstruct " << node.real_name << "* ptr)\n{\n";

				
			file << "}\n\n";
		}

		void generate_arg_remarshal_visitor(std::ostream& file, projection& node)
		{
			file << "void " << node.arg_remarshal_visitor
					<< "(\n\tstruct fipc_message* msg,\n\tstruct " << node.real_name << "* ptr)\n{\n";

				
			file << "}\n\n";
		}

		void generate_arg_unremarshal_visitor(std::ostream& file, projection& node)
		{
			file << "void " << node.arg_unremarshal_visitor
					<< "(\n\tstruct fipc_message* msg,\n\tstruct " << node.real_name << "* ptr)\n{\n";

				
			file << "}\n\n";
		}

		void generate_ret_marshal_visitor(std::ostream& file, projection& node)
		{
			file << "void " << node.ret_marshal_visitor
					<< "(\n\tstruct fipc_message* msg,\n\tstruct " << node.real_name << "* ptr)\n{\n";

				
			file << "}\n\n";
		}

		void generate_ret_unmarshal_visitor(std::ostream& file, projection& node)
		{
			file << "void " << node.ret_unmarshal_visitor
					<< "(\n\tstruct fipc_message* msg,\n\tstruct " << node.real_name << "* ptr)\n{\n";

				
			file << "}\n\n";
		}

		void generate_common_source(projection_vec_view projections)
		{
			std::ofstream file {"common.c"};
			file.exceptions(file.badbit | file.failbit);

			file << "#include <lcd_config/pre_hook.h>\n\n";
			file << "#include \"common.h\"\n\n";
			file << "#include <lcd_config/post_hook.h>\n\n";
			for (const auto& projection : projections) {
				generate_arg_marshal_visitor(file, *projection);
				generate_arg_unmarshal_visitor(file, *projection);
				generate_arg_remarshal_visitor(file, *projection);
				generate_arg_unremarshal_visitor(file, *projection);
				generate_ret_marshal_visitor(file, *projection);
				generate_ret_unmarshal_visitor(file, *projection);
			}
		}

		class projection_collection_walk : public pgraph_walk<projection_collection_walk> {
		public:
			projection_collection_walk(projection_vec& projections) :
				m_projections {projections}
			{}

			bool visit_projection(projection& node)
			{
				const auto last = m_projections.end();
				if (std::find(m_projections.begin(), last, &node) == last)
					m_projections.emplace_back(&node);

				return true;
			}

		private:
			projection_vec& m_projections;
		};

		projection_vec get_projections(rpc_vec_view rpcs)
		{
			projection_vec projs {};
			projection_collection_walk walk {projs};
			for (const auto& rpc : rpcs) {
				if (rpc->ret_pgraph)
					walk.visit_value(*rpc->ret_pgraph);
				
				for (const auto& arg : rpc->arg_pgraphs)
					walk.visit_value(*arg);
			}

			return projs;
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

	const auto projections = idlc::get_projections(rpcs);
	idlc::create_function_strings(rpcs);
	idlc::generate_common_header(rpcs, projections);
	idlc::generate_common_source(projections);
	idlc::generate_caller(rpcs);
	idlc::generate_callee(rpcs);
	idlc::generate_linker_script(rpcs);
}