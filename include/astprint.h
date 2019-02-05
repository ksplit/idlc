#include "lcd_ast.h"
#include <stdio.h>

class ASTPrintVisitor : public ASTVisitor {
  public:
	ASTPrintVisitor();
	void visit(Module *m);
	void visit(Include *e);	
};
