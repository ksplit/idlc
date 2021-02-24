#include "analysis.h"

#include <cassert>
#include <exception>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <absl/strings/string_view.h>

#include "../string_heap.h"
#include "../ast/ast.h"
#include "../ast/ast_walk.h"
#include "../ast/pgraph.h"
#include "../ast/pgraph_walk.h"
#include "../ast/pgraph_dump.h"

namespace idlc {
	namespace {
		// Remember: passed_type is quite slim, only a tag value and a pointer (16 bytes total)

		passed_type generate_string_type();
		passed_type generate_stem_type(const type_stem& node);
		passed_type generate_array_type(const type_array& node);
		node_ptr<value> generate_value(const type_spec& node);

		passed_type generate_string_type()
		{
			return std::make_unique<null_terminated_array>(
				std::make_unique<value>(primitive::ty_char, annotation::use_default, true) // TODO: assumes const
			);
		}

		passed_type generate_stem_type(const type_stem& node)
		{
			const auto visit = [](auto&& item) -> passed_type
			{
				using type = std::decay_t<decltype(item)>;
				if constexpr (std::is_same_v<type, type_primitive>) {
					return item;
				}
				else if constexpr (std::is_same_v<type, type_string>) {
					return generate_string_type();
				}
				else if constexpr (std::is_same_v<type, node_ref<type_array>>) {
					return generate_array_type(*item);
				}
				else if constexpr (std::is_same_v<type, node_ref<type_proj>>) {
					// Importantly, we defer the translation of projections, since we cannot know at this stage
					// what annotations it should be defaulted with
					return item->definition;
				}
				else if constexpr (std::is_same_v<type, node_ref<type_rpc>>) {
					return std::make_unique<rpc_ptr>(item.get().get()->definition);
				}

				std::terminate();
			};

			return std::visit(visit, node);
		}

		passed_type generate_array_type(const type_array& node)
		{
			const auto visit = [&node](auto&& item) -> passed_type
			{
				using type = std::decay_t<decltype(item)>;
				if constexpr (std::is_same_v<type, tok_kw_null>) {
					return std::make_unique<null_terminated_array>(generate_value(*node.element));
				}
				else if constexpr (std::is_same_v<type, unsigned>) {
					std::cout << "Warning: static-sized arrays are not yet implemented\n";
					std::terminate();
				}
				else if constexpr (std::is_same_v<type, ident>) {
					std::cout << "Warning: dynamic-sized arrays are not yet fully supported\n";
					return std::make_unique<dyn_array>(generate_value(*node.element), item);
				}

				std::terminate();
			};

			return std::visit(visit, *node.size);
		}

		auto generate_field(proj_field& node)
		{
			const auto visit = [](auto&& item) -> std::pair<ident, node_ptr<value>>
			{
				using type = std::decay_t<decltype(item)>;
				if constexpr (std::is_same_v<type, node_ref<naked_proj_decl>>) {
					std::cout << "Warning: Naked projections are not yet implemented\n";
					std::cout << "Warning: Unknown how to proceed, aborting\n";
					std::terminate();
				}
				else if constexpr (std::is_same_v<type, node_ref<var_decl>>) {
					return {item->name, generate_value(*item->type)};
				}

				std::terminate();
			};

			return std::visit(visit, node);
		}

		auto generate_union_dummy(proj_def& def, const std::string& name)
		{
			std::cout << "Warning: Union projections are not yet implemented\n";
			std::cout << "Warning: Will implement as an empty struct projection \"" << def.name << "\"\n";
			return std::make_shared<projection>(def.type, std::move(name));
		}

		auto generate_empty_struct(proj_def& def, const std::string& name)
		{
			return std::make_shared<projection>(def.type, std::move(name));
		}

		auto generate_struct(proj_def& def, const std::string& name)
		{
			const auto& field_nodes = *def.fields;
			decltype(projection::fields) fields {};
			fields.reserve(field_nodes.size());
			for (const auto& field : field_nodes) {
				auto pgraph = generate_field(*field);
				fields.emplace_back(std::move(pgraph));
			}

			return std::make_shared<projection>(def.type, std::move(name), std::move(fields));
		}

		node_ptr<value> generate_value(const type_spec& node)
		{
			auto type = generate_stem_type(*node.stem);
			bool is_const {node.is_const};
			for (const auto& ptr_node : node.indirs) {
				const auto annots = ptr_node->attrs;
				const auto ptr_annots = annots & annotation::is_ptr;
				const auto val_annots = annots & annotation::is_val;
				auto field = std::make_unique<value>(std::move(type), val_annots, is_const);
				type = std::make_unique<pointer>(std::move(field), ptr_annots);
				is_const = ptr_node->is_const;
			}

			assert((node.attrs & annotation::is_val) == node.attrs);
			auto field = std::make_unique<value>(std::move(type), node.attrs, is_const);

			return std::move(field);
		}

		class type_walk : public ast_walk<type_walk> {
		public:
			bool visit_rpc_def(rpc_def& node)
			{
				if (node.ret_type)
					node.ret_pgraph = generate_value(*node.ret_type);

				if (node.arguments) {
					for (auto& argument : *node.arguments)
						node.arg_pgraphs.emplace_back(generate_value(*argument->type));
				}

				return traverse(*this, node);
			}

			// TODO: support naked projections
		};

		void create_pgraphs_from_types(rpc_vec_view rpcs)
		{
			type_walk walk {};
			for (const auto& rpc : rpcs) {
				const auto succeeded = walk.visit_rpc_def(*rpc);
				assert(succeeded);
			}
		}

		// TODO: is it necessary to detect if a projection self-references by value?
		std::shared_ptr<idlc::projection> generate_projection(
			proj_def& node,
			const std::string& name)
		{
			auto pgraph_node = [&node, &name] {
				if (!node.fields)
					return generate_empty_struct(node, std::move(name));
				else if (node.kind == proj_def_kind::union_kind)
					return generate_union_dummy(node, std::move(name));
				else
					return generate_struct(node, std::move(name));
			}();

			return std::move(pgraph_node);
		}

		class rpc_collection_walk : public ast_walk<rpc_collection_walk> {
		public:
			bool visit_rpc_def(rpc_def& node)
			{
				m_defs.emplace_back(&node);
				return true;
			}

			auto& get()
			{
				return m_defs;
			}

		private:
			std::vector<gsl::not_null<rpc_def*>> m_defs {};
		};

		bool annotate_pgraph(value& node, annotation default_with);
		passed_type instantiate_projection(proj_def& node, annotation default_with);

		// TODO: implement error checking, currently focused on generating defaults
		class annotation_walk : public pgraph_walk<annotation_walk> {
		public:
			annotation_walk(annotation default_with) : m_default_with {default_with}
			{
				// We only support propagating a default value annotation
				assert(is_clear(default_with & annotation::ptr_only));
			}

			bool visit_value(value& node)
			{
				// The core logic of propagating a top-level value annotation until an explicit one is found, and
				// continuing
				if (!is_clear(node.value_annots & annotation::val_only)) {
					m_default_with = node.value_annots;				
				}
				else {
					node.value_annots = m_default_with;
				}

				return traverse(*this, node);
			}

			bool visit_pointer(pointer& node)
			{
				// Default if no pointer annotations are set
				// TODO: what are the correct defaults here?
				if (is_clear(node.pointer_annots & annotation::ptr_only)) {
					if (m_default_with == annotation::in)
						node.pointer_annots = annotation::bind_caller;
					else if (m_default_with == annotation::out)
						node.pointer_annots = annotation::bind_callee;
					else if (m_default_with == (annotation::in | annotation::out))
						node.pointer_annots = (annotation::bind_callee | annotation::bind_caller); // FIXME
				}
				else {
					// Annotation already set, ignore
				}

				return traverse(*this, node);
			}

			bool visit_projection(projection& node)
			{
				for (auto& [name, field] : node.fields) {
					if (!annotate_pgraph(*field, m_default_with))
						return false;
				}

				return true; // NOTE: do *not* traverse projection nodes directly
			}

			bool visit_passed_type(passed_type& node)
			{
				const auto unlowered = std::get_if<gsl::not_null<proj_def*>>(&node);
				if (!unlowered) {
					return traverse(*this, node);
				}
				else {
					node = instantiate_projection(**unlowered, m_default_with);
					return true;
				}
			}

		private:
			annotation m_default_with {};
		};

		bool annotate_pgraph(value& node, annotation default_with)
		{
			annotation_walk annotator {default_with};
			return annotator.visit_value(node);
		}

		std::string get_instance_name(absl::string_view base_name, annotation default_with)
		{
			// double underscore used to reduce odds of collisions
			std::string instance_name {base_name};
			switch (default_with) {
			case annotation::in:
				instance_name += "__in";
				break;

			case annotation::out:
				instance_name += "__out";
				break;

			case annotation::in | annotation::out:
				instance_name += "__io";
				break;

			default:
				std::terminate();
			}

			return instance_name;
		}

		passed_type instantiate_projection(proj_def& node, node_ptr<projection>& cached, annotation default_with)
		{
			if (cached) {
				// NOTE: It's important that we don't try and modify the cached copy, as it could be a partial one
				return cached;
			}
			
			const auto instance_name = get_instance_name(node.scoped_name, default_with);
			auto pgraph = generate_projection(node, instance_name);
			cached = pgraph;

			annotation_walk walk {default_with};
			if (!walk.visit_projection(*pgraph)) {
				std::cout << "Error: could not annotate projection in this context\n";
				std::terminate(); // TODO: don't like the std::terminate here
			}

			return pgraph;
		}

		passed_type instantiate_projection(proj_def& node, annotation default_with)
		{
			std::cout << "Debug: Non-lowered projection \"" << node.name << "\"\n";
			switch (default_with) {
			case annotation::in | annotation::out:
				std::cout << "Debug: Need \"in/out\" instance\n";
				return instantiate_projection(node, node.in_out_proj, default_with);

			case annotation::in:
				std::cout << "Debug: Need \"in\" instance\n";
				return instantiate_projection(node, node.in_proj, default_with);

			case annotation::out:
				std::cout << "Debug: Need \"out\" instance\n";
				return instantiate_projection(node, node.out_proj, default_with);

			default:
				std::terminate();
			}
		}

		bool annotate_pgraphs(rpc_def& rpc)
		{
			if (rpc.ret_type) {
				if (!annotate_pgraph(*rpc.ret_pgraph, annotation::out))
					return false;
			}

			for (const auto& pgraph : rpc.arg_pgraphs) {
				if (!annotate_pgraph(*pgraph, annotation::in))
					return false;
			}

			return true;
		}

		bool annotate_pgraphs(rpc_vec_view rpcs)
		{
			for (const auto& rpc : rpcs) {
				if (!annotate_pgraphs(*rpc))
					return false;
			}

			return true;
		}

		bool generate_pgraphs(rpc_vec_view rpcs)
		{
			create_pgraphs_from_types(rpcs);
			return annotate_pgraphs(rpcs);
		}
	}
}



std::optional<idlc::rpc_vec> idlc::generate_rpc_pgraphs(file& root)
{
	rpc_collection_walk walk {};
	const auto succeeded = walk.visit_file(root);
	assert(succeeded);

	auto& rpcs = walk.get();
	if (!generate_pgraphs(rpcs))
		return std::nullopt;

	return std::make_optional(std::move(rpcs));
}
