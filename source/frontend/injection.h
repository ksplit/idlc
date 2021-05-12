#pragma once

#include "../ast/ast_walk.h"
#include "../ast/ast.h"
#include "../parser/parse_globals.h"

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
				// HACK: extreme hackery abounds here: every global generates a corresponding `__global_init_*` rpc, and we abuse the scoping bug
				if (!def->type->indirs.empty())
					def->type->indirs.back()->attrs->kind |= annotation_kind::unused;

				node.items->emplace_back(
					std::make_shared<module_item>(
						std::make_shared<rpc_def>(
							def->type,
							parser::idents.intern(concat("__global_init_var_", def->name)),
							nullptr,
							nullptr,
							rpc_def_kind::direct
						)
					)
				);
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
