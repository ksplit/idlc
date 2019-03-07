// ========================================================================= //
// IDL Compiler Infrastructure for LCDs					     //
// ========================================================================= //

// astprint.cpp:
// ============
// This file implements the ASTPrintVisitor class.

#include <Passes/astprint.h>
#include <stdio.h>

ASTPrintVisitor::ASTPrintVisitor() {}

void ASTPrintVisitor::visit(Project *p) {
  std::cout << __FILE__ << " Visited Project node" << std::endl;
}

void ASTPrintVisitor::visit(Module *m) {
  std::cout << __FILE__ << " Visited Module node: " << m->identifier()
            << std::endl;
}

void ASTPrintVisitor::visit(Include *i) {
  std::cout << __FILE__ << " Visited Include node with path: " << i->get_path()
            << std::endl;
}

void ASTPrintVisitor::visit(Require *rq) {
  std::cout << __FILE__ << " Visited Require node that requires module: "
            << rq->get_required_module_name() << std::endl;
}

void ASTPrintVisitor::visit(Rpc *rp) {
  std::cout << __FILE__ << " Visited Rpc node: " << rp->name() << std::endl;
}

void ASTPrintVisitor::visit(ReturnVariable *rv) {
  std::cout << __FILE__ << " Visited ReturnVariable: " << rv->name_
            << std::endl;
  std::cout << __FILE__ << " ----------------------in: " << rv->in_
            << std::endl;
  std::cout << __FILE__ << " ----------------------out: " << rv->out_
            << std::endl;
  std::cout << __FILE__
            << " ----------------------alloc_caller: " << rv->alloc_caller_
            << std::endl;
  std::cout << __FILE__
            << " ----------------------alloc_callee: " << rv->alloc_callee_
            << std::endl;
}

void ASTPrintVisitor::visit(Type *type) {
  std::cout << __FILE__ << " Visited Type node: " << type->name() << std::endl;
}

void ASTPrintVisitor::visit(LexicalScope *lexical_scope) {
  std::cout << __FILE__ << " Visited Lexical Scope node" << std::endl;
}

// Visits for each type
void ASTPrintVisitor::visit(UnresolvedType *ut) {
  std::cout << __FILE__ << "Visited UnresolvedType node" << std::endl;
}

void ASTPrintVisitor::visit(Typedef *td) {
  std::cout << __FILE__ << "Visited Typedef node" << std::endl;
}

void ASTPrintVisitor::visit(VoidType *vt) {
  std::cout << __FILE__ << " Visited VoidType node" << std::endl;
}

void ASTPrintVisitor::visit(IntegerType *it) {
  std::cout << __FILE__ << " Visited IntegerType node" << std::endl;
}

void ASTPrintVisitor::visit(ProjectionType *pt) {
  std::cout << __FILE__ << " Visited ProjectionType node. " << std::endl;
  std::cout << "real_type: " << pt->real_type_ << std::endl;
  std::vector<ProjectionField *> fields = pt->fields_;
  for (auto field : fields) {
    std::cout << __FILE__ << " Visited ProjectionField node (field name): "
              << field->field_name_ << std::endl;
    std::cout << __FILE__ << " ProjectionField type name                : "
              << field->type_->name() << std::endl;
    std::cout << __FILE__ << " ----------------------------in: " << field->in_
              << std::endl;
    std::cout << __FILE__ << " ----------------------------out: " << field->out_
              << std::endl;
    visit(field->type_);
  }
}

void ASTPrintVisitor::visit(Function *fp) {
  std::cout << __FILE__ << " Visited Function node" << std::endl;
}

void ASTPrintVisitor::visit(Channel *c) {
  std::cout << __FILE__ << " Visited Channel node" << std::endl;
}

void ASTPrintVisitor::visit(ProjectionConstructorType *pct) {
  std::cout << __FILE__ << " Visited ProjectionConstructorType node"
            << std::endl;
}

void ASTPrintVisitor::visit(InitializeType *it) {
  std::cout << __FILE__ << " Visited InitializeType node" << std::endl;
}

void ASTPrintVisitor::visit(BoolType *bt) {
  std::cout << __FILE__ << " Visited BoolType node" << std::endl;
}

void ASTPrintVisitor::visit(DoubleType *dt) {
  std::cout << __FILE__ << " Visited DoubleType node" << std::endl;
}

void ASTPrintVisitor::visit(FloatType *ft) {
  std::cout << __FILE__ << " Visited FloatType node" << std::endl;
}

void ASTPrintPass::visit_node(Project *tree, ASTPrintVisitor *astprint) {
  std::vector<VisitNode *> nodes;
  nodes.push_back(tree);
  for (auto m : *tree)
    nodes.push_back(m);
  /*Put other types of nodes in "nodes" vector to test below*/
  for (auto node : nodes)
    node->accept(astprint);
}

void ASTPrintPass::visit_module_requires(Module *module,
                                         ASTPrintVisitor *astprint) {
  std::vector<Require *> module_requires;
  module_requires = module->requires();
  for (auto rq : module_requires) {
    rq->accept(astprint);
  }
}

void ASTPrintPass::visit_module_rpcs(Module *module,
                                     ASTPrintVisitor *astprint) {
  std::vector<Rpc *> module_rpcs;
  module_rpcs = module->rpc_definitions();

  // Visiting each rpc's type node (which is the return variable)
  for (auto rp : module_rpcs) {
    Type *type;
    ReturnVariable *rv;
    rp->accept(astprint);
    rv = rp->return_variable();
    rv->accept(astprint);
    type = rv->type();
    type->accept(astprint);
  }
}

void ASTPrintPass::visit_project_includes(Project *tree,
                                          ASTPrintVisitor *astprint) {
  std::vector<Include *> project_includes = tree->includes();
  for (auto incl : project_includes) {
    incl->accept(astprint);
  }
}

void ASTPrintPass::visit_scope_types(LexicalScope *scope,
                                     ASTPrintVisitor *astprint) {
  std::map<std::string, Type *> module_types = scope->type_definitions_;
  for (auto type : module_types) {
    std::cout << __FILE__ << " Visiting type node: " << type.first << std::endl;
    type.second->accept(astprint);
  }
}

void ASTPrintPass::visit_module_scope_types(Module *module,
                                            ASTPrintVisitor *astprint) {
  std::cout << __FILE__ << " Visiting scope of module: " << module->module_name_
            << std::endl;
  visit_scope_types(module->module_scope_, astprint);
}

void ASTPrintPass::visit_module_outerscope_types(Module *module,
                                                 ASTPrintVisitor *astprint) {
  std::cout << __FILE__
            << " Visiting outer scope of module: " << module->module_name_
            << std::endl;
  visit_scope_types(module->module_scope_->outer_scope_, astprint);
}

void ASTPrintPass::visit_module_globalscope_types(Module *module,
                                                  ASTPrintVisitor *astprint) {
  std::cout << __FILE__
            << " Visiting global scope of module: " << module->module_name_
            << std::endl;
  visit_scope_types(module->module_scope_->globalScope, astprint);
}

void ASTPrintPass::visit_module_innerscopes_types(Module *module,
                                                  ASTPrintVisitor *astprint) {
  std::cout << __FILE__
            << " Visiting inner scopes of module: " << module->module_name_
            << std::endl;
  std::vector<LexicalScope *> inner_scopes =
      module->module_scope_->inner_scopes_;
  for (auto scope : inner_scopes) {
    int count = 1;
    std::cout << "Inner scope # " << count++ << std::endl;
    visit_scope_types(scope, astprint);
  }
}

void ASTPrintPass::do_pass(Project *tree) {

  ASTPrintVisitor *astprint = new ASTPrintVisitor();
  std::cout << __FILE__ << " printing tree" << std::endl;

  visit_project_includes(tree, astprint);

  for (auto module : *tree) {
    // Visiting the module
    module->accept(astprint);
    // visit_module_scope_types(module, astprint);
    // visit_module_outerscope_types(module, astprint);
    // visit_module_globalscope_types(module, astprint);
    visit_module_innerscopes_types(module, astprint);
    // visit_module_requires(module, astprint);
    visit_module_rpcs(module, astprint);
  }
}
