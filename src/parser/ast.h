#ifndef _LCDS_IDL_PARSER_AST_H_
#define _LCDS_IDL_PARSER_AST_H_

#include <memory>
#include <vector>
#include <variant>

#include <gsl/gsl>

namespace idlc::parser {
	template<typename type>
	using node_ptr = std::shared_ptr<type>;
	
	template<typename type>
	using node_ref = gsl::not_null<node_ptr<type>>;

	struct driver_def;
	struct driver_file;
	
	using file = std::variant<std::shared_ptr<driver_file>>;

	struct driver_def {
		gsl::czstring<> name;
		std::shared_ptr<std::vector<gsl::czstring<>>> imports;
	};

	struct driver_file {
		node_ptr<std::vector<gsl::czstring<>>> former;
		node_ref<driver_def> driver;
		node_ptr<std::vector<gsl::czstring<>>> latter;
	};
}

#endif
