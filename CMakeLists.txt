cmake_minimum_required(VERSION 3.10.0)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake
	CACHE STRING "Vcpkg toolchain file")

project(lcds)

find_package(Python2)
find_package(Microsoft.GSL CONFIG REQUIRED)
find_package(absl CONFIG REQUIRED)

add_custom_command(
	OUTPUT ${PROJECT_SOURCE_DIR}/parser/parser.cpp ${PROJECT_SOURCE_DIR}/parser/parser.h
	COMMAND ${Python2_EXECUTABLE} ${PROJECT_SOURCE_DIR}/parser/vembyr-1.1/peg.py --save=${PROJECT_SOURCE_DIR}/parser/parser.h --h ${PROJECT_SOURCE_DIR}/parser/idl.peg
	COMMAND ${Python2_EXECUTABLE} ${PROJECT_SOURCE_DIR}/parser/vembyr-1.1/peg.py --save=${PROJECT_SOURCE_DIR}/parser/parser.cpp --cpp ${PROJECT_SOURCE_DIR}/parser/idl.peg
	MAIN_DEPENDENCY parser/idl.peg)

add_executable(
	idlc
	"parser/parser.cpp"
	"test/test.cpp"
	"frontend/string_heap.cpp"
)

set_property(TARGET idlc PROPERTY CXX_STANDARD 17)
target_link_libraries(idlc PRIVATE Microsoft.GSL::GSL)
target_link_libraries(idlc PRIVATE absl::strings)
