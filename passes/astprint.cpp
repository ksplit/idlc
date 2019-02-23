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

void ASTPrintVisitor::visit(Module *m){
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
  std::cout<<__FILE__<<" Visited ReturnVariable: "<<rv->name_<<std::endl;
  std::cout<<__FILE__<<" ----------------------in: "<<rv->in_<<std::endl;
  std::cout<<__FILE__<<" ----------------------out: "<<rv->out_<<std::endl;
  std::cout<<__FILE__<<" ----------------------alloc_caller: "<<rv->alloc_caller_<<std::endl;
  std::cout<<__FILE__<<" ----------------------alloc_callee: "<<rv->alloc_callee_<<std::endl;
}

void ASTPrintVisitor::visit(Type *type){
  std::cout<<__FILE__<<" Visited Type node: "<<type->name()<<std::endl;
}

void ASTPrintVisitor::visit(LexicalScope *lexical_scope){
  std::cout<<__FILE__<<" Visited Lexical Scope node"<<std::endl;
}

// Visits for each type
void ASTPrintVisitor::visit(UnresolvedType *ut){
  std::cout<<__FILE__<<"Visited UnresolvedType node"<<std::endl;
}
void ASTPrintVisitor::visit(Typedef *td){
  std::cout<<__FILE__<<"Visited Typedef node"<<std::endl;
}
void ASTPrintVisitor::visit(VoidType *vt){
  std::cout<<__FILE__<<" Visited VoidType node"<<std::endl;
}
void ASTPrintVisitor::visit(IntegerType *it){
  std::cout<<__FILE__<<" Visited IntegerType node"<<std::endl;
}
void ASTPrintVisitor::visit(ProjectionType *pt){
  std::cout<<__FILE__<<" Visited ProjectionType node"<<std::endl;
}
void ASTPrintVisitor::visit(Function *fp){
  std::cout<<__FILE__<<" Visited Function node"<<std::endl;
}
void ASTPrintVisitor::visit(Channel *c){
  std::cout<<__FILE__<<" Visited Channel node"<<std::endl;
}
void ASTPrintVisitor::visit(ProjectionConstructorType *pct){
  std::cout<<__FILE__<<" Visited ProjectionConstructorType node"<<std::endl;
}
void ASTPrintVisitor::visit(InitializeType *it){
  std::cout<<__FILE__<<" Visited InitializeType node"<<std::endl;
}
void ASTPrintVisitor::visit(BoolType *bt){
  std::cout<<__FILE__<<" Visited BoolType node"<<std::endl;
}
void ASTPrintVisitor::visit(DoubleType *dt){
  std::cout<<__FILE__<<" Visited DoubleType node"<<std::endl;
}
void ASTPrintVisitor::visit(FloatType *ft){
  std::cout<<__FILE__<<" Visited FloatType node"<<std::endl;
}

void ASTPrintPass::do_pass(Project * tree){

  ASTPrintVisitor *astprint= new ASTPrintVisitor();
  std::cout<<__FILE__<<" printing tree"<<std::endl;		 

  // ASTPrintVisitor Test 1 - visit Module, Requires, Rpcs, ReturnVariable, Type //
  // --------------------------------------------------------------------------- //

  for (auto module: *tree) {

    // Visiting the module
    module->accept(astprint);

    // Visiting the outer/base lexical scope of the module
    LexicalScope* module_scope = module->module_scope_;
    LexicalScope* module_outer_scope = module_scope->outer_scope_;
    LexicalScope* module_global_scope = module_scope->globalScope;
    std::vector<LexicalScope*> module_inner_scopes = module_scope->inner_scopes_;
    module_outer_scope->accept(astprint);

    // Visiting the types in the lexical scope
    std::cout<<__FILE__<<" Visiting the types in the module (outer/base) scope."<<std::endl;		 
    std::map<std::string, Type*> module_types = module_outer_scope->type_definitions_; 
   // std::map<std::string, Type*> module_types = module_global_scope->type_definitions_; 
   // std::map<std::string, Type*> module_types = module_scope->type_definitions_; 
  int count=1;
//  for (auto module_inner_scope : module_inner_scopes){
    std::cout<<"Inner scope # "<<count++<<std::endl;   
//    std::map<std::string, Type*> module_types = module_inner_scope->type_definitions_; 
    for (auto type: module_types) {
      std::cout<<__FILE__<<" Visiting type node: "<<type.first<<std::endl;		 
      type.second->accept(astprint);
    }
 // }

    // Visiting require nodes of module
    std::vector<Require*> module_requires; 
    module_requires = module->requires();
    for (auto rq: module_requires) {
      rq->accept(astprint);
    }

    // Visiting rpcs of module	
    std::vector<Rpc*> module_rpcs; 
    module_rpcs=module->rpc_definitions();
    for (auto rp: module_rpcs) {

      // Visiting each rpc's type node (which is the return variable)
      Type *type;
      ReturnVariable *rv;
      rp->accept(astprint);
      rv=rp->return_variable();
      rv->accept(astprint);
      type=rv->type();
      type->accept(astprint);
    }
  }

  // ASTPrintVisitor Test 2 - visit Include //
  // -------------------------------------- //

  std::vector<Include*> project_includes = tree->includes();
  for (auto incl: project_includes) {
    incl->accept(astprint);	
  }

  // ASTPrintVisitor Test 3 - vist Node //
  // ---------------------------------- //
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

