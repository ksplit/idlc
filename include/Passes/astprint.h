#include "lcd_ast.h"
#include <string>

class ASTDeriveSpecsPass {
  

  // Visit any Node of the project
  void visit_node(Project *tree, ASTDeriveSpecsVisitor *astprint);

  // Visit includes of the project
  void visit_project_includes(Project *tree, ASTDeriveSpecsVisitor *astprint);

  // Visit require nodes of module
  void visit_module_requires(Module *module, ASTDeriveSpecsVisitor *astprint);

  // Visit rpcs of module
  void visit_module_rpcs(Module *module, ASTDeriveSpecsVisitor *astprint);

  // A helper fn for visiting the types of a lexical scope
  void visit_scope_types(LexicalScope *scope, ASTDeriveSpecsVisitor *astprint);

  // Visit types in scope of module
  void visit_module_scope_types(Module *module, ASTDeriveSpecsVisitor *astprint);

  // Visit types in outer scope of module's scope -- same as a module's
  // global scope (if module is not nested inside another module)
  void visit_module_outerscope_types(Module *module, ASTDeriveSpecsVisitor *astprint);

  // Visit types in global scope of module's scope -- same as a module's
  // outer scope (if module is not nested in another module)
  void visit_module_globalscope_types(Module *module,
                                      ASTDeriveSpecsVisitor *astprint);

  // Visit types in inner scopes of module's scope
  void visit_module_innerscopes_types(Module *module,
                                      ASTDeriveSpecsVisitor *astprint);

public:
  void do_pass(Project *tree);
};
