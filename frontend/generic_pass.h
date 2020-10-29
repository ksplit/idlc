#ifndef _GENERIC_PASS_H_
#define _GENERIC_PASS_H_

#include <utility>

namespace idlc {
	template<typename derived>
	class generic_pass {
	private:
		derived& self() noexcept
		{
			return *static_cast<derived*>(this);
		}

	public:
		bool operator()(header_import& n) noexcept(noexcept(std::declval<generic_pass>().self().visit_header_import(n)))
		{
			return self().visit_header_import(n);
		}

		bool operator()(module& n) noexcept(noexcept(std::declval<generic_pass>().self().visit_module(n)))
		{
			return self().visit_module(n);
		}

		bool operator()(file& n) noexcept(noexcept(std::declval<generic_pass>().self().visit_file(n)))
		{
			return self().visit_file(n);
		}

		bool operator()(include& n) noexcept(noexcept(std::declval<generic_pass>().self().visit_include(n)))
		{
			return self().visit_include(n);
		}

		bool operator()(rpc_definition& n) noexcept(noexcept(std::declval<generic_pass>().self().visit_rpc(n)))
		{
			return self().visit_rpc(n);
		}

		bool operator()(struct_projection_definition& n) noexcept(noexcept(std::declval<generic_pass>().self().visit_projection(n)))
		{
			return self().visit_projection(n);
		}

		bool operator()(primitive_type& n) noexcept(noexcept(std::declval<generic_pass>().self().visit_primitive_type(n)))
		{
			return self().visit_primitive_type(n);
		}

		bool operator()(var_field& n) noexcept(noexcept(std::declval<generic_pass>().self().visit_var_field(n)))
		{
			return self().visit_var_field(n);
		}

		bool operator()(rpc_field& n) noexcept(noexcept(std::declval<generic_pass>().self().visit_rpc_field(n)))
		{
			return self().visit_rpc_field(n);
		}

		bool operator()(variable_type& n) noexcept(noexcept(std::declval<generic_pass>().self().visit_type(n)))
		{
			return self().visit_type(n);
		}

		bool operator()(signature& n) noexcept(noexcept(std::declval<generic_pass>().self().visit_signature(n)))
		{
			return self().visit_signature(n);
		}

		bool operator()(attributes& n) noexcept(noexcept(std::declval<generic_pass>().self().visit_attributes(n)))
		{
			return self().visit_attributes(n);
		}

		bool operator()(projection_type& n) noexcept(noexcept(std::declval<generic_pass>().self().visit_projection_type(n)))
		{
			return self().visit_projection_type(n);
		}

		bool operator()(require& n) noexcept(noexcept(std::declval<generic_pass>().self().visit_require(n)))
		{
			return self().visit_require(n);
		}

		bool visit_header_import(header_import&) noexcept { return true; }
		bool visit_module(module&) noexcept { return true; }
		bool visit_file(file&) noexcept { return true; }
		bool visit_include(include&) noexcept { return true; }
		bool visit_rpc(rpc_definition&) noexcept { return true; }
		bool visit_projection(struct_projection_definition&) noexcept { return true; }
		bool visit_primitive_type(primitive_type&) noexcept { return true; }
		bool visit_var_field(var_field&) noexcept { return true; }
		bool visit_rpc_field(rpc_field&) noexcept { return true; }
		bool visit_type(variable_type&) noexcept { return true; }
		bool visit_signature(signature&) noexcept { return true; }
		bool visit_attributes(attributes&) noexcept { return true; }
		bool visit_projection_type(projection_type&) noexcept { return true; }
		bool visit_require(require&) noexcept { return true; }
	};
}

#endif // !_GENERIC_PASS_H_
