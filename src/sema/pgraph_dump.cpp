#include "pgraph_dump.h"

#include "pgraph_walk.h"
#include "pgraph.h"
#include "../tab_over.h"

namespace idlc::sema {
	namespace {
		class null_walk : public pgraph_walk<null_walk> {
		public:
			bool visit_data_field(data_field& node)
			{
				tab_over(std::cout << "[debug]", level_) << "data_field\n";
				++level_;
				if (!traverse(*this, node))
					return false;

				--level_;

				return true;
			}

			bool visit_field_type(field_type& node)
			{
				tab_over(std::cout << "[debug]", level_) << "field_type\n";
				++level_;
				if (!traverse(*this, node))
					return false;

				--level_;

				return true;
			}

			bool visit_projection(projection& node)
			{
				if (!traverse.visited(&node))
					tab_over(std::cout << "[debug]", level_) << "projection\n";
				else
					tab_over(std::cout << "[debug]", level_) << "projection (skipped)\n";

				++level_;
				if (!traverse(*this, node))
					return false;

				--level_;

				return true;
			}

			bool visit_dyn_array(dyn_array& node)
			{
				tab_over(std::cout << "[debug]", level_) << "dyn_array\n";
				++level_;
				if (!traverse(*this, node))
					return false;

				--level_;

				return true;
			}

			bool visit_null_terminated_array(null_terminated_array& node)
			{
				tab_over(std::cout << "[debug]", level_) << "null_terminated_array\n";
				++level_;
				if (!traverse(*this, node))
					return false;

				--level_;

				return true;
			}

			bool visit_pointer(pointer& node)
			{
				tab_over(std::cout << "[debug]", level_) << "pointer\n";
				++level_;
				if (!traverse(*this, node))
					return false;

				--level_;

				return true;
			}

			bool visit_rpc_ptr(rpc_ptr& node)
			{
				tab_over(std::cout << "[debug]", level_) << "rpc_ptr\n";
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
