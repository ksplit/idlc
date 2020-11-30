cmake_minimum_required(VERSION 3.10.0)

set(
	CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake
	CACHE STRING "Vcpkg toolchain file"
)

project(lcds)

find_package(Microsoft.GSL CONFIG REQUIRED)
find_package(absl CONFIG REQUIRED)

add_custom_command(
	OUTPUT ${PROJECT_SOURCE_DIR}/src/parser/parser.cpp ${PROJECT_SOURCE_DIR}/src/parser/parser.h
	COMMAND python2 ${PROJECT_SOURCE_DIR}/src/parser/vembyr-1.1/peg.py --save=${PROJECT_SOURCE_DIR}/src/parser/parser.h --h ${PROJECT_SOURCE_DIR}/src/parser/idl.peg
	COMMAND python2 ${PROJECT_SOURCE_DIR}/src/parser/vembyr-1.1/peg.py --save=${PROJECT_SOURCE_DIR}/src/parser/parser.cpp --cpp ${PROJECT_SOURCE_DIR}/src/parser/idl.peg
	MAIN_DEPENDENCY ${PROJECT_SOURCE_DIR}/src/parser/idl.peg
)

add_executable(
	idlc
	"src/parser/parser.cpp"
	"src/parser/string_heap.cpp"
	"src/parser/parse_globals.cpp"
	"src/marshal/build_pgraph.cpp"
	"src/main.cpp"
)

set_property(TARGET idlc PROPERTY CXX_STANDARD 17)
target_link_libraries(idlc PRIVATE Microsoft.GSL::GSL)
target_link_libraries(idlc PRIVATE absl::strings)
