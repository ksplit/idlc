#include "ast_dump.h"

#include <cassert>
#include <iostream>

#include "walk.h"
#include "ast.h"
#include "../tab_over.h"

namespace idlc::ast {
	namespace {
		class null_walk : public ast_walk<null_walk> {
		public:
			bool visit_file(file& node)
			{
				tab_over(std::cout << "[debug]  ", level_) << "file" << std::endl;
				++level_;
				traverse(*this, node);
				--level_;
				return true;
			}

			bool visit_module_def(module_def& node)
			{
				tab_over(std::cout << "[debug]  ", level_) << "module_def" << std::endl;
				++level_;
				traverse(*this, node);
				--level_;
				return true;
			}

			bool visit_driver_file(driver_file& node)
			{
				tab_over(std::cout << "[debug]  ", level_) << "driver_file" << std::endl;
				++level_;
				traverse(*this, node);
				--level_;
				return true;
			}

			bool visit_driver_def(driver_def& node)
			{
				tab_over(std::cout << "[debug]  ", level_) << "driver_def" << std::endl;
				++level_;
				traverse(*this, node);
				--level_;
				return true;
			}

			bool visit_module_item(module_item& node)
			{
				tab_over(std::cout << "[debug]  ", level_) << "module_item" << std::endl;
				++level_;
				traverse(*this, node);
				--level_;
				return true;
			}

			bool visit_proj_def(proj_def& node)
			{
				tab_over(std::cout << "[debug]  ", level_) << "union_proj_def" << std::endl;
				++level_;
				traverse(*this, node);
				--level_;
				return true;
			}

			bool visit_rpc_def(rpc_def& node)
			{
				tab_over(std::cout << "[debug]  ", level_) << "rpc_def" << std::endl;
				++level_;
				traverse(*this, node);
				--level_;
				return true;
			}

			bool visit_proj_field(proj_field& node)
			{
				tab_over(std::cout << "[debug]  ", level_) << "proj_field" << std::endl;
				++level_;
				traverse(*this, node);
				--level_;
				return true;
			}

			bool visit_var_decl(var_decl& node)
			{
				tab_over(std::cout << "[debug]  ", level_) << "var_decl" << std::endl;
				++level_;
				traverse(*this, node);
				--level_;
				return true;
			}

			bool visit_type_spec(type_spec& node)
			{
				tab_over(std::cout << "[debug]  ", level_) << "type_spec" << std::endl;
				++level_;
				traverse(*this, node);
				--level_;
				return true;
			}

			bool visit_indirection(indirection& node)
			{
				tab_over(std::cout << "[debug]  ", level_) << "indirection" << std::endl;
				++level_;
				traverse(*this, node);
				--level_;
				return true;
			}

			bool visit_type_stem(type_stem& node)
			{
				tab_over(std::cout << "[debug]  ", level_) << "type_stem" << std::endl;
				++level_;
				traverse(*this, node);
				--level_;
				return true;
			}

			bool visit_type_any_of(type_any_of& node)
			{
				tab_over(std::cout << "[debug]  ", level_) << "type_any_of" << std::endl;
				++level_;
				traverse(*this, node);
				--level_;
				return true;
			}

			bool visit_type_array(type_array& node)
			{
				tab_over(std::cout << "[debug]  ", level_) << "type_array" << std::endl;
				++level_;
				traverse(*this, node);
				--level_;
				return true;
			}

			bool visit_array_size(array_size& node)
			{
				tab_over(std::cout << "[debug]  ", level_) << "array_size" << std::endl;
				++level_;
				traverse(*this, node);
				--level_;
				return true;
			}

			bool visit_type_proj(type_proj& node)
			{
				tab_over(std::cout << "[debug]  ", level_) << "type_proj" << std::endl;
				++level_;
				traverse(*this, node);
				--level_;
				return true;
			}

			bool visit_type_rpc(type_rpc& node)
			{
				tab_over(std::cout << "[debug]  ", level_) << "type_rpc" << std::endl;
				++level_;
				traverse(*this, node);
				--level_;
				return true;
			}

			bool visit_naked_proj_decl(naked_proj_decl& node)
			{
				tab_over(std::cout << "[debug]  ", level_) << "naked_proj_decl" << std::endl;
				++level_;
				traverse(*this, node);
				--level_;
				return true;
			}

			bool visit_rpc_item(rpc_item& node)
			{
				tab_over(std::cout << "[debug]  ", level_) << "rpc_item" << std::endl;
				++level_;
				traverse(*this, node);
				--level_;
				return true;
			}

		private:
			unsigned level_ {};
		};
	}
}

void idlc::ast::dump_ast(file& root)
{
	null_walk walk {};
	const auto succeeded = walk.visit_file(root);
	assert(succeeded);
}
