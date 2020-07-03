cmake_minimum_required(VERSION 3.10.0)

project(lcds)

find_package(Python2)

add_custom_command(
    OUTPUT ${PROJECT_SOURCE_DIR}/parser/parser.cpp ${PROJECT_SOURCE_DIR}/parser/parser.h
    COMMAND ${Python2_EXECUTABLE} ${PROJECT_SOURCE_DIR}/parser/vembyr-1.1/peg.py --save=${PROJECT_SOURCE_DIR}/parser/parser.h --h ${PROJECT_SOURCE_DIR}/parser/idl.peg
    COMMAND ${Python2_EXECUTABLE} ${PROJECT_SOURCE_DIR}/parser/vembyr-1.1/peg.py --save=${PROJECT_SOURCE_DIR}/parser/parser.cpp --cpp ${PROJECT_SOURCE_DIR}/parser/idl.peg
    MAIN_DEPENDENCY parser/idl.peg)

add_executable(idlc parser/parser.cpp main/main.cpp)
set_property(TARGET idlc PROPERTY CXX_STANDARD 17)
target_include_directories(idlc PRIVATE gsl/include)
