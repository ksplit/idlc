#include "analysis.h"

#include <cassert>
#include <exception>
#include <memory>
#include <optional>
#include <tuple>
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
			return std::make_shared<null_terminated_array>(
				std::make_shared<value>(primitive::ty_char, annotation_kind::use_default, false)
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
					return std::make_shared<rpc_ptr>(item.get().get()->definition);
				}
				else if constexpr (std::is_same_v<type, node_ref<type_casted>>) {
					return std::make_shared<casted_type>(
						generate_value(*item->declared_type),
						generate_value(*item->true_type)
					);
				}
				else if constexpr (std::is_same_v<type, type_none>) {
					return none {};
				}

				std::cout << "Debug: Unknown stem type conversion\n";
				std::cout.flush();
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
					return std::make_shared<null_terminated_array>(generate_value(*node.element));
				}
				else if constexpr (std::is_same_v<type, unsigned>) {
					return std::make_shared<static_array>(generate_value(*node.element), item);
				}
				else if constexpr (std::is_same_v<type, ident>) {
					return std::make_shared<dyn_array>(generate_value(*node.element), item);
				}

				std::terminate();
			};

			return std::visit(visit, *node.size);
		}

		auto generate_field(proj_field& node)
		{
			const auto& [def, width] = node;
			const auto visit = [](auto&& item) -> idlc::projection_field
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

			return std::visit(visit, def);
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
				const auto val_annots = annots->kind & annotation_kind::is_val;
				auto field = std::make_shared<value>(std::move(type), val_annots, is_const);

				type = std::make_shared<pointer>(
					std::move(field),
					annotation {
						annots->kind & annotation_kind::is_ptr,
						annots->share_global,
						annots->size_verbatim,
						annots->flags_verbatim,
						annots->member
					}
				);

				is_const = ptr_node->is_const;
			}

			assert((node.attrs & annotation_kind::is_val) == node.attrs);
			auto field = std::make_shared<value>(std::move(type), node.attrs, is_const);

			return std::move(field);
		}

		void create_pgraphs_from_types(rpc_vec_view rpcs, global_vec_view globals)
		{
			for (const auto& rpc : rpcs) {
				rpc->ret_pgraph = generate_value(*rpc->ret_type);
				if (rpc->arguments) {
					for (auto& argument : *rpc->arguments)
						rpc->arg_pgraphs.emplace_back(generate_value(*argument->type));
				}
			}

			for (const auto& global : globals)
				global->pgraph = generate_value(*global->type);
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

			pgraph_node->def = &node;

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

		bool annotate_pgraph(value& node, annotation_kind default_with);
		passed_type instantiate_projection(proj_def& node, annotation_kind default_with);

		// TODO: implement error checking, currently focused on generating defaults
		class annotation_walk : public pgraph_walk<annotation_walk> {
		public:
			annotation_walk(annotation_kind default_with) : m_default_with {default_with}
			{
				// We only support propagating a default value annotation
				assert(is_clear(default_with & annotation_kind::ptr_only));
			}

			bool visit_value(value& node)
			{
				// The core logic of propagating a top-level value annotation until an explicit one is found, and
				// continuing
				if (!is_clear(node.value_annots & annotation_kind::io_only)) {
					m_default_with = node.value_annots & annotation_kind::io_only;			
				}
				else {
					node.value_annots |= m_default_with; // FIXME
				}

				return traverse(*this, node);
			}

			bool visit_pointer(pointer& node)
			{
				// Default if no pointer annotations are set
				// TODO: what are the correct defaults here?
				if (is_clear(node.pointer_annots.kind & annotation_kind::ptr_only)) {
					if (m_default_with == annotation_kind::in)
						node.pointer_annots.kind = annotation_kind::bind_caller;
					else if (m_default_with == annotation_kind::out)
						node.pointer_annots.kind = annotation_kind::bind_callee;
					else if (m_default_with == (annotation_kind::in | annotation_kind::out))
						node.pointer_annots.kind = (annotation_kind::bind_callee | annotation_kind::bind_caller);
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
			annotation_kind m_default_with {};
		};

		bool annotate_pgraph(value& node, annotation_kind default_with)
		{
			annotation_walk annotator {default_with};
			return annotator.visit_value(node);
		}

		std::string get_instance_name(absl::string_view base_name, annotation_kind default_with)
		{
			// double underscore used to reduce odds of collisions
			std::string instance_name {base_name};
			switch (default_with) {
			case annotation_kind::in:
				instance_name += "__in";
				break;

			case annotation_kind::out:
				instance_name += "__out";
				break;

			case annotation_kind::in_out:
				instance_name += "__io";
				break;

			default:
				std::terminate();
			}

			return instance_name;
		}

		passed_type instantiate_projection(proj_def& node, node_ptr<projection>& cached, annotation_kind default_with)
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

		passed_type instantiate_projection(proj_def& node, annotation_kind default_with)
		{
			std::cout << "Debug: Non-lowered projection \"" << node.name << "\"\n";
			switch (default_with) {
			case annotation_kind::in_out:
				std::cout << "Debug: Need \"in/out\" instance\n";
				return instantiate_projection(node, node.in_out_proj, default_with);

			case annotation_kind::in:
				std::cout << "Debug: Need \"in\" instance\n";
				return instantiate_projection(node, node.in_proj, default_with);

			case annotation_kind::out:
				std::cout << "Debug: Need \"out\" instance\n";
				return instantiate_projection(node, node.out_proj, default_with);

			default:
				std::terminate();
			}
		}

		class const_walk : public pgraph_walk<const_walk> {
		public:
			bool visit_null_terminated_array(null_terminated_array& node)
			{
				node.element->is_const = m_const;
				return traverse(*this, node);
			}

			bool visit_dyn_array(dyn_array& node)
			{
				node.element->is_const = m_const;
				return traverse(*this, node);
			}

			bool visit_static_array(static_array& node)
			{
				node.element->is_const = m_const;
				return traverse(*this, node);
			}

			bool visit_value(value& node)
			{
				const auto old = m_const;
				m_const = node.is_const;
				if (!traverse(*this, node))
					return false;

				m_const = old;

				return true;
			}

		private:
			bool m_const {};
		};

		bool annotate_pgraphs(rpc_def& rpc)
		{
			if (!annotate_pgraph(*rpc.ret_pgraph, annotation_kind::out))
				return false;
			
			const_walk const_propagator {};
			const auto succeeded = const_propagator.visit_value(*rpc.ret_pgraph);
			assert(succeeded);

			for (const auto& pgraph : rpc.arg_pgraphs) {
				if (!annotate_pgraph(*pgraph, annotation_kind::in))
					return false;

				const_walk const_propagator {};
				const auto succeeded = const_propagator.visit_value(*pgraph);
				assert(succeeded);
			}

			return true;
		}

		bool annotate_pgraphs(rpc_vec_view rpcs, global_vec_view globals)
		{
			for (const auto& rpc : rpcs) {
				if (!annotate_pgraphs(*rpc))
					return false;
			}

			for (const auto& global : globals) {
				if (!annotate_pgraph(*global->pgraph, annotation_kind::out))
					return false;
			}

			return true;
		}

		class rpc_context_walk : public ast_walk<rpc_context_walk> {
		public:
			bool visit_rpc_def(rpc_def& node)
			{
				const auto old = m_current;
				m_current = &node;
				if (!traverse(*this, node))
					return false;

				m_current = old;

				return true;
			}

			bool visit_proj_def(proj_def& node)
			{
				node.parent = m_current;
				return traverse(*this, node);
			}

		private:
			const rpc_def* m_current {};
		};

		bool generate_pgraphs(rpc_vec_view rpcs, global_vec_view globals)
		{
			create_pgraphs_from_types(rpcs, globals);
			return annotate_pgraphs(rpcs, globals);
		}

		class global_collection_walk : public ast_walk<global_collection_walk> {
		public:
			bool visit_global_def(global_def& node)
			{
				m_globals.emplace_back(&node);
				return true;
			}

			auto& get() const
			{
				return m_globals;
			}

		private:
			global_vec m_globals {};
		};
	}
}

std::optional<std::tuple<idlc::rpc_vec, idlc::global_vec>> idlc::generate_all_pgraphs(file& root)
{
	rpc_collection_walk rpc_walk {};
	auto succeeded = rpc_walk.visit_file(root);
	assert(succeeded);

	global_collection_walk glb_walk {};
	succeeded = glb_walk.visit_file(root);
	assert(succeeded);

	rpc_context_walk ctx_walk {};
	succeeded = ctx_walk.visit_file(root);
	assert(succeeded);

	auto& rpcs = rpc_walk.get();
	auto& globals = glb_walk.get();
	if (!generate_pgraphs(rpcs, globals))
		return std::nullopt;

	return std::make_optional(std::make_tuple(std::move(rpcs), std::move(globals)));
}
