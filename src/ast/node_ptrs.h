#ifndef IDLC_AST_H
#define IDLC_AST_H

#include <vector>
#include <memory>

#include <gsl/gsl>

namespace idlc {
	template<typename type>
	using node_ptr = std::shared_ptr<type>;

	template<typename type>
	using node_ref = gsl::not_null<node_ptr<type>>;

	template<typename type>
	using ptr_vec = std::vector<node_ptr<type>>;

	template<typename type>
	using ref_vec = std::vector<node_ref<type>>;
}

#endif
