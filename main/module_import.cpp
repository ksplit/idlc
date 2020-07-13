#include "module_import.h"

#include <iostream>
#include <sstream>

#include "visit.h"
#include "marshaling.h"

namespace idlc {
	namespace {
		// Logically, collects the information of all signatures that need distinct marshaling
		// Assigns them the names of the functions they bind to, or if they are fptrs, assigns them
		// a signature-dependent hash (function pointers with identical signatures will share identical
		// marshaling units)
		class marshal_unit_collection_pass : public generic_pass<marshal_unit_collection_pass> {
		public:
			marshal_unit_collection_pass(
				std::vector<marshal_unit>& rpcs,
				std::vector<marshal_unit>& rpc_pointers
			) :
				m_rpcs {rpcs},
				m_rpc_pointers {rpc_pointers}
			{
			}

			void visit_rpc(const rpc& rpc)
			{
				m_rpcs.push_back({
					&rpc.get_signature(),
					"rpc_" + std::string {rpc.identifier()}
				});
			}

			void visit_rpc_field(const rpc_field& rpc)
			{
				std::stringstream stream;
				stream << "rpc_" << rpc.signature_hash;
				m_rpc_pointers.push_back({
					&rpc.get_signature(),
					stream.str()
				});
			}

		private:
			std::vector<marshal_unit>& m_rpcs;
			std::vector<marshal_unit>& m_rpc_pointers;
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

idlc::module_import_pass::module_import_pass(
	std::vector<marshal_unit>& rpcs,
	std::vector<marshal_unit>& rpc_pointers,
	node_map<module>& modules
) :
	m_modules {modules},
	m_rpcs {rpcs},
	m_rpc_pointers {rpc_pointers}
{
}

void idlc::module_import_pass::visit_require(const require& require)
{
	module* const ptr {m_modules.get(require.identifier())};
	if (!ptr) {
		std::cout << "[error] Could not resolve required module " << require.identifier() << "\n";
		throw std::exception {};
	}

	std::cout << "[info] Processing required module " << require.identifier() << "\n";

	module& mod {*ptr};
	rpc_field_signature_pass rfs;
	marshal_unit_collection_pass muc {m_rpcs, m_rpc_pointers};
	visit(rfs, mod);
	visit(muc, mod);
}