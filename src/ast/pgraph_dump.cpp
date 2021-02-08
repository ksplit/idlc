#include "pgraph_dump.h"

#include <cassert>
#include <iostream>

#include "pgraph_walk.h"
#include "pgraph.h"
#include "../utility.h"

namespace idlc {
	namespace {
		class null_walk : public pgraph_walk<null_walk> {
		public:
			bool visit_projection(projection& node)
			{
				if (!traverse.visited(&node))
					indent(std::cout, indent_level_) << "projection\n";
				else
					indent(std::cout, indent_level_) << "projection (skipped)\n";

				++indent_level_;
				if (!traverse(*this, node))
					return false;

				--indent_level_;

				return true;
			}

			bool visit_dyn_array(dyn_array& node)
			{
				indent(std::cout, indent_level_) << "dyn_array\n";
				++indent_level_;
				if (!traverse(*this, node))
					return false;

				--indent_level_;

				return true;
			}

			bool visit_null_terminated_array(null_terminated_array& node)
			{
				indent(std::cout, indent_level_) << "null_terminated_array\n";
				++indent_level_;
				if (!traverse(*this, node))
					return false;

				--indent_level_;

				return true;
			}

			bool visit_pointer(pointer& node)
			{
				indent(std::cout, indent_level_) << "pointer\n";
				++indent_level_;
				if (!traverse(*this, node))
					return false;

				--indent_level_;

				return true;
			}

			bool visit_rpc_ptr(rpc_ptr& node)
			{
				indent(std::cout, indent_level_) << "rpc_ptr\n";
				return true;
			}

			bool visit_primitive(primitive node)
			{
				indent(std::cout, indent_level_) << "primitive\n";
				return true;
			}

			bool visit_proj_def(proj_def& node)
			{
				indent(std::cout, indent_level_) << "proj_def\n";
				return true;
			}

		private:
			unsigned indent_level_ {1};
		};
	}
}

void idlc::dump_pgraph(value& root)
{
	null_walk walk {};
	const auto succeeded = walk.visit_value(root);
	assert(succeeded);
}
