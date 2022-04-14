#pragma once

#include <set>

#include "../ast/ast_walk.h"
#include "../ast/ast.h"
#include "../parser/parse_globals.h"

namespace idlc {
	template<typename type, typename... types>
	auto make(types... arguments) { return std::make_shared<type>(arguments...); }

	auto make_void()
	{
		return make<type_spec>(
			make<type_stem>(type_none {}),
			ref_vec<indirection> {},
			annotation_kind::use_default,
			false,
			false
		);
	}

	enum class sync_type {
		get,
		set
	};

	auto make_sync_argument(const lock_scope& lock, sync_type type)
	{
		const auto attribute = type == sync_type::get ? annotation_kind::out : annotation_kind::in;
		return make<ref_vec<var_decl>>(ref_vec<var_decl> {
			make<var_decl>(
				make<type_spec>(
					make<type_stem>(make<type_proj>(lock.parent->name)),
					ref_vec<indirection> {make<indirection>(
						make<annotation>(attribute | annotation_kind::bind_caller),
						false
					)},
					annotation_kind::use_default,
					false,
					false
				),
				parser::idents.intern("data")
			)
		});
	}

	class annotation_erasure_walk : public ast_walk<annotation_erasure_walk> {
	public:
		bool visit_value(value& node)
		{
			node.value_annots = annotation_kind::use_default;
			return true;
		}

		bool visit_indirection(indirection& node)
		{
			node.attrs->kind = annotation_kind::use_default;
			return true;
		}
	};

	class type_clone_walk : public ast_walk<type_clone_walk> {
	public:
		type_clone_walk(std::set<const proj_def*>& projections) : m_projections {projections} {}

		bool visit_type_proj(type_proj& node)
		{
			type_clone_walk subwalk {m_projections};
			const auto is_known = m_projections.find(node.definition) != m_projections.end();
			return (is_known || subwalk.visit_proj_def(*node.definition)) && traverse(*this, node);
		}

		bool visit_proj_def(proj_def& node)
		{
			m_projections.insert(&node);
			return traverse(*this, node);
		}

	private:
		std::set<const proj_def*>& m_projections;
	};

	auto make_clones(const lock_scope& scope)
	{
		std::set<const proj_def*> projections;
		type_clone_walk cloning {projections};
		cloning.visit_proj_def(*scope.parent);
		std::cerr << "Debug: need clones for " << projections.size() << " projections\n";
		const auto clones = make<ref_vec<rpc_item>>();
		for (const auto proj : projections) {
			auto proj_clone = clone(*proj);
			annotation_erasure_walk walk {};
			walk.visit_proj_def(proj_clone);
			clones->emplace_back(make<rpc_item>(make<proj_def>(std::move(proj_clone))));
		}

		return clones;
	}

	class injected_rpcs_walk : public ast_walk<injected_rpcs_walk> {
	public:
		bool visit_global_def(const global_def& node)
		{
			m_defs.emplace_back(&node);
			return true;
		}

		bool visit_lock_scope(lock_scope& node)
		{
			node.parent = m_parent;
			m_locks.emplace_back(&node);
			return true;
		}

		bool visit_lock_def(lock_def& node)
		{
			m_lock_defs.emplace_back(&node);
			return true;
		}

		bool visit_proj_def(proj_def& node)
		{
			const auto old = m_parent;
			m_parent = &node;
			if (!traverse(*this, node))
				return false;

			m_parent = old;

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
					make<module_item>(
						make<rpc_def>(
							def->type,
							parser::idents.intern(concat("__global_init_var_", def->name)),
							nullptr,
							nullptr,
							rpc_def_kind::direct
						)
					)
				);
			}

			for (const auto& lock : m_locks) {
				std::cerr << "Debug: injecting RPC for lock " << lock->name << " in projection for type " << lock->parent->type << '\n';
				node.items->emplace_back(make<module_item>(make<rpc_def>(
					make_void(),
					parser::idents.intern(concat(lock->parent->type, "__", lock->name, "__lock")),
					make_sync_argument(*lock, sync_type::get),
					make_clones(*lock),
					rpc_def_kind::direct
				)));

				node.items->emplace_back(make<module_item>(make<rpc_def>(
					make_void(),
					parser::idents.intern(concat(lock->parent->type, "__", lock->name, "__unlock")),
					make_sync_argument(*lock, sync_type::set),
					make_clones(*lock),
					rpc_def_kind::direct
				)));
			}

			m_defs.clear();
			m_locks.clear();

			return true;
		}

		auto&& move_locks()
		{
			return std::move(m_lock_defs);
		}

	private:
		std::vector<const global_def*> m_defs {};
		std::vector<const lock_scope*> m_locks {};
		std::vector<const lock_def*> m_lock_defs {};
		proj_def* m_parent {};
	};

	auto inject_global_rpcs(file& root)
	{
		injected_rpcs_walk walk {};
		const auto result = walk.visit_file(root);
		assert(result);
		return walk.move_locks();
	}
}
