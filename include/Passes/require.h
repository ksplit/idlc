#include "lcd_ast.h"
#include <stdlib.h>
#include "lcd_idl.h"

class RequirePass {
  public:
    void test_func();
    Project * do_pass(std::string file);

  private:
    Project * process_includes(std::string input);
    void create_module_map();
    void print_maps();
    Project * resolve_requires(Project * tree);
    Module * expand_module_requires(Module * m);
};


