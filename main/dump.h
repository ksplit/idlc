#ifndef _DUMP_H_
#define _DUMP_H_

#include <ostream>
#include "../parser/ast.h"

namespace idlc {
	inline std::ostream& tab_over(unsigned int level, std::ostream& os)
	{
		for (unsigned int i {0}; i < level; ++i)
			os << "\t";

		return os;
	}

	gsl::czstring<> to_string(rpc_side side) noexcept;

	void dump(const primitive_type& pt, std::ostream& os, unsigned int level);
	void dump(const signature& sig, std::ostream& os, unsigned int level);
	void dump(const projection_type& pt, std::ostream& os, unsigned int level);
	void dump(const copy_type& ct, std::ostream& os, unsigned int level);
	void dump(const copy_type& ct, std::ostream& os, unsigned int level);
	void dump(const attributes& attrs, std::ostream& os, unsigned int level);
	void dump(const type& type, std::ostream& os, unsigned int level);
	void dump(const var_field& field, std::ostream& os, unsigned int level);
	void dump(const rpc_field& field, std::ostream& os, unsigned int level);
	void dump(const field& field, std::ostream& os, unsigned int level);
	void dump(const projection& proj, std::ostream& os, unsigned int level);
	void dump(const rpc& rpc, std::ostream& os, unsigned int level);
	void dump(const require& req, std::ostream& os, unsigned int level);
	void dump(const module_item& item, std::ostream& os, unsigned int level);
	void dump(const module& mod, std::ostream& os, unsigned int level);
	void dump(const include& inc, std::ostream& os, unsigned int level);
	void dump(const file_item& fi, std::ostream& os, unsigned int level);
	void dump(const file& file, std::ostream& os, unsigned int level = 0);
}

#endif