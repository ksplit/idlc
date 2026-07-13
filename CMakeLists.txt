cmake_minimum_required(VERSION 3.10.0)

project(lcds)

find_path(ABSL_INCLUDE_DIR absl/strings/string_view.h)
if(NOT ABSL_INCLUDE_DIR)
	message(FATAL_ERROR "absl headers not found")
endif()

# generation.cpp uses absl::operator<<(ostream&, string_view) from
# libabsl_strings. Ubuntu's libabsl-dev uses an inline namespace suffix
# (absl::debian2::) which mangles the symbol; linking against absl::strings
# via the installed CMake package pulls in the right library.
find_package(absl REQUIRED)

set(PARSER_CPP ${PROJECT_SOURCE_DIR}/source/parser/parser.cpp)
set(PARSER_H ${PROJECT_SOURCE_DIR}/source/parser/parser.h)

if(NOT EXISTS ${PARSER_CPP} OR NOT EXISTS ${PARSER_H})
	find_program(PYTHON2_EXECUTABLE NAMES python2 python2.7)
	if(NOT PYTHON2_EXECUTABLE)
		message(FATAL_ERROR "python2 is required to generate parser.cpp/parser.h")
	endif()

	add_custom_command(
		OUTPUT ${PARSER_CPP} ${PARSER_H}
		COMMAND ${PYTHON2_EXECUTABLE} ${PROJECT_SOURCE_DIR}/source/parser/vembyr-1.1/peg.py --save=${PARSER_H} --h ${PROJECT_SOURCE_DIR}/source/parser/idl.peg
		COMMAND ${PYTHON2_EXECUTABLE} ${PROJECT_SOURCE_DIR}/source/parser/vembyr-1.1/peg.py --save=${PARSER_CPP} --cpp ${PROJECT_SOURCE_DIR}/source/parser/idl.peg
		MAIN_DEPENDENCY ${PROJECT_SOURCE_DIR}/source/parser/idl.peg
	)
endif()

add_executable(
	idlc
	"source/parser/parser.cpp"
	"source/parser/parse_globals.cpp"
	"source/ast/ast_dump.cpp"
	"source/ast/pgraph_dump.cpp"
	"source/frontend/analysis.cpp"
	"source/frontend/name_binding.cpp"
	"source/backend/generation.cpp"
	"source/backend/c_specifiers.cpp"
	"source/backend/helpers.cpp"
	"source/string_heap.cpp"
	"source/main.cpp"
)

set_property(TARGET idlc PROPERTY CXX_STANDARD 17)
target_include_directories(idlc PRIVATE ${PROJECT_SOURCE_DIR}/source ${ABSL_INCLUDE_DIR})
target_link_libraries(idlc PRIVATE absl::strings)
