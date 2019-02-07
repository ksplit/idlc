#include <stdio.h>
#include <Passes/astprint.h>

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

void ASTPrintPass::do_pass(Project * tree){
  ASTPrintVisitor *astprint= new ASTPrintVisitor();

  // ASTPrintVisitor Test 1 - visit Module 
  for (auto m: *tree) {
    std::cout<<__FILE__<<" printing tree"<<std::endl;		 
    m->accept(astprint);
  }

  // ASTPrintVisitor Test 2 - visit Include
  std::vector<Include*> project_includes = tree->includes();
  for (auto incl: project_includes) {
    incl->accept(astprint);	
  }

  // ASTPrintVisitor Test 3 - vist Node
  std::vector<VisitNode*> nodes;
  nodes.push_back(tree);
  for (auto m: *tree) {
    nodes.push_back(m);
  }
  for (auto incl: project_includes) {
    nodes.push_back(incl);
  }
  for (auto node: nodes){
    //std::cout<<__LINE__<<std::endl;	
    node->accept(astprint);
  }
}	
