// ========================================================================= //
// IDL Compiler Infrastructure for LCDs					     //
// ========================================================================= //

// astprint.cpp:
// ============
// This file implements the ASTPrintVisitor class. 

#include <stdio.h>
#include <Passes/astprint.h>

ASTPrintVisitor::ASTPrintVisitor(){
}
void ASTPrintVisitor::visit(Project *p){
  std::cout<<__FILE__<<" Visited Project node"<<std::endl;
}	
void ASTPrintVisitor::visit(Module *m) {
  std::cout<<__FILE__<<" Visited Module node: "<<m->identifier()<<std::endl;
}
void ASTPrintVisitor::visit(Include *i){
  std::cout<<__FILE__<<" Visited Include node with path: "<<i->get_path()<<std::endl;
}
void ASTPrintVisitor::visit(Require *rq){
  std::cout<<__FILE__<<" Visited Require node that requires module: "<<rq->get_required_module_name()<<std::endl;
}
void ASTPrintVisitor::visit(Rpc *rp){
  std::cout<<__FILE__<<" Visited Rpc node: "<<rp->name()<<std::endl;
}
void ASTPrintVisitor::visit(ReturnVariable *rv){
  std::cout<<__FILE__<<" Visited ReturnVariable node : "<<std::endl;
}
void ASTPrintVisitor::visit(Type *type){
  std::cout<<__FILE__<<" Visited Type node : "<<type->name()<<std::endl;
}

void ASTPrintPass::do_pass(Project * tree){

  ASTPrintVisitor *astprint= new ASTPrintVisitor();

  // ASTPrintVisitor Test 1 - visit Module, Requires, Rpcs, ReturnVariable, Type
  std::vector<Require*> module_requires; 
  std::vector<Rpc*> module_rpcs; 
  ReturnVariable *rv;
  Type *type;
  for (auto m: *tree) {
    std::cout<<__FILE__<<" printing tree"<<std::endl;		 
    m->accept(astprint);
    module_requires=m->requires();
    for (auto rq: module_requires) {
      rq->accept(astprint);
    }
    module_rpcs=m->rpc_definitions();
    for (auto rp: module_rpcs) {
      rp->accept(astprint);
      rv=rp->return_variable();
      rv->accept(astprint);
      type=rv->type();
      type->accept(astprint);
    }
  }

  // ASTPrintVisitor Test 2 - visit Include
  std::vector<Include*> project_includes = tree->includes();
  for (auto incl: project_includes) {
    incl->accept(astprint);	
  }

  // ASTPrintVisitor Test 3 - vist Node
  /*
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
  */

}	
