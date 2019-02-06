#include "lcd_ast.h"
#include <stdio.h>

ASTPrintVisitor::ASTPrintVisitor(){
}
void ASTPrintVisitor::visit(Project *p){
  std::cout<<__FILE__<<" Visited Project node"<<std::endl;
}	
void ASTPrintVisitor::visit(Module *m) {
  std::cout<<__FILE__<<" Visited Module node"<<std::endl;
}
void ASTPrintVisitor::visit(Include *e){
  std::cout<<__FILE__<<" Visited Include node"<<std::endl;
}	
