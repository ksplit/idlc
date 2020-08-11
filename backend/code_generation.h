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
		gsl::span<marshal_unit_lists> rpc_lists,
		gsl::span<marshal_unit_lists> rpc_pointer_lists
	);
}

#endif
