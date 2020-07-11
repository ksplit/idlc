#ifndef _GENERIC_PASS_H_
#define _GENERIC_PASS_H_

#include <utility>

namespace idlc {
	// TODO: possibly subject to revision, since it depends on name-hiding (that's why you see the gsl::suppress' everywhere)
	template<typename derived>
	class generic_pass {
	private:
		derived& self() noexcept
		{
			return *static_cast<derived*>(this);
		}

	public:
		void operator()(module& n) noexcept(noexcept(std::declval<generic_pass>().self().visit_module(n)))
		{
			self().visit_module(n);
		}

		void operator()(file& n) noexcept(noexcept(std::declval<generic_pass>().self().visit_file(n)))
		{
			self().visit_file(n);
		}

		void operator()(include& n) noexcept(noexcept(std::declval<generic_pass>().self().visit_include(n)))
		{
			self().visit_include(n);
		}

		void operator()(rpc& n) noexcept(noexcept(std::declval<generic_pass>().self().visit_rpc(n)))
		{
			self().visit_rpc(n);
		}

		void operator()(projection& n) noexcept(noexcept(std::declval<generic_pass>().self().visit_projection(n)))
		{
			self().visit_projection(n);
		}

		void operator()(primitive_type& n) noexcept(noexcept(std::declval<generic_pass>().self().visit_primitive_type(n)))
		{
			self().visit_primitive_type(n);
		}

		void operator()(var_field& n) noexcept(noexcept(std::declval<generic_pass>().self().visit_var_field(n)))
		{
			self().visit_var_field(n);
		}

		void operator()(rpc_field& n) noexcept(noexcept(std::declval<generic_pass>().self().visit_rpc_field(n)))
		{
			self().visit_rpc_field(n);
		}

		void operator()(type& n) noexcept(noexcept(std::declval<generic_pass>().self().visit_type(n)))
		{
			self().visit_type(n);
		}

		void operator()(signature& n) noexcept(noexcept(std::declval<generic_pass>().self().visit_signature(n)))
		{
			self().visit_signature(n);
		}

		void operator()(attributes& n) noexcept(noexcept(std::declval<generic_pass>().self().visit_attributes(n)))
		{
			self().visit_attributes(n);
		}

		void operator()(projection_type& n) noexcept(noexcept(std::declval<generic_pass>().self().visit_projection_type(n)))
		{
			self().visit_projection_type(n);
		}

		void operator()(require& n) noexcept(noexcept(std::declval<generic_pass>().self().visit_require(n)))
		{
			self().visit_require(n);
		}

		void visit_module(module&) noexcept {}
		void visit_file(file&) noexcept {}
		void visit_include(include&) noexcept {}
		void visit_rpc(rpc&) noexcept {}
		void visit_projection(projection&) noexcept {}
		void visit_primitive_type(primitive_type&) noexcept {}
		void visit_var_field(var_field&) noexcept {}
		void visit_rpc_field(rpc_field&) noexcept {}
		void visit_type(type&) noexcept {}
		void visit_signature(signature&) noexcept {}
		void visit_attributes(attributes&) noexcept {}
		void visit_projection_type(projection_type&) noexcept {}
		void visit_require(require&) noexcept {}
	};
}

#endif // !_GENERIC_PASS_H_
