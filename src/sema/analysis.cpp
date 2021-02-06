#include "analysis.h"

#include <cassert>
#include <exception>
#include <memory>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "../parser/string_heap.h"
#include "../ast/ast.h"
#include "../ast/walk.h"
#include "../tag_types.h"
#include "pgraph.h"
#include "pgraph_walk.h"
#include "pgraph_dump.h"

// These functions translate the AST into "high-level" pgraphs (before defaults propagation, projection linking)

namespace idlc::sema {
	namespace {
		// Remember: field_type is quite slim, only a tag value and a pointer (16 bytes total)

		field_type generate_string_type(bool is_const);
		field_type generate_stem_type(const ast::type_stem& node, bool is_const);
		field_type generate_array_type(const ast::type_array& node, bool is_const);
		node_ptr<data_field> generate_data_field(const ast::type_spec& node);

		field_type generate_string_type(bool is_const)
		{
			return std::make_unique<null_terminated_array>(
				std::make_unique<data_field>(primitive::ty_char, ast::annotation::use_default),
				is_const
			);
		}

		field_type generate_stem_type(const ast::type_stem& node, bool is_const)
		{
			const auto visit = [is_const](auto&& item) -> field_type
			{
				using type = std::decay_t<decltype(item)>;
				if constexpr (std::is_same_v<type, ast::type_primitive>) {
					return item;
				}
				else if constexpr (std::is_same_v<type, ast::type_string>) {
					return generate_string_type(is_const);
				}
				else if constexpr (std::is_same_v<type, ast::node_ref<ast::type_array>>) {
					return generate_array_type(*item, is_const);
				}
				else if constexpr (std::is_same_v<type, ast::node_ref<ast::type_proj>>) {
					return item->definition;
				}
				else if constexpr (std::is_same_v<type, ast::node_ref<ast::type_rpc>>) {
					// std::cout << "[debug] generating dummy pgraph node for RPC pointer \"" << item->name << "\"\n";
					return std::make_unique<rpc_ptr>(item.get().get()->definition);
				}

				std::terminate();
			};

			return std::visit(visit, node);
		}

		field_type generate_array_type(const ast::type_array& node, bool is_const)
		{
			const auto visit = [&node, is_const](auto&& item) -> field_type
			{
				using type = std::decay_t<decltype(item)>;
				if constexpr (std::is_same_v<type, ast::tok_kw_null>) {
					return std::make_unique<null_terminated_array>(
						generate_data_field(*node.element),
						is_const
					);
				}
				else if constexpr (std::is_same_v<type, unsigned>) {
					std::cout << "static-sized arrays are not yet implemented\n";
					std::terminate();
				}
				else if constexpr (std::is_same_v<type, ident>) {
					return std::make_unique<dyn_array>(
						generate_data_field(*node.element),
						item,
						is_const
					);
				}

				std::terminate();
			};

			return std::visit(visit, *node.size);
		}

		auto generate_field(ast::proj_field& node)
		{
			const auto visit = [](auto&& item) -> std::pair<ident, node_ptr<data_field>>
			{
				using type = std::decay_t<decltype(item)>;
				if constexpr (std::is_same_v<type, ast::node_ref<ast::naked_proj_decl>>) {
					std::cout << "Naked projections are not yet implemented\n";
					std::cout << "Unknown how to proceed, aborting\n";
					std::terminate();
				}
				else if constexpr (std::is_same_v<type, ast::node_ref<ast::var_decl>>) {
					// std::cout << "[pgraph] building var_decl data_field for \"" << item->name << "\"\n";
					return {item->name, generate_data_field(*item->type)};
				}

				std::terminate();
			};

			return std::visit(visit, node);
		}

		auto generate_union_dummy(ast::proj_def& def, const std::string& name)
		{
			std::cout << "Union projections are not yet implemented\n";
			std::cout << "Will implement as an empty struct projection \"" << def.name << "\"\n";
			return std::make_shared<projection>(def.type, std::move(name));
		}

		auto generate_empty_struct(ast::proj_def& def, const std::string& name)
		{
			// std::cout << "[pgraph] generating empty pgraph projection for \"" << def.name << "\"\n";
			return std::make_shared<projection>(def.type, std::move(name));
		}

		auto generate_struct(ast::proj_def& def, const std::string& name)
		{
			const auto& field_nodes = *def.fields;
			decltype(projection::fields) fields {};
			fields.reserve(field_nodes.size());
			for (const auto& field : field_nodes) {
				auto pgraph = generate_field(*field);
				// std::cout << "[pgraph] completed field \"" << pgraph.first << "\"\n";
				fields.emplace_back(std::move(pgraph));
			}

			// std::cout << "[pgraph] finished \"" << def.name << "\"\n";

			return std::make_shared<projection>(def.type, std::move(name), std::move(fields));
		}

		class type_walk : public ast::ast_walk<type_walk> {
		public:
			bool visit_rpc_def(ast::rpc_def& node)
			{
				if (node.ret_type)
					node.ret_pgraph = insert_field(*node.ret_type);

				if (node.arguments) {
					for (auto& argument : *node.arguments)
						node.arg_pgraphs.emplace_back(insert_field(*argument->type));
				}

				return ast::traverse(*this, node);
			}

			// TODO: support naked projections

			// FIXME: this is probably a hack, thanks to the unclean split between the AST and the assoicated data
			// structures
			auto get_pgraph_owner()
			{
				return std::move(store_);
			}

		private:
			std::vector<node_ptr<data_field>> store_ {};

			data_field* insert_field(const ast::type_spec& node)
			{
				auto pgraph_node = generate_data_field(node);
				const auto ptr = pgraph_node.get();
				store_.emplace_back(std::move(pgraph_node));
				return ptr;
			}
		};

		node_ptr<data_field> generate_data_field(const ast::type_spec& node)
		{
			// TODO: finish const handling
			auto type = generate_stem_type(*node.stem, node.is_const);
			for (const auto& ptr_node : node.indirs) {
				const auto annots = ptr_node->attrs;
				const auto ptr_annots = annots & ast::annotation::is_ptr;
				const auto val_annots = annots & ast::annotation::is_val;
				auto field = std::make_unique<data_field>(std::move(type), val_annots);
				type = std::make_unique<pointer>(std::move(field), ptr_annots, ptr_node->is_const);
			}

			assert((node.attrs & ast::annotation::is_val) == node.attrs);
			auto field = std::make_unique<data_field>(std::move(type), node.attrs);

			return std::move(field);
		}

		// The references in the AST are *not* owners, this vector is
		std::vector<node_ptr<data_field>> generate_pgraph_roots(rpc_table_ref rpcs)
		{
			type_walk walk {};
			for (const auto& rpc : rpcs) {
				const auto succeeded = walk.visit_rpc_def(*rpc);
				assert(succeeded);
			}

			return walk.get_pgraph_owner();
		}

		// TODO: is it necessary to detect if a projection self-references by value?
		std::shared_ptr<idlc::sema::projection> generate_projection(
			ast::proj_def& node,
			const std::string& name)
		{
			auto pgraph_node = [&node, &name] {
				if (!node.fields)
					return generate_empty_struct(node, std::move(name));
				else if (node.kind == ast::proj_def_kind::union_kind)
					return generate_union_dummy(node, std::move(name));
				else
					return generate_struct(node, std::move(name));
			}();

			return std::move(pgraph_node);
		}

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

		class type_string_walk : public pgraph_walk<type_string_walk> {
		public:
			auto get() const
			{
				return string_;
			}

			bool visit_data_field(data_field& node)
			{
				if (!traverse(*this, node))
					return false;

				node.type_string = string_;		
				return true;
			}

			bool visit_projection(projection& node)
			{
				string_ = "struct ";
				string_ += node.real_name;				
				return true;
			}

			bool visit_pointer(pointer& node)
			{
				if (!traverse(*this, node))
					return false;
				
				string_ += "*";
				return true;
			}

			bool visit_rpc_ptr(rpc_ptr& node)
			{
				string_ += node.definition->typedef_id;
				return true;
			}

			bool visit_primitive(primitive node)
			{
				switch (node) {
				case primitive::ty_bool:
					string_ = "bool";
					break;

				case primitive::ty_char:
					string_ = "char";
					break;

				case primitive::ty_schar:
					string_ = "signed char";
					break;

				case primitive::ty_uchar:
					string_ = "unsigned char";
					break;

				case primitive::ty_short:
					string_ = "short";
					break;

				case primitive::ty_ushort:
					string_ = "unsigned short";
					break;

				case primitive::ty_int:
					string_ = "int";
					break;

				case primitive::ty_uint:
					string_ = "unsigned int";
					break;

				case primitive::ty_long:
					string_ = "long";
					break;

				case primitive::ty_ulong:
					string_ = "unsigned long";
					break;

				case primitive::ty_llong:
					string_ = "long long";
					break;

				case primitive::ty_ullong:
					string_ = "unsigned long long";
					break;

				default:
					std::cout << "Unhandled primtive was " << static_cast<std::uintptr_t>(node) << "\n";
					assert(false);
				}

				return true;
			}

		private:
			std::string string_ {};
		};

		// NOTE: this *does not* walk into projections, and should only be applied to
		// variable types (fields or arguments) and return types, as done in the lowering pass
		void assign_type_strings(data_field& node)
		{
			type_string_walk type_walk {};
			const auto succeeded = type_walk.visit_data_field(node);
			assert(succeeded);
			std::cout << "Created type string: \"" << type_walk.get() << "\"\n";	
			dump_pgraph(node);
		}

		bool lower(data_field& node, annotation default_with)
		{
			lowering_walk lowerer {default_with};
			return lowerer.visit_data_field(node);
		}

		field_type lower(ast::proj_def& node, std::shared_ptr<projection>& cached, annotation default_with)
		{
			if (cached) {
				std::cout << "Found a prebuilt\n";
				// NOTE: It's important that we don't try and modify this, as it could be partial
				return cached;
			}
			
			std::string lowered_name {node.scoped_name};
			switch (default_with) {
			case annotation::in:
				lowered_name += "_in";
				break;

			case annotation::out:
				lowered_name += "_out";
				break;

			case annotation::in | annotation::out:
				lowered_name += "_in_out";
				break;

			default:
				assert(false);
			}

			auto pgraph = generate_projection(node, lowered_name);
			cached = pgraph;

			lowering_walk walk {default_with};
			if (!walk.visit_projection(*pgraph)) {
				std::cout << "Error: could not instantiate projection in this context\n";
				std::terminate(); // TODO: don't like the std::terminate here
			}

			// TODO: should this really be here?
			// We make sure we perform type string assignment *after* the projection has been fully lowered
			for (const auto& [name, field] : pgraph->fields) {
				std::cout << "Creating type string for \"" << name << "\"\n";
				assign_type_strings(*field);
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

		bool lower(ast::rpc_def& rpc)
		{
			std::cout << "Propagating for RPC \"" << rpc.name << "\"\n";
			if (rpc.ret_type) {
				std::cout << "Processing return type\n";
				if (!lower(*rpc.ret_pgraph, annotation::out))
					return false;
				
				assign_type_strings(*rpc.ret_pgraph);
			}

			for (const auto& pgraph : rpc.arg_pgraphs) {
				std::cout << "Processing argument type\n";
				if (!lower(*pgraph, annotation::in))
					return false;

				assign_type_strings(*pgraph);
			}

			return true;
		}

		bool lower(gsl::span<const gsl::not_null<ast::rpc_def*>> rpcs)
		{
			for (const auto& rpc : rpcs) {
				if (!lower(*rpc))
					return false;

				std::cout << "Finished RPC\n";
			}

			return true;
		}

		using pgraph_root_table = std::vector<node_ptr<data_field>>;
	}
}

std::optional<idlc::sema::pgraph_root_table> idlc::sema::generate_pgraphs(rpc_table_ref rpcs)
{
	auto roots = generate_pgraph_roots(rpcs);
	if (!lower(rpcs))
		return std::nullopt;
	
	return std::make_optional(std::move(roots));
}

idlc::sema::rpc_table idlc::sema::get_rpcs(ast::file& root)
{
	rpc_collector walk {};
	const auto succeeded = walk.visit_file(root);
	assert(succeeded);
	return walk.get();
}
