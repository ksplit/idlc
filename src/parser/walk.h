#ifndef _LCDS_IDL_PARSER_WALK_H_
#define _LCDS_IDL_PARSER_WALK_H_

#include <type_traits>

#include "ast.h"

namespace idlc::parser {	
	template<typename derived>
	class ast_walk {
	public:
		bool traverse_file(const file& node)
		{
			const auto visit = [this](auto&& subnode)
			{
				using type = decltype(subnode);
				if constexpr (std::is_same_v<type, std::vector<module_def>>) {
					for (const auto& def : subnode) {
						if (!self().traverse_module_def(def))
							return false;
					}

					return true;
				}
				else if constexpr (std::is_same_v<type, driver_file>) {
					return self().traverse_driver_file(subnode);
				}
				else {
					assert(false);
				}
			};			

			return std::visit(visit, node);
		}

		bool traverse_module_def(const module_def& node)
		{
			return true;
		}

		bool traverse_driver_file(const driver_file& node)
		{
			return traverse_driver_def(node.driver);
		}

		bool traverse_driver_def(const driver_def& node)
		{
			return true;
		}

	private:
		auto self()
		{
			return *reinterpret_cast<derived*>(this);
		}
	};

	class null_walk : public ast_walk<null_walk> {

	};
}

#endif
