#include <cassert>
#include <iostream>
#include <vector>

#include "../ast/ast.h"
#include "../ast/walk.h"
#include "../tag_types.h"
#include "pgraph_walk.h"
#include "pgraph.h"

namespace idlc::sema {
	namespace {
		class rpc_collector : public ast::ast_walk<rpc_collector> {
		public:
			bool visit_rpc_def(ast::rpc_def& node)
			{
				defs_.emplace_back(&node);
				return true;
			}

			auto get()
			{
				return defs_;
			}

		private:
			std::vector<ast::rpc_def*> defs_ {};
		};

		// TODO: implement error checking, currently focused on generating defaults
		class propagation_walk : public pgraph_walk<propagation_walk> {
		public:
			propagation_walk(annotation default_with) : default_with_ {default_with}
			{
				assert(is_clear(default_with & annotation::is_ptr));
			}

			bool visit_data_field(data_field& node)
			{
				// The core logic of propagating a top-level value annotation until one is found, and continuing
				if (!is_clear(node.value_annots & annotation::is_set)) {
					std::cout << "[debug] Continuing with new annotation default\n";
					default_with_ = node.value_annots;				
				}
				else {
					std::cout << "[debug] Applying annotation default\n";
					node.value_annots = default_with_;
				}

				return traverse(*this, node);
			}

			bool visit_pointer(pointer& node)
			{
				if (is_clear(node.pointer_annots & annotation::is_set)) {
					std::cout << "[debug] Pointer annotation inferred from default\n";
					if (default_with_ == annotation::in)
						node.pointer_annots = annotation::bind_callee;
					else if (default_with_ == annotation::out)
						node.pointer_annots = annotation::bind_caller;
					else if (default_with_ == (annotation::in | annotation::out)) // TODO: does this make sense?
						node.pointer_annots = (annotation::bind_callee | annotation::bind_caller);
				}

				return traverse(*this, node);
			}

			bool visit_projection(projection& node)
			{
				std::cout << "[debug] Struck projection\n";
				return true; // NOTE: do *not* traverse projection nodes directly
			}

		private:
			annotation default_with_ {};
		};
	
		auto get_rpcs(ast::file& root)
		{
			rpc_collector walk {};
			const auto succeeded = walk.visit_file(root);
			assert(succeeded);
			return walk.get();
		}

		void propagate_defaults(data_field& node, annotation default_with)
		{
			propagation_walk ret_walk {default_with};
			const auto succeeded = ret_walk.visit_data_field(node);
			assert(succeeded);
		}
	}

	void propagate_defaults(ast::file& root)
	{
		const auto rpcs = get_rpcs(root);
		for (const auto& rpc : rpcs) {
			std::cout << "[debug] Propagating for RPC \"" << rpc->name << "\"\n";
			gsl::span<data_field*> pgraphs {rpc->pgraphs};
			if (rpc->ret_type) {
				std::cout << "[debug] Processing return type\n";
				propagate_defaults(*pgraphs[0], annotation::out);
				pgraphs = pgraphs.subspan(1);
			}

			for (const auto& pgraph : pgraphs) {
				std::cout << "[debug] Processing argument type\n";
				propagate_defaults(*pgraph, annotation::in);
			}
		}
	}
}
