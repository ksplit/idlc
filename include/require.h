#include "lcd_ast.h"
#include <stdlib.h>

class RequirePass {
  public:
    void test_func();
    Project * do_pass(std::string file);
//    Project * do_pass(Project * tree);
};

Project * process_includes(std::string input);
void create_module_map();
void print_maps();
Project * resolve_requires(Project * tree);
