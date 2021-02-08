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
				indent(std::cout, indent_level_) << "module_def" << std::endl;
				++indent_level_;
				traverse(*this, node);
				--indent_level_;
				return true;
			}

			bool visit_driver_file(driver_file& node)
			{
				indent(std::cout, indent_level_) << "driver_file" << std::endl;
				++indent_level_;
				traverse(*this, node);
				--indent_level_;
				return true;
			}

			bool visit_driver_def(driver_def& node)
			{
				indent(std::cout, indent_level_) << "driver_def" << std::endl;
				++indent_level_;
				traverse(*this, node);
				--indent_level_;
				return true;
			}

			bool visit_proj_def(proj_def& node)
			{
				indent(std::cout, indent_level_) << "proj_def" << std::endl;
				++indent_level_;
				traverse(*this, node);
				--indent_level_;
				return true;
			}

			bool visit_rpc_def(rpc_def& node)
			{
				indent(std::cout, indent_level_) << "rpc_def" << std::endl;
				++indent_level_;
				traverse(*this, node);
				--indent_level_;
				return true;
			}

			bool visit_var_decl(var_decl& node)
			{
				indent(std::cout, indent_level_) << "var_decl" << std::endl;
				++indent_level_;
				traverse(*this, node);
				--indent_level_;
				return true;
			}

			bool visit_type_spec(type_spec& node)
			{
				indent(std::cout, indent_level_) << "type_spec" << std::endl;
				++indent_level_;
				traverse(*this, node);
				--indent_level_;
				return true;
			}

			bool visit_indirection(indirection& node)
			{
				indent(std::cout, indent_level_) << "indirection" << std::endl;
				++indent_level_;
				traverse(*this, node);
				--indent_level_;
				return true;
			}

			bool visit_type_any_of(type_any_of& node)
			{
				indent(std::cout, indent_level_) << "type_any_of" << std::endl;
				++indent_level_;
				traverse(*this, node);
				--indent_level_;
				return true;
			}

			bool visit_type_array(type_array& node)
			{
				indent(std::cout, indent_level_) << "type_array" << std::endl;
				++indent_level_;
				traverse(*this, node);
				--indent_level_;
				return true;
			}

			bool visit_array_size(array_size& node)
			{
				indent(std::cout, indent_level_) << "array_size" << std::endl;
				++indent_level_;
				traverse(*this, node);
				--indent_level_;
				return true;
			}

			bool visit_type_proj(type_proj& node)
			{
				indent(std::cout, indent_level_) << "type_proj" << std::endl;
				++indent_level_;
				traverse(*this, node);
				--indent_level_;
				return true;
			}

			bool visit_type_rpc(type_rpc& node)
			{
				indent(std::cout, indent_level_) << "type_rpc" << std::endl;
				++indent_level_;
				traverse(*this, node);
				--indent_level_;
				return true;
			}

			bool visit_naked_proj_decl(naked_proj_decl& node)
			{
				indent(std::cout, indent_level_) << "naked_proj_decl" << std::endl;
				++indent_level_;
				traverse(*this, node);
				--indent_level_;
				return true;
			}

		private:
			unsigned indent_level_ {};
		};
	}
}

void idlc::dump_ast(file& root)
{
	null_walk walk {};
	const auto succeeded = walk.visit_file(root);
	assert(succeeded);
}
