#include "pgraph_dump.h"

#include <cassert>
#include <iostream>

#include "pgraph_walk.h"
#include "pgraph.h"
#include "../tab_over.h"

namespace idlc::sema {
	namespace {
		class null_walk : public pgraph_walk<null_walk> {
		public:
			bool visit_projection(projection& node)
			{
				if (!traverse.visited(&node))
					tab_over(std::cout, level_) << "projection\n";
				else
					tab_over(std::cout, level_) << "projection (skipped)\n";

				++level_;
				if (!traverse(*this, node))
					return false;

				--level_;

				return true;
			}

			bool visit_dyn_array(dyn_array& node)
			{
				tab_over(std::cout, level_) << "dyn_array\n";
				++level_;
				if (!traverse(*this, node))
					return false;

				--level_;

				return true;
			}

			bool visit_null_terminated_array(null_terminated_array& node)
			{
				tab_over(std::cout, level_) << "null_terminated_array\n";
				++level_;
				if (!traverse(*this, node))
					return false;

				--level_;

				return true;
			}

			bool visit_pointer(pointer& node)
			{
				tab_over(std::cout, level_) << "pointer\n";
				++level_;
				if (!traverse(*this, node))
					return false;

				--level_;

				return true;
			}

			bool visit_rpc_ptr(rpc_ptr& node)
			{
				tab_over(std::cout, level_) << "rpc_ptr\n";
				return true;
			}

			bool visit_primitive(primitive node)
			{
				tab_over(std::cout, level_) << "primitive\n";
				return true;
			}

			bool visit_proj_def(ast::proj_def& node)
			{
				tab_over(std::cout, level_) << "proj_def\n";
				return true;
			}

		private:
			unsigned level_ {1};
		};
	}
}

void idlc::sema::dump_pgraph(data_field& root)
{
	null_walk walk {};
	const auto succeeded = walk.visit_data_field(root);
	assert(succeeded);
}
