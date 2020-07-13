#include "module_import.h"

#include <iostream>
#include <sstream>

#include "visit.h"
#include "marshaling.h"

namespace idlc {
	namespace {
		class type_collection_pass : public generic_pass<type_collection_pass> {
		public:
			void visit_module(module& module) noexcept
			{
				m_types = &module.types;
			}

			void visit_projection(const projection& projection)
			{
				if (!m_types->insert(projection)) {
					std::cout << "Encountered projection redefinition: " << projection.identifier() << "\n";
					throw std::exception {};
				}
			}

		private:
			node_map<const projection>* m_types;
		};

		// TODO: would be nice to have an error context
		// NOTE: I don't think exceptions are terribly appropriate for this

		class type_resolve_pass : public generic_pass<type_resolve_pass> {
		public:
			void visit_module(module& module) noexcept
			{
				m_types = &module.types;
			}

			void visit_projection_type(projection_type& proj)
			{
				const projection* def {m_types->get(proj.identifier())};
				if (def) {
					proj.definition(def);
				}
				else {
					std::cout << "[error] could not resolve projection: " << proj.identifier() << "\n";
					throw std::exception {};
				}
			}

		private:
			node_map<const projection>* m_types;
		};

		// Logically, collects the information of all signatures that need distinct marshaling
		// Assigns them the names of the functions they bind to, or if they are fptrs, assigns them
		// a signature-dependent hash (function pointers with identical signatures will share identical
		// marshaling units)
		class marshal_unit_collection_pass : public generic_pass<marshal_unit_collection_pass> {
		public:
			marshal_unit_collection_pass(std::vector<marshal_unit>& unit_vec) : m_units {unit_vec}
			{
			}

			void visit_rpc(const rpc& rpc)
			{
				m_units.push_back(marshal_unit {
					rpc.get_signature(),
					"rpc_" + std::string {rpc.identifier()},
					false});
			}

			void visit_rpc_field(const rpc_field& rpc)
			{
				std::stringstream stream;
				stream << "rpc_" << rpc.signature_hash;
				m_units.push_back(marshal_unit {
					rpc.get_signature(),
					stream.str(),
					true});
			}

		private:
			std::vector<marshal_unit>& m_units;
		};

		// Credit goes to the boost people
		template <class T>
		inline void hash_combine(std::size_t& seed, const T& v)
		{
			std::hash<T> hasher;
			seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		}

		// Maybe not the *clearest* pass
		class signature_hash_pass : public generic_pass<signature_hash_pass> {
		public:
			signature_hash_pass(std::uint64_t& hash) :
				m_hash {hash}
			{}

			void visit_projection_type(const projection_type& pt)
			{
				// Since the underlying type is guaranteed unique, we can just use the pointer to identify it
				// Alternatively, the identifier pointer, due to the string_heap semantics
				hash_combine(m_hash, &pt.definition());
			}

			void visit_type(const type& ty)
			{
				hash_combine(m_hash, ty.stars());
			}

			void visit_copy_type(const copy_type& ct)
			{
				hash_combine(m_hash, ct.kind());
			}

			void visit_primitive_type(const primitive_type& pt)
			{
				hash_combine(m_hash, pt.kind());
			}

			void visit_attributes(const attributes& attribs)
			{
				hash_combine(m_hash, attribs.get_value_copy_direction());
				// A little hairy, but the sharing_op is technically in a defined
				// (if meaningless) state for markers with no sharing attributes
				hash_combine(m_hash, attribs.get_sharing_op());
				hash_combine(m_hash, attribs.get_sharing_op_side());
			}

		private:
			std::uint64_t& m_hash;
			bool m_visited_signature {};
		};

		class rpc_field_signature_pass : public generic_pass<rpc_field_signature_pass> {
		public:
			void visit_rpc_field(rpc_field& field)
			{
				signature_hash_pass sh {field.signature_hash};
				visit(sh, field.get_signature());
			}
		};
	}
}

idlc::module_import_pass::module_import_pass(std::vector<marshal_unit>& units, node_map<module>& modules) :
	m_modules {modules},
	m_units {units}
{
}

void idlc::module_import_pass::visit_require(const require& require)
{
	module* const ptr {m_modules.get(require.identifier())};
	if (!ptr) {
		std::cout << "[error] could not resolve required module " << require.identifier() << "\n";
		throw std::exception {};
	}

	// NOTE: is it the best idea to run module verification on-demand?
	// Surely we don't want to waste marshaling work on RPCs that will never be used
	std::cout << "[info] processing required module " << require.identifier() << "\n";

	module& mod {*ptr};
	type_collection_pass tc;
	type_resolve_pass tr;
	rpc_field_signature_pass rfs;
	marshal_unit_collection_pass muc {m_units};
	visit(tc, mod);
	visit(tr, mod);
	visit(rfs, mod);
	visit(muc, mod);
}