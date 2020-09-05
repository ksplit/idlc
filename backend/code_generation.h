#ifndef _CODE_GENERATION_H_
#define _CODE_GENERATION_H_

#include <gsl/gsl>

#include "marshaling.h"

#ifdef __GNUC__
#if __GNUC__ < 8
#include <experimental/filesystem>
namespace std {
	namespace filesystem = std::experimental::filesystem;
}
#else
#include <filesystem>
#endif
#else
#include <filesystem>
#endif

namespace idlc {
	void generate_module(
		const std::filesystem::path& destination,
		std::string_view driver_name,
		gsl::span<const gsl::czstring<>> headers,
		gsl::span<const rpc_unit> rpc_lists,
		gsl::span<const rpc_pointer_unit> rpc_pointer_lists
	);
}

#endif
