#pragma once

#include "../ast/ast_walk.h"
#include "../ast/ast.h"

namespace idlc {
	class global_rpcs_walk : public ast_walk<global_rpcs_walk> {
	public:
		bool visit_global_def(global_def& node)
		{
			m_defs.emplace_back(&node);
			return true;
		}

		bool visit_module_def(module_def& node)
		{
			if (!traverse(*this, node))
				return false;

			for (const auto& def : m_defs) {
				// Deep-copy!
				// We iterate through its items, appropriately copy and re-annotate the projections
				// (I think shallow-copy)
				// and then inject these, along with any other necessary items (rpc ptrs) into the definition
			}

			m_defs.clear();

			return true;
		}

	private:
		std::vector<const global_def*> m_defs {};
	};

	void inject_global_rpcs(file& root)
	{
		global_rpcs_walk walk {};
		const auto result = walk.visit_file(root);
		assert(result);
	}
}
