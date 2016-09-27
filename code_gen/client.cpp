#include "ccst.h"
#include "code_gen.h"
#include "utils.h"
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

CCSTFile* generate_client_header(Module* mod)
{
  std::vector<CCSTExDeclaration*> definitions;
  if (mod->rpc_definitions().empty()) {
    return NULL;
  }
  std::vector<Rpc*> rpcs = mod->rpc_definitions();
  for (std::vector<Rpc*>::iterator it = rpcs.begin(); it != rpcs.end();
      it++) {
    Rpc *r = *it;
    // print definitions of callee functions, all other functions are
    // already declared in the kernel
    if (r->function_pointer_defined()) {
      definitions.push_back(callee_declaration(r));
    }
  }
  return new CCSTFile(definitions);
}

CCSTFile* generate_client_source(Module* f, std::vector<Include*> includes)
{ 
  std::vector<CCSTExDeclaration*> definitions;

  // Includes
  for(std::vector<Include*>::iterator it = includes.begin(); it != includes.end(); it ++) {
    Include *inc = *it;
    definitions.push_back(new CCSTPreprocessor(inc->get_path(), inc->is_relative()));
  }


  // declare globals
  std::vector<GlobalVariable*> globals = f->channels(); // if you odn't do this you get use after free heap issues

  for(std::vector<GlobalVariable*>::iterator it = globals.begin(); it != globals.end(); it ++) {
    GlobalVariable *gv = (GlobalVariable*) *it;
    definitions.push_back(declare_static_variable(gv));
  }

  // declare cspaces
  std::vector<GlobalVariable*> cspaces = f->cspaces_;
  for(std::vector<GlobalVariable*>::iterator it = cspaces.begin(); it != cspaces.end(); it ++) {
    GlobalVariable *gv = *it;
    definitions.push_back(declare_static_variable(gv));
  }

  // declare channel group
  definitions.push_back(declare_static_variable(f->channel_group));

  // create initialization function
  definitions.push_back(function_definition(interface_init_function_declaration(f)
					    , caller_interface_init_function_body(f)));
  
  // create exit function
  definitions.push_back(function_definition(interface_exit_function_declaration(f)
					    , caller_interface_exit_function_body(f)));

  // define container structs
  


  // functions
  std::vector<Rpc*> rpcs = f->rpc_definitions();
  for(std::vector<Rpc*>::iterator it = rpcs.begin(); it != rpcs.end(); it ++) {
    Rpc *r_tmp = (Rpc*) *it;
    if(r_tmp->function_pointer_defined()) {
      std::cout << "function pointer defined function\n";
      definitions.push_back(function_definition(callee_declaration(r_tmp)
						, callee_body(r_tmp, f)));
    } else {
      definitions.push_back(function_definition(function_declaration(r_tmp)
                                                , caller_body(r_tmp, f)));
    }
  }
  
  return new CCSTFile(definitions);
}

std::vector<CCSTDeclaration*> declare_containers(Variable *v)
{
  std::vector<CCSTDeclaration*> declarations;
  
  if(v->container() != 0x0) {
    declarations.push_back(declare_variable(v->container()));
  }

  if(v->type()->num() == PROJECTION_TYPE || v->type()->num() == PROJECTION_CONSTRUCTOR_TYPE) {
    ProjectionType *pt = dynamic_cast<ProjectionType*>(v->type());
    Assert(pt != 0x0, "Error: dynamic cast to projection type failed.\n");

    for (auto pf : *pt) {
      if(pf->container() != 0x0) {
	std::vector<CCSTDeclaration*> tmp = declare_containers(pf);
	declarations.insert(declarations.end(), tmp.begin(), tmp.end());
      }
    }
  }
  
  return declarations;
}

/*
 * use module to get things like channels and cspaces.
 * or add channel and cspace as a field to an rpc....
 * that way each rpc can have its own channel or something....
 */
CCSTCompoundStatement* caller_body(Rpc *r, Module *m)
{
  std::vector<CCSTDeclaration*> declarations;
  std::vector<CCSTStatement*> statements;
  // allocate necessary container things
  
  /* code that loops through parameters and allocates/initializes whatever necessary before marshalling*/
  
  // loop through params, declare a tmp and pull out marshal value
  std::vector<Parameter*> params = r->parameters();

  
  const char* cspace_to_use;
  if(r->function_pointer_defined()) { // cspace is 1st hidden arg
    cspace_to_use = r->hidden_args_.at(0)->identifier();
  } else {
    cspace_to_use =  m->cspaces_.at(0)->identifier();
  }
				     
  // for every parameter that has a container. declare containers. then alloc or container of
  for (auto p : *r) {
      if(p->container() != 0x0) {
	// declare containers
	std::vector<CCSTDeclaration*> tmp = declare_containers(p);
	declarations.insert(declarations.end(), tmp.begin(), tmp.end());

	statements.push_back(alloc_link_container_caller(p, cspace_to_use));

      }
    }

  /* projection channel allocation */
  for (auto p : *r) {
    if(p->type_->num() == PROJECTION_TYPE || p->type_->num() == PROJECTION_CONSTRUCTOR_TYPE) { // if a projection
      ProjectionType *pt = dynamic_cast<ProjectionType*>(p->type_);
      Assert(pt != 0x0, "Error: dynamic cast to projection type failed\n");
      std::vector<CCSTStatement*> tmp_statements = caller_allocate_channels(pt);
      statements.insert(statements.end(), tmp_statements.begin(), tmp_statements.end());
    }
  }

  // projection channel initialization
  for (auto p : *r) {
    if((p->type_->num() == PROJECTION_TYPE || p->type_->num() == PROJECTION_CONSTRUCTOR_TYPE) && p->alloc_caller()) {
      ProjectionType *pt = dynamic_cast<ProjectionType*>(p->type_);
      Assert(pt != 0x0, "Error: dynamic cast to projection type failed\n");
      std::vector<CCSTStatement*> tmp_statements = caller_initialize_channels(pt);
      statements.insert(statements.end(), tmp_statements.begin(), tmp_statements.end());
    }
  }

  
  // 

  /* TODO: what about function pointers */
  
  /* marshal parameters */
  for (auto p : *r) {
    if(p->in()) {
      std::cout << "going to marshal variable " << p->identifier() <<  " for function " <<  r->name() << std::endl;
      statements.push_back(marshal_variable(p, "in"));    
    }
  }
  
  /* if it is a function pointer need to marshal hidden args */
  if (r->function_pointer_defined()) {
    std::vector<Parameter*> hidden_args = r->hidden_args_;
    for(std::vector<Parameter*>::iterator it = hidden_args.begin(); it != hidden_args.end(); it ++) {
      Parameter *p = *it;
      if(p->in()) {
	std::cout << "going to marshal hidden arg " << p->identifier() << " for function " <<  r->name() << std::endl;
	statements.push_back(marshal_variable(p, "in"));
      }
    }
  }

  /* marshal function tag */
  std::string *func_type = new std::string(r->name());
  std_string_toupper(*func_type);
  statements.push_back(marshal(new CCSTEnumConst(func_type->c_str()), 0));

  /* make remote call using appropriate channel */

  std::vector<CCSTInitDeclarator*> err;
  err.push_back(new CCSTDeclarator(0x0, new CCSTDirectDecId("err")));
  declarations.push_back(new CCSTDeclaration(int_type(), err));

  std::vector<CCSTAssignExpr*> lcd_sync_call_args;
  lcd_sync_call_args.push_back(new CCSTPrimaryExprId(m->channels().at(0)->identifier())); // first channel
  statements.push_back(new CCSTExprStatement( new CCSTAssignExpr( new CCSTPrimaryExprId("err"), equals(), function_call("lcd_sync_call", lcd_sync_call_args))));

  statements.push_back(if_cond_fail(new CCSTPrimaryExprId("err"), "lcd_sync_call"));
  

  /* unmarshal appropriate parameters and return value */
  for (auto& p : *r) {
    if(p->type()->num() != VOID_TYPE) {
      if(p->out()) {
	std::vector<CCSTStatement*> tmp_stmts = unmarshal_variable_caller(p);
	statements.insert(statements.end(), tmp_stmts.begin(), tmp_stmts.end());

	// unmarshal container things associated with this param
	tmp_stmts = unmarshal_container_refs_caller(p);
	statements.insert(statements.end(), tmp_stmts.begin(), tmp_stmts.end());
      }
    }
  }

  /* if function pointer defined unmarshal hidden args*/
  if(r->function_pointer_defined()) {
    std::vector<Parameter*> hidden_args = r->hidden_args_;
    for(std::vector<Parameter*>::iterator it = hidden_args.begin(); it != hidden_args.end(); it ++) {
      Parameter *p = *it;
      if(p->out()) {
	std::vector<CCSTStatement*> tmp_stmts = unmarshal_variable_caller(p);
	statements.insert(statements.end(), tmp_stmts.begin(), tmp_stmts.end());
      }
    }
  }

  // if anything is marked dealloc. dealloc
  for (auto p : *r) {
    std::vector<CCSTStatement*> tmp_statements = dealloc_containers_caller(p, cspace_to_use, r->current_scope());
    statements.insert(statements.end(), tmp_statements.begin(), tmp_statements.end());
  }

  /* Todo:  clear capability registers? */

  /* return value to caller */
  if(r->return_variable()->type()->num() != VOID_TYPE) {
    // declare return var.
    declarations.push_back(declare_variable(r->return_variable()));

    // unmarshal return var
    std::vector<CCSTStatement*> tmp_stmts = unmarshal_variable_no_check(r->return_variable());
    statements.insert(statements.end(), tmp_stmts.begin(), tmp_stmts.end());
    statements.push_back(new CCSTReturn(new CCSTPrimaryExprId(r->return_variable()->identifier())));
  } else {
    statements.push_back(new CCSTReturn());
  }
  
  return new CCSTCompoundStatement(declarations, statements);  
}
