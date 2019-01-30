$(info ************ PROCESSING MAKEFILE **********)
.PHONY: default clean test lcd_idl.h lcd_idl.cpp

bin = compiler
idl_cpp = parser/lcd_idl.cpp
idl_h = include/lcd_idl.h
idl = $(idl_cpp) $(idl_h)

CXXFLAGS = -g -fno-omit-frame-pointer -I include/ #-fsanitize=address
CXXFLAGS += -MMD -std=c++1z -Wall
CXX = g++
LDFLAGS = #-lasan

CPP_a = $(shell find . -type f ! -path "./parser/vembyr-1.1/*" -name "*.cpp") $(idl_cpp)
CPP = $(filter-out ./test/%,$(CPP_a))
OBJ = $(patsubst %.cpp,%.o,$(CPP))
# ah to resolve - where the .d files are created
DEP = $(patsubst %.cpp,%.d,$(CPP)) 

default: $(bin)

$(bin): $(OBJ)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

main/main.cpp: $(idl_h)

# ah note - vembyr parser invoked and supplied with the rule file

$(idl_h): parser/lcd_idl.peg 
	parser/vembyr-1.1/peg.py --h $^ > $@ 

$(idl_cpp): parser/lcd_idl.peg
	parser/vembyr-1.1/peg.py --cpp $^ > $@ 

clean:
	-rm -f $(OBJ) $(bin) $(idl) include/lcd_ast.h.gch $(DEP)

test: $(bin)	
	./test/test.py

-include $(DEP)
