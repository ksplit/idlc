#ifndef _MARSHALING_H_
#define _MARSHALING_H_

namespace idlc {
	struct marshal_unit {
		const signature& signature;
		std::string identifier;
		bool is_pointer;
	};
}

#endif // !_MARSHALING_H_
