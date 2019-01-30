#include "lcd_ast.h"
#include <stdlib.h>

class RequirePass {
  public:
    void test_func();
    Project * do_pass(std::string file);
    // Project * do_pass(Project * tree);
};
