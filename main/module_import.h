#ifndef _MODULE_IMPORT_H_
#define _MODULE_IMPORT_H_

#include "../parser/ast.h"
#include "generic_pass.h"
#include "marshaling.h"

namespace idlc {
	class module_import_pass : public generic_pass<module_import_pass> {
	public:
		module_import_pass(std::vector<marshal_unit>& units, node_map<module>& modules);
		void visit_require(const require& require);

	private:
		std::vector<marshal_unit>& m_units;
		const node_map<module>& m_modules;
	};
}

#endif // !_MODULE_IMPORT_H_
