#include "ast_dump.h"

#include <cassert>
#include <iostream>

#include "ast_walk.h"
#include "ast.h"
#include "../utility.h"

namespace idlc {
	namespace {
		class null_walk : public ast_walk<null_walk> {
		public:
			bool visit_module_def(module_def& node)
			{
				indent(std::cout, m_indent_level) << "module_def" << std::endl;
				++m_indent_level;
				traverse(*this, node);
				--m_indent_level;
				return true;
			}

			bool visit_driver_file(driver_file& node)
			{
				indent(std::cout, m_indent_level) << "driver_file" << std::endl;
				++m_indent_level;
				traverse(*this, node);
				--m_indent_level;
				return true;
			}

			bool visit_driver_def(driver_def& node)
			{
				indent(std::cout, m_indent_level) << "driver_def" << std::endl;
				++m_indent_level;
				traverse(*this, node);
				--m_indent_level;
				return true;
			}

			bool visit_proj_def(proj_def& node)
			{
				indent(std::cout, m_indent_level) << "proj_def" << std::endl;
				++m_indent_level;
				traverse(*this, node);
				--m_indent_level;
				return true;
			}

			bool visit_rpc_def(rpc_def& node)
			{
				indent(std::cout, m_indent_level) << "rpc_def" << std::endl;
				++m_indent_level;
				traverse(*this, node);
				--m_indent_level;
				return true;
			}

			bool visit_var_decl(var_decl& node)
			{
				indent(std::cout, m_indent_level) << "var_decl" << std::endl;
				++m_indent_level;
				traverse(*this, node);
				--m_indent_level;
				return true;
			}

			bool visit_type_spec(type_spec& node)
			{
				indent(std::cout, m_indent_level) << "type_spec" << std::endl;
				++m_indent_level;
				traverse(*this, node);
				--m_indent_level;
				return true;
			}

			bool visit_indirection(indirection& node)
			{
				indent(std::cout, m_indent_level) << "indirection" << std::endl;
				++m_indent_level;
				traverse(*this, node);
				--m_indent_level;
				return true;
			}

			bool visit_type_any_of(type_any_of& node)
			{
				indent(std::cout, m_indent_level) << "type_any_of" << std::endl;
				++m_indent_level;
				traverse(*this, node);
				--m_indent_level;
				return true;
			}

			bool visit_type_array(type_array& node)
			{
				indent(std::cout, m_indent_level) << "type_array" << std::endl;
				++m_indent_level;
				traverse(*this, node);
				--m_indent_level;
				return true;
			}

			bool visit_array_size(array_size& node)
			{
				indent(std::cout, m_indent_level) << "array_size" << std::endl;
				++m_indent_level;
				traverse(*this, node);
				--m_indent_level;
				return true;
			}

			bool visit_type_proj(type_proj& node)
			{
				indent(std::cout, m_indent_level) << "type_proj" << std::endl;
				++m_indent_level;
				traverse(*this, node);
				--m_indent_level;
				return true;
			}

			bool visit_type_rpc(type_rpc& node)
			{
				indent(std::cout, m_indent_level) << "type_rpc" << std::endl;
				++m_indent_level;
				traverse(*this, node);
				--m_indent_level;
				return true;
			}

			bool visit_naked_proj_decl(naked_proj_decl& node)
			{
				indent(std::cout, m_indent_level) << "naked_proj_decl" << std::endl;
				++m_indent_level;
				traverse(*this, node);
				--m_indent_level;
				return true;
			}

			bool visit_type_none(type_none)
			{
				indent(std::cout, m_indent_level) << "type_none" << std::endl;
				return true;
			}

		private:
			unsigned m_indent_level {};
		};
	}
}

void idlc::dump_ast(file& root)
{
	null_walk walk {};
	const auto succeeded = walk.visit_file(root);
	assert(succeeded);
}
