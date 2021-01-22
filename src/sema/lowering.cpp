#include "lowering.h"

#include <cassert>
#include <iostream>
#include <vector>

#include "../ast/ast.h"
#include "../ast/walk.h"
#include "../tag_types.h"
#include "pgraph_walk.h"
#include "pgraph.h"
#include "pgraph_generation.h"

// These functions lower the pgraph IR by propagating defaults, linking projections as they are needed,
// and translating / lowering projections as-needed

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
			std::vector<gsl::not_null<ast::rpc_def*>> defs_ {};
		};

		bool lower(data_field& node, annotation default_with);
		field_type lower(ast::proj_def& node, annotation default_with);

		// TODO: implement error checking, currently focused on generating defaults
		class lowering_walk : public pgraph_walk<lowering_walk> {
		public:
			lowering_walk(annotation default_with) : default_with_ {default_with}
			{
				assert(is_clear(default_with & annotation::ptr_only));
			}

			bool visit_data_field(data_field& node)
			{
				// The core logic of propagating a top-level value annotation until one is found, and continuing
				if (!is_clear(node.value_annots & annotation::val_only)) {
					std::cout << "Continuing with new value annotation default 0x" << std::hex;
					std::cout << static_cast<std::uintptr_t>(node.value_annots) << std::dec << "\n";
					default_with_ = node.value_annots;				
				}
				else {
					std::cout << "Applying value annotation default\n";
					node.value_annots = default_with_;
				}

				return traverse(*this, node);
			}

			bool visit_pointer(pointer& node)
			{
				// Default if no pointer annotations are set
				// TODO: is_set bit no longer means what I thought it did!
				if (is_clear(node.pointer_annots & annotation::ptr_only)) {
					std::cout << "Pointer annotation inferred from value default\n";
					if (default_with_ == annotation::in)
						node.pointer_annots = annotation::bind_callee;
					else if (default_with_ == annotation::out)
						node.pointer_annots = annotation::bind_caller;
					else if (default_with_ == (annotation::in | annotation::out)) // TODO: does this make sense?
						node.pointer_annots = (annotation::bind_callee | annotation::bind_caller);
				}
				else {
					std::cout << "Pointer annotation previously set, ignoring\n";
				}

				return traverse(*this, node);
			}

			bool visit_projection(projection& node)
			{
				std::cout << "Entered projection with 0x" << std::hex;
				std::cout << static_cast<std::uintptr_t>(default_with_) << std::dec << "\n";
				for (auto& [name, field] : node.fields) {
					std::cout<< "Propagating for projection field \"" << name << "\"\n";
					if (!lower(*field, default_with_))
						return false;
				}

				std::cout << "Finished projection\n";

				return true; // NOTE: do *not* traverse projection nodes directly
			}

			bool visit_field_type(field_type& node)
			{
				const auto unlowered = std::get_if<gsl::not_null<ast::proj_def*>>(&node);
				if (!unlowered) {
					return traverse(*this, node); // FIXME: is this correct?
				}
				else {
					node = lower(**unlowered, default_with_);
					return true;
				}
			}

		private:
			annotation default_with_ {};
		};

		bool lower(data_field& node, annotation default_with)
		{
			lowering_walk ret_walk {default_with};
			return ret_walk.visit_data_field(node);
		}

		field_type lower(ast::proj_def& node, std::shared_ptr<projection>& cached, annotation default_with)
		{
			if (cached) {
				std::cout << "Found a prebuilt\n";
				// NOTE: It's important that we don't try and modify this, as it could be partial
				return cached;
			}
			
			auto pgraph = generate_projection(node);
			cached = pgraph;

			lowering_walk walk {default_with};
			if (!walk.visit_projection(*pgraph)) {
				std::cout << "Error: could not instantiate projection in this context\n";
				std::terminate(); // TODO: don't like the std::terminate here
			}

			return pgraph;
		}

		field_type lower(ast::proj_def& node, annotation default_with)
		{
			std::cout << "Non-lowered projection \"" << node.name << "\"\n";
			switch (default_with) {
			case annotation::in | annotation::out:
				std::cout << "Need \"in/out\" instance\n";
				return lower(node, node.in_out_proj, default_with);

			case annotation::in:
				std::cout << "Need \"in\" instance\n";
				return lower(node, node.in_proj, default_with);

			case annotation::out:
				std::cout << "Need \"out\" instance\n";
				return lower(node, node.out_proj, default_with);

			default:
				std::terminate();
			}
		}
	}

	std::vector<gsl::not_null<ast::rpc_def*>> get_rpcs(ast::file& root)
	{
		rpc_collector walk {};
		const auto succeeded = walk.visit_file(root);
		assert(succeeded);
		return walk.get();
	}

	void create_alternate_names(ast::rpc_def& rpc)
	{
		rpc.enum_id = "RPC_ID_";
		rpc.enum_id += rpc.name;
		rpc.callee_id = rpc.name;
		rpc.callee_id += "_callee";

		if (rpc.kind == ast::rpc_def_kind::indirect) {
			rpc.trmp_id = "trmp_";
			rpc.trmp_id += rpc.name;
			rpc.impl_id = "trmp_impl_";
			rpc.impl_id += rpc.name;
			rpc.typedef_id = "fptr_";
			rpc.typedef_id += rpc.name;
		}
	}

	bool lower(ast::rpc_def& rpc)
	{
		std::cout << "Propagating for RPC \"" << rpc.name << "\"\n";
		gsl::span<data_field*> pgraphs {rpc.pgraphs};
		if (rpc.ret_type) {
			std::cout << "Processing return type\n";
			if (!lower(*pgraphs[0], annotation::out))
				return false;
			
			pgraphs = pgraphs.subspan(1);
		}

		for (const auto& pgraph : pgraphs) {
			std::cout << "Processing argument type\n";
			if (!lower(*pgraph, annotation::in))
				return false;
		}

		return true;
	}

	bool lower(gsl::span<const gsl::not_null<ast::rpc_def*>> rpcs)
	{
		for (const auto& rpc : rpcs) {
			create_alternate_names(*rpc);
			if (!lower(*rpc))
				return false;

			std::cout << "Finished RPC\n";
		}

		return true;
	}
}
