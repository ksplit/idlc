#include "module_import.h"

#include <iostream>
#include <sstream>

#include "visit.h"
#include "../backend/marshaling.h"
#include "log.h"

namespace idlc {
	namespace {
		// Logically, collects the information of all signatures that need distinct marshaling
		// Assigns them the names of the functions they bind to, or if they are fptrs, assigns them
		// a rpc_signature-dependent mangle (function pointers with identical signatures will share identical
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

			bool visit_rpc(const rpc& rpc)
			{
				m_rpcs.push_back(
					{
						&rpc.get_signature(),
						rpc.identifier()
					}
				);

				return true;
			}

			bool visit_rpc_field(const rpc_field& rpc)
			{
				const auto is_same_rpc = [id = rpc.mangled_signature](auto&& unit) {
					return unit.identifier == id;
				};

				const auto sentinel = end(m_rpc_pointers);
				const auto find_iter = std::find_if(
					begin(m_rpc_pointers),
					sentinel,
					is_same_rpc
				);

				if (find_iter == sentinel) {
					m_rpc_pointers.push_back(
						{
							&rpc.get_signature(),
							rpc.mangled_signature
						}
					);

					return true;
				}
				else {
					return true;
				}
			}

		private:
			std::vector<marshal_unit>& m_rpcs;
			std::vector<marshal_unit>& m_rpc_pointers;
		};

		// Maybe not the *clearest* pass
		class signature_mangle_pass : public generic_pass<signature_mangle_pass> {
		public:
			signature_mangle_pass(std::string& mangle) :
				m_mangle {mangle}
			{}

			bool visit_projection_type(const projection_type& pt)
			{
				// Since the underlying type is guaranteed unique, we can just use the pointer to identify it
				// Alternatively, the identifier pointer, due to the string_heap semantics
				m_mangle += '_';
				// If this weren't here, it'd be possible for mangled names to collide
				m_mangle += pt.definition().parent_module; // ugly hack, but necessary, since projections are module-scoped
				m_mangle += '_';
				m_mangle += pt.identifier();
				return true;
			}

			bool visit_rpc_field(const rpc_field& rpc)
			{
				// Another ugly hack, this time to encode rpc pointers
				// Essentially "rpc_ptr_<number of arguments + return>"
				m_mangle += "_rpc_ptr_";
				m_mangle += std::to_string(rpc.get_signature().arguments().size() + 1);
				return true;
			}

			bool visit_type(const type& ty)
			{
				m_mangle += ty.stars() ? '_' + std::to_string(ty.stars()) : "";
				if (!ty.get_copy_type()) {
					m_mangle += "_void";
				}

				return true;
			}

			bool visit_primitive_type(const primitive_type& pt)
			{
				switch (pt.kind()) {
				case primitive_type_kind::bool_k:
					m_mangle += "_bool";
					break;

				case primitive_type_kind::char_k:
					m_mangle += "_char";
					break;

				case primitive_type_kind::double_k:
					m_mangle += "_double";
					break;

				case primitive_type_kind::float_k:
					m_mangle += "_float";
					break;

				case primitive_type_kind::int_k:
					m_mangle += "_int";
					break;

				case primitive_type_kind::long_k:
					m_mangle += "_long";
					break;

				case primitive_type_kind::long_long_k:
					m_mangle += "_long_long";
					break;

				case primitive_type_kind::short_k:
					m_mangle += "_short";
					break;

				case primitive_type_kind::unsigned_char_k:
					m_mangle += "_unsigned_char";
					break;

				case primitive_type_kind::unsigned_int_k:
					m_mangle += "_unsigned_int";
					break;

				case primitive_type_kind::unsigned_long_k:
					m_mangle += "_unsigned_long";
					break;

				case primitive_type_kind::unsigned_long_long_k:
					m_mangle += "_unsigned_long_long";
					break;

				case primitive_type_kind::unsigned_short_k:
					m_mangle += "_unsigned_short";
					break;
				}

				return true;
			}

			bool visit_attributes(const attributes& attribs)
			{
				switch (attribs.get_value_copy_direction()) {
				case copy_direction::in:
					m_mangle += "_in";
					break;

				case copy_direction::out:
					m_mangle += "_out";
					break;

				case copy_direction::both:
					m_mangle += "_in_out";
					break;
				}

				// A little hairy, but the sharing_op is technically in a defined
				// (if meaningless) state for markers with no sharing attributes
				switch (attribs.get_sharing_op_side()) {
				case rpc_side::callee:
					m_mangle += "_callee";
					break;

				case rpc_side::caller:
					m_mangle += "_caller";
					break;

				case rpc_side::none:
					return true; // no need to get_field the op
				}

				switch (attribs.get_sharing_op()) {
				case sharing_op::alloc:
					m_mangle += "_alloc";
					break;

				case sharing_op::dealloc:
					m_mangle += "_dealloc";
					break;

				case sharing_op::bind:
					m_mangle += "_bind";
					break;
				}

				return true;
			}

		private:
			std::string& m_mangle;
			bool m_visited_signature {};
		};

		class rpc_field_signature_pass : public generic_pass<rpc_field_signature_pass> {
		public:
			bool visit_rpc_field(rpc_field& field)
			{
				std::string mangle;
				signature_mangle_pass sh {mangle};
				if (!visit(sh, field.get_signature())) {
					return false;
				}

				field.mangled_signature = mangle_heap.intern(mangle);

				return true;
			}

		private:
			// TODO: best place for this?
			static string_heap mangle_heap;
		};

		string_heap rpc_field_signature_pass::mangle_heap;

		class header_import_pass : public generic_pass<header_import_pass> {
		public:
			header_import_pass(std::vector<gsl::czstring<>>& headers) noexcept :
				m_headers {headers}
			{}

			bool visit_header_import(const header_import& header)
			{
				m_headers.push_back(header.path());
				return true;
			}

		private:
			std::vector<gsl::czstring<>>& m_headers;
		};
	}
}

idlc::module_import_pass::module_import_pass(
	std::vector<gsl::czstring<>>& headers,
	std::vector<marshal_unit>& rpcs,
	std::vector<marshal_unit>& rpc_pointers,
	node_map<module>& modules
) :
	m_headers {headers},
	m_modules {modules},
	m_rpcs {rpcs},
	m_rpc_pointers {rpc_pointers}
{
}

bool idlc::module_import_pass::visit_require(const require& require)
{
	module* const ptr {m_modules.get(require.identifier())};
	if (!ptr) {
		log_error("Could not resolve required module ", require.identifier());
		return false;
	}

	log_note("Processing required module ", require.identifier());

	module& mod {*ptr};
	header_import_pass hip {m_headers};
	rpc_field_signature_pass rfs;
	marshal_unit_collection_pass muc {m_rpcs, m_rpc_pointers};
	if (!visit(hip, mod)) {
		return false;
	}
	
	if (!visit(rfs, mod)) {
		return false;
	}

	return visit(muc, mod);
}