// ========================================================================= //
// IDL Compiler Infrastructure for LCDs					     //
// ========================================================================= //

// derive_specs.cpp:
// ============
// This file implements the ASTDeriveSpecsVisitor class.

#include <Passes/astprint.h>
#include <lcd_idl.h>
#include <stdio.h>

std::vector<Rpc *> module_rpcs;

// Map of projection types that are allocated by function-pointer rpcs
std::map<std::string, Variable *> projtypes_in_fp_rpcs;

// Map of projection types that are allocated by non-function-pointer rpcs
std::map<std::string, Variable *> projtypes_in_normal_rpcs;

// Map info:
// <Name of Projection real type, <the projection itself, allocation_by_fp?>
std::map<std::string, std::map<ProjectionType *, bool>>
    projection_allocation_info;

void print_proj_alloc_info() {
  std::cout << "Printing projection allocation info" << std::endl;
  std::map<ProjectionType *, bool> projection_extract;
  for (auto full_entry : projection_allocation_info) {
    std::cout << "Real type of projection: " << full_entry.first << std::endl;
    projection_extract = full_entry.second;
    for (auto entry : projection_extract) {
      std::cout << (entry.first)->name() << ": " << entry.second << std::endl;
    }
  }
}

void print_var_specifiers(Variable *var) {
  std::cout << __FILE__ << " ----------------------in: " << var->in()
            << std::endl;
  std::cout << __FILE__ << " ----------------------out: " << var->out()
            << std::endl;
  std::cout << __FILE__
            << " ----------------------alloc_caller: " << var->alloc_caller()
            << std::endl;
  std::cout << __FILE__
            << " ----------------------alloc_callee: " << var->alloc_callee()
            << std::endl;
  std::cout << __FILE__ << " ----------------------dealloc_caller: "
            << var->dealloc_caller() << std::endl;
  std::cout << __FILE__ << " ----------------------dealloc_callee: "
            << var->dealloc_callee() << std::endl;
  std::cout << __FILE__
            << " ----------------------bind_caller: " << var->bind_caller()
            << std::endl;
  std::cout << __FILE__
            << " ----------------------bind_callee: " << var->bind_callee()
            << std::endl;
  std::cout << __FILE__
            << " ----------------------fp_access: " << var->fp_access()
            << std::endl;
}

ASTDeriveSpecsVisitor::ASTDeriveSpecsVisitor() : updated_projtype_maps(false) {}

void ASTDeriveSpecsVisitor::visit(Project *p) {
  std::cout << __FILE__ << " Visited Project node" << std::endl;
}

void ASTDeriveSpecsVisitor::visit(Module *m) {
  std::cout << __FILE__ << " Visited Module node: " << m->identifier()
            << std::endl;
}

void ASTDeriveSpecsVisitor::visit(Include *i) {
  std::cout << __FILE__ << " Visited Include node with path: " << i->get_path()
            << std::endl;
}

void ASTDeriveSpecsVisitor::visit(Require *rq) {
  std::cout << __FILE__ << " Visited Require node that requires module: "
            << rq->get_required_module_name() << std::endl;
}

// This function processes projection type variables (parameters and return
// variables) of functions and function pointers
void process_projection_variable(Variable *proj_var,
                                 ASTDeriveSpecsVisitor *derive_specs) {

  // Keep a projection type version handly, it would be needed throughout the
  // fn.
  ProjectionType *pt = dynamic_cast<ProjectionType *>(proj_var->type());
  // if this projection variable has an allocation specifier,
  // save its projectin type, unless it has already been saved earlier
  if (proj_var->alloc_callee() || proj_var->alloc_caller()) {
    std::pair<std::string, Variable *> elem =
        std::make_pair(pt->real_type(), proj_var);
    if (proj_var->fp_access()) {
      if (projtypes_in_fp_rpcs.find(pt->real_type()) ==
          projtypes_in_fp_rpcs.end()) { // this check is important because we'll
                                        // be doing this pass multiple times. We
                                        // don't want to do a double insert.
        projtypes_in_fp_rpcs.insert(elem);
        std::cout << __FILE__ << " inserted projection info in fp_rpcs map"
                  << std::endl;
        derive_specs->updated_projtype_maps = true;
      }
    }
    if (proj_var->fp_access() == false) {
      if (projtypes_in_normal_rpcs.find(pt->real_type()) ==
          projtypes_in_normal_rpcs
              .end()) { // this check is important because we'll
                        // be doing this pass multiple times.
        projtypes_in_normal_rpcs.insert(elem);
        std::cout << __FILE__ << " inserted projection info in normal_rpcs map"
                  << std::endl;
        derive_specs->updated_projtype_maps = true;
      }
    }
  } else { // No allocation info is associated with this variable, need to
           // lookup the allocation info from the maps for the projection type
           // of this variable. If even the maps don't have it, it is because we
           // haven't added the allocation info into the map yet. Hence will
           // have to run this pass multiple times.
    std::map<std::string, Variable *>::iterator it;
    if (projtypes_in_fp_rpcs.find(pt->real_type()) !=
        projtypes_in_fp_rpcs.end()) {
      std::cout
          << __FILE__
          << " Allocation info found in fp rpc map for this projection variable"
          << std::endl;
      it = projtypes_in_fp_rpcs.find(pt->real_type());
      if (proj_var->fp_access()) {
        if (it->second->alloc_callee())
          proj_var->set_bind_callee(true);
        if (it->second->alloc_caller())
          proj_var->set_bind_caller(true);
      }
      if (proj_var->fp_access() == false) {
        std::cout << __FILE__ << " got variable in fp_rpcs_map " << it->second
                  << std::endl;
        if (it->second->alloc_callee())
          proj_var->set_bind_caller(true);
        if (it->second->alloc_caller())
          proj_var->set_bind_callee(true);
      }
    } else if (projtypes_in_normal_rpcs.find(pt->real_type()) !=
               projtypes_in_normal_rpcs.end()) {
      std::cout << __FILE__
                << " Allocation info found in normal rpc map for this "
                   "projection variable"
                << std::endl;
      it = projtypes_in_normal_rpcs.find(pt->real_type());
      if (proj_var->fp_access()) {
        if (it->second->alloc_callee())
          proj_var->set_bind_caller(true);
        if (it->second->alloc_caller())
          proj_var->set_bind_callee(true);
      }
      if (proj_var->fp_access() == false) {
        if (it->second->alloc_callee())
          proj_var->set_bind_callee(true);
        if (it->second->alloc_caller())
          proj_var->set_bind_caller(true);
      }
    } else if (projtypes_in_normal_rpcs.find(pt->real_type()) ==
                   projtypes_in_normal_rpcs.end() &&
               projtypes_in_fp_rpcs.find(pt->real_type()) ==
                   projtypes_in_fp_rpcs.end()) {
      // This projection's type was not found in the maps, nor was
      // it allocated explicitly in the IDL.
      if (derive_specs->final_pass == false) {
        std::cout
            << __FILE__
            << " No allocation info found in maps for this projection variable"
            << std::endl;

        // Let's check now if it has any parent projections, which has info to
        // pass to it.
        ProjectionField *field_var = dynamic_cast<ProjectionField *>(proj_var);
        if (field_var != 0x0) {
          std::cout << __FILE__ << " " << __func__
                    << " We have a child projection" << std::endl;
          proj_var->set_in(field_var->parent_proj_var_->in());
          proj_var->set_out(field_var->parent_proj_var_->out());
          proj_var->set_alloc_callee(
              field_var->parent_proj_var_->alloc_callee());
          proj_var->set_alloc_caller(
              field_var->parent_proj_var_->alloc_caller());
          proj_var->set_dealloc_callee(
              field_var->parent_proj_var_->dealloc_callee());
          proj_var->set_dealloc_caller(
              field_var->parent_proj_var_->dealloc_caller());
          proj_var->set_bind_callee(field_var->parent_proj_var_->bind_callee());
          proj_var->set_bind_caller(field_var->parent_proj_var_->bind_caller());
          std::pair<std::string, Variable *> elem =
              std::make_pair(pt->real_type(), proj_var);
          if (proj_var->fp_access()) {
            if (projtypes_in_fp_rpcs.find(pt->real_type()) ==
                projtypes_in_fp_rpcs
                    .end()) { // this check is important because we'll
                              // be doing this pass multiple times. We
                              // don't want to do a double insert.
              projtypes_in_fp_rpcs.insert(elem);
              std::cout << __FILE__
                        << " inserted projection info in fp_rpcs map"
                        << std::endl;
              derive_specs->updated_projtype_maps = true;
            }
          }
          if (proj_var->fp_access() == false) {
            if (projtypes_in_normal_rpcs.find(pt->real_type()) ==
                projtypes_in_normal_rpcs
                    .end()) { // this check is important because we'll
                              // be doing this pass multiple times.
              projtypes_in_normal_rpcs.insert(elem);
              std::cout << __FILE__
                        << " inserted projection info in normal_rpcs map"
                        << std::endl;
              derive_specs->updated_projtype_maps = true;
            }
          }
        }
      } else if (derive_specs->final_pass == true) {
        // the allocation info for the projection variable was not found, and
        // all the passes have been done, then we need to make this final pass
        // to bind them.
        // TODO: For now we shall always bind them to the callee side,
        // regardless of how they are used (i.e. as return variable, or function
        // argument).
        proj_var->set_bind_callee(true);
      }
    }
  }
}

void ASTDeriveSpecsVisitor::visit(Parameter *p) {
  std::cout << __FILE__ << " Visited Parameter: " << p->identifier()
            << std::endl;
  print_var_specifiers(p);
  if (p->type()->num() == PROJECTION_TYPE ||
      p->type()->num() == PROJECTION_CONSTRUCTOR_TYPE) {
    std::cout << __FILE__ << " This param is a projection type" << std::endl;
    process_projection_variable(p, this);
    std::cout << __FILE__ << " Specifiers after processing it. The param: "
              << p->identifier() << std::endl;
    print_var_specifiers(p);
    ProjectionType *pt = dynamic_cast<ProjectionType *>(p->type());

    // the propagation of specifiers would begin from here.
    pt->var_version = p;
    pt->accept(this);
  }
  if (p->type()->num() == FUNCTION_TYPE) {
    std::cout << __FILE__ << " This parameter is a Function, expanding it"
              << std::endl;
    Function *f = dynamic_cast<Function *>(p->type());
    f->accept(this);
  }
}

void ASTDeriveSpecsVisitor::visit(Rpc *r) {
  std::cout << __FILE__ << " Visited Rpc node: " << r->name() << std::endl;
  // go over parameters
  for (auto p : *r) {
    std::cout << __FILE__ << " about to visit parameter " << p->identifier()
              << " Set its fp_access to " << r->function_pointer_defined()
              << std::endl;
    p->fp_access_ = r->function_pointer_defined();
    p->accept(this);
  }
  // go thru return variable
  std::cout << __FILE__ << " about to visit return variable "
            << r->return_variable()->identifier() << " Set its fp_access to "
            << r->function_pointer_defined() << std::endl;
  r->return_variable()->fp_access_ = r->function_pointer_defined();
  r->return_variable()->accept(this);
}

void ASTDeriveSpecsVisitor::visit(ReturnVariable *rv) {
  std::cout << __FILE__ << " Visited ReturnVariable: " << rv->name_
            << std::endl;
  if (rv->type()->num() == PROJECTION_TYPE ||
      rv->type()->num() == PROJECTION_CONSTRUCTOR_TYPE) {
    print_var_specifiers(rv);
    process_projection_variable(rv, this);
    std::cout << __FILE__ << " Specifiers after processing it. The param: "
              << rv->identifier() << std::endl;
    print_var_specifiers(rv);
    ProjectionType *pt = dynamic_cast<ProjectionType *>(rv->type());

    // the propagation of specifiers would begin from here.
    pt->var_version = rv;
    pt->accept(this);
  }
}

void ASTDeriveSpecsVisitor::visit(Type *type) {
  std::cout << __FILE__ << " Visited Type node: " << type->name() << std::endl;
}

void ASTDeriveSpecsVisitor::visit(LexicalScope *lexical_scope) {
  std::cout << __FILE__ << " Visited Lexical Scope node" << std::endl;
}

// Visits for each type
void ASTDeriveSpecsVisitor::visit(UnresolvedType *ut) {
  std::cout << __FILE__ << "Visited UnresolvedType node" << std::endl;
}

void ASTDeriveSpecsVisitor::visit(Typedef *td) {
  std::cout << __FILE__ << "Visited Typedef node" << std::endl;
}

void ASTDeriveSpecsVisitor::visit(VoidType *vt) {
  std::cout << __FILE__ << " Visited VoidType node" << std::endl;
}

void ASTDeriveSpecsVisitor::visit(IntegerType *it) {
  std::cout << __FILE__ << " Visited IntegerType node" << std::endl;
}

void ASTDeriveSpecsVisitor::visit(ProjectionField *field) {
  std::cout << __FILE__ << " Visited ProjectionField node (field name): "
            << field->field_name_ << std::endl;
  std::cout << __FILE__ << " ProjectionField type name                : "
            << field->type_->name() << std::endl;
  if (field->type()->num() == PROJECTION_TYPE ||
      field->type()->num() == PROJECTION_CONSTRUCTOR_TYPE) {
    print_var_specifiers(field);
    std::cout << __FILE__ << " This field is a Projection Type, expanding it"
              << std::endl;
    ProjectionType *pt = dynamic_cast<ProjectionType *>(field->type());
    pt->var_version = field;
    pt->accept(this);
  }
  if (field->type()->num() == FUNCTION_TYPE) {
    std::cout << __FILE__ << " This parameter is a Function, expanding it"
              << std::endl;
    Function *f = dynamic_cast<Function *>(field->type());
    f->accept(this);
  }
}

void ASTDeriveSpecsVisitor::visit(ProjectionType *pt) {
  std::cout << __FILE__ << " Visited ProjectionType node: " << pt->name()
            << std::endl;
  std::cout << __FILE__ << " real_type: " << pt->real_type() << std::endl;
  std::cout << __FILE__ << " number of fields: " << pt->fields().size()
            << std::endl;

  for (auto field : pt->fields()) {
    if (field->type()->num() == PROJECTION_TYPE ||
        field->type()->num() == PROJECTION_CONSTRUCTOR_TYPE) {
      std::cout << __FILE__ << " field " << field->identifier()
                << " is a projection child of " << pt->name() << std::endl;
      field->parent_proj_var_ = pt->var_version;
      field->fp_access_ = field->parent_proj_var_->fp_access();
      process_projection_variable(field, this);
      std::cout << __FILE__
                << " Specifiers after processing it. The projection variable: "
                << field->identifier() << std::endl;
      print_var_specifiers(field);
    }
    field->accept(this);
  }
}

void ASTDeriveSpecsVisitor::visit(Function *fp) {
  std::cout << __FILE__ << " Visited Function node " << fp->name() << std::endl;
  // go over parameters
  for (auto p : *fp) {
    p->fp_access_ = true;
    p->accept(this);
  }
  // go thru return variable
  fp->return_var_->fp_access_ = true;
  fp->return_var_->accept(this);
}

void ASTDeriveSpecsVisitor::visit(Channel *c) {
  std::cout << __FILE__ << " Visited Channel node" << std::endl;
}

void ASTDeriveSpecsVisitor::visit(ProjectionConstructorType *pct) {
  std::cout << __FILE__ << " Visited ProjectionConstructorType node"
            << std::endl;
}

void ASTDeriveSpecsVisitor::visit(InitializeType *it) {
  std::cout << __FILE__ << " Visited InitializeType node" << std::endl;
}

void ASTDeriveSpecsVisitor::visit(BoolType *bt) {
  std::cout << __FILE__ << " Visited BoolType node" << std::endl;
}

void ASTDeriveSpecsVisitor::visit(DoubleType *dt) {
  std::cout << __FILE__ << " Visited DoubleType node" << std::endl;
}

void ASTDeriveSpecsVisitor::visit(FloatType *ft) {
  std::cout << __FILE__ << " Visited FloatType node" << std::endl;
}

void ASTDeriveSpecsPass::visit_node(Project *tree,
                                    ASTDeriveSpecsVisitor *derive_specs) {
  std::vector<VisitNode *> nodes;
  nodes.push_back(tree);
  for (auto m : *tree)
    nodes.push_back(m);
  /*Put other types of nodes in "nodes" vector to test below*/
  for (auto node : nodes)
    node->accept(derive_specs);
}

void ASTDeriveSpecsPass::visit_module_requires(
    Module *module, ASTDeriveSpecsVisitor *derive_specs) {
  std::vector<Require *> module_requires;
  module_requires = module->requires();
  for (auto rq : module_requires) {
    rq->accept(derive_specs);
  }
}

void ASTDeriveSpecsPass::visit_module_rpcs(
    Module *module, ASTDeriveSpecsVisitor *derive_specs) {
  std::cout << "in " << __func__ << std::endl;
  module_rpcs = module->rpc_definitions();

  // visit each non-fp rpc
  for (auto rpc : module_rpcs) {
    if (!rpc->function_pointer_defined()) {
      // Type *type;
      // ReturnVariable *rv;
      rpc->accept(derive_specs);
      // rv = rpc->return_variable();
      // rv->accept(derive_specs);
      // type = rv->type();
      // type->accept(derive_specs);
    }
  }
}

void ASTDeriveSpecsPass::visit_project_includes(
    Project *tree, ASTDeriveSpecsVisitor *derive_specs) {
  std::vector<Include *> project_includes = tree->includes();
  for (auto incl : project_includes) {
    incl->accept(derive_specs);
  }
}

void ASTDeriveSpecsPass::visit_scope_types(
    LexicalScope *scope, ASTDeriveSpecsVisitor *derive_specs) {
  std::map<std::string, Type *> module_types = scope->type_definitions_;
  for (auto type : module_types) {
    std::cout << __FILE__ << " Visiting type node: " << type.first << std::endl;
    type.second->accept(derive_specs);
  }
}

void ASTDeriveSpecsPass::visit_module_scope_types(
    Module *module, ASTDeriveSpecsVisitor *derive_specs) {
  std::cout << __FILE__ << " Visiting scope of module: " << module->module_name_
            << std::endl;
  visit_scope_types(module->module_scope_, derive_specs);
}

void ASTDeriveSpecsPass::visit_module_outerscope_types(
    Module *module, ASTDeriveSpecsVisitor *derive_specs) {
  std::cout << __FILE__
            << " Visiting outer scope of module: " << module->module_name_
            << std::endl;
  visit_scope_types(module->module_scope_->outer_scope_, derive_specs);
}

void ASTDeriveSpecsPass::visit_module_globalscope_types(
    Module *module, ASTDeriveSpecsVisitor *derive_specs) {
  std::cout << __FILE__
            << " Visiting global scope of module: " << module->module_name_
            << std::endl;
  visit_scope_types(module->module_scope_->globalScope, derive_specs);
}

void ASTDeriveSpecsPass::visit_module_innerscopes_types(
    Module *module, ASTDeriveSpecsVisitor *derive_specs) {
  std::cout << __FILE__
            << " Visiting inner scopes of module: " << module->module_name_
            << std::endl;
  std::vector<LexicalScope *> inner_scopes =
      module->module_scope_->inner_scopes_;
  for (auto scope : inner_scopes) {
    int count = 1;
    std::cout << "Inner scope # " << count++ << std::endl;
    visit_scope_types(scope, derive_specs);
  }
}

void ASTDeriveSpecsPass::do_pass(Project *tree) {

  ASTDeriveSpecsVisitor *derive_specs = new ASTDeriveSpecsVisitor();
  std::cout << __FILE__ << " printing tree" << std::endl;

  visit_project_includes(tree, derive_specs);

  for (auto module : *tree) {
    // Visiting the module
    // module->accept(derive_specs);
    // visit_module_scope_types(module, derive_specs);
    // visit_module_outerscope_types(module, derive_specs);
    // visit_module_globalscope_types(module, derive_specs);
    // visit_module_innerscopes_types(module, derive_specs);
    // visit_module_requires(module, derive_specs);
    int count = 1;
    this->visit_module_rpcs(module, derive_specs);
    std::cout << "Performed " << count << " derive specs pass on AST."
              << std::endl;
    while (derive_specs->updated_projtype_maps == true) {
      derive_specs->updated_projtype_maps = false;
      count++;
      std::cout << "Performed " << count << " derive specs pass on AST."
                << std::endl;
      this->visit_module_rpcs(module, derive_specs);
    }
    // At this stage we have performed all passes where we have added
    // allocation info for all projections. There are also no further
    // allocation info from the idl for any projection type to add to the maps.
    // However, there may be some projections that have structures that are not
    // even defined in the IDL, and thus we would not find their allocation
    // info from the maps. We shall assume these projections to have static
    // structures. We shall assume they are allocated on the callee side of the
    // function from which they are returned, and on the caller side of the
    // function to which they are sent as an argument. We therefore bind these
    // projections to the appropriate side accordingly.
    std::cout << "Performing final pass, to bind projection variables that "
                 "have been unresolved."
              << std::endl;
    derive_specs->final_pass = true;
    this->visit_module_rpcs(module, derive_specs);
  }
}
