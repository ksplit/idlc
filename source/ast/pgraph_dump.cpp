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
					indent(std::cout, m_indent_level) << "projection\n";
				else
					indent(std::cout, m_indent_level) << "projection (skipped)\n";

				++m_indent_level;
				if (!traverse(*this, node))
					return false;

				--m_indent_level;

				return true;
			}

			bool visit_dyn_array(dyn_array& node)
			{
				indent(std::cout, m_indent_level) << "dyn_array\n";
				++m_indent_level;
				if (!traverse(*this, node))
					return false;

				--m_indent_level;

				return true;
			}

			bool visit_static_array(static_array& node)
			{
				indent(std::cout, m_indent_level) << "static_array\n";
				++m_indent_level;
				if (!traverse(*this, node))
					return false;

				--m_indent_level;

				return true;
			}

			bool visit_null_terminated_array(null_terminated_array& node)
			{
				indent(std::cout, m_indent_level) << "null_terminated_array\n";
				++m_indent_level;
				if (!traverse(*this, node))
					return false;

				--m_indent_level;

				return true;
			}

			bool visit_pointer(pointer& node)
			{
				indent(std::cout, m_indent_level) << "pointer\n";
				++m_indent_level;
				if (!traverse(*this, node))
					return false;

				--m_indent_level;

				return true;
			}

			bool visit_rpc_ptr(rpc_ptr& node)
			{
				indent(std::cout, m_indent_level) << "rpc_ptr\n";
				return true;
			}

			bool visit_primitive(primitive node)
			{
				indent(std::cout, m_indent_level) << "primitive\n";
				return true;
			}

			bool visit_proj_def(proj_def& node)
			{
				indent(std::cout, m_indent_level) << "proj_def\n";
				return true;
			}

		private:
			unsigned m_indent_level {1};
		};
	}
}

void idlc::dump_pgraph(value& root)
{
	null_walk walk {};
	const auto succeeded = walk.visit_value(root);
	assert(succeeded);
}
