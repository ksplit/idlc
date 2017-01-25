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

  /// generate function declaration for sync, async loop
  definitions.push_back(dispatch_sync_function_declaration());
  definitions.push_back(dispatch_async_function_declaration(mod));

  /// generate function declaration for init, exit
  definitions.push_back(interface_init_function_declaration(mod));
  definitions.push_back(interface_exit_function_declaration(mod));

  for (auto r : *mod) {
    // print definitions of callee functions, all other functions are
    // already declared in the kernel
    if (r->function_pointer_defined()) {
      definitions.push_back(callee_declaration(r));
    }
  }
  return new CCSTFile(definitions);
}

CCSTFile* generate_client_source(Module* m, std::vector<Include*> includes)
{
  std::vector<CCSTExDeclaration*> definitions;

  // Includes
  for (std::vector<Include*>::iterator it = includes.begin();
    it != includes.end(); it++) {
    Include *inc = *it;
    definitions.push_back(
      new CCSTPreprocessor(inc->get_path(), inc->is_relative()));
  }

  // declare globals
  // if you don't do this you get use after free heap issues
  std::vector<GlobalVariable*> globals = m->channels();

  for (std::vector<GlobalVariable*>::iterator it = globals.begin();
    it != globals.end(); it++) {
    GlobalVariable *gv = (GlobalVariable*) *it;
    definitions.push_back(declare_static_variable(gv));
  }

  // declare cspaces
  std::vector<GlobalVariable*> cspaces = m->cspaces_;
  for (std::vector<GlobalVariable*>::iterator it = cspaces.begin();
    it != cspaces.end(); it++) {
    GlobalVariable *gv = *it;
    definitions.push_back(declare_static_variable(gv));
  }

  // declare channel group
  definitions.push_back(declare_static_variable(m->channel_group));

  // create initialization function
  definitions.push_back(
    function_definition(interface_init_function_declaration(m),
      caller_interface_init_function_body(m)));

  // create exit function
  definitions.push_back(
    function_definition(interface_exit_function_declaration(m),
      caller_interface_exit_function_body(m)));

  // define container structs

  // define functions
  for (auto r : *m) {
    std::cout << "generating function definition for " << r->name()
      << std::endl;
    if (r->function_pointer_defined()) {
      definitions.push_back(
        function_definition(callee_declaration(r), callee_body(r, m)));
    } else {
      definitions.push_back(
        function_definition(function_declaration(r), caller_body(r, m)));
    }
  }
  return new CCSTFile(definitions);
}

std::vector<CCSTDeclaration*> declare_containers(Variable *v)
{
  std::vector<CCSTDeclaration*> declarations;

  if (v->container()) {
    declarations.push_back(declare_variable(v->container()));
  }

  if (v->type()->num() == PROJECTION_TYPE
    || v->type()->num() == PROJECTION_CONSTRUCTOR_TYPE) {

    ProjectionType *pt = dynamic_cast<ProjectionType*>(v->type());
    Assert(pt != 0x0, "Error: dynamic cast to projection type failed.\n");

    for (auto pf : *pt) {
      if (pf->container()) {
        std::vector<CCSTDeclaration*> tmp = declare_containers(pf);
        declarations.insert(declarations.end(), tmp.begin(), tmp.end());
      }
    }
  }

  return declarations;
}

/// use module to get things like channels and cspaces.
/// or add channel and cspace as a field to an rpc....
/// that way each rpc can have its own channel or something....
CCSTCompoundStatement* caller_body(Rpc *r, Module *m)
{
  std::vector<CCSTDeclaration*> declarations;
  std::vector<CCSTStatement*> statements;
  std::string cspace_to_use;

  if (r->function_pointer_defined()) {
    // cspace is 1st hidden arg
    cspace_to_use = r->hidden_args_.at(0)->identifier();
  } else {
    cspace_to_use = m->cspaces_.at(0)->identifier();
  }
				     
  // for every parameter that has a container. declare containers. then alloc or container of
  for (auto p : *r) {
    if (p->container()) {
      // declare containers
      std::vector<CCSTDeclaration*> tmp = declare_containers(p);
      declarations.insert(declarations.end(), tmp.begin(), tmp.end());

      CCSTCompoundStatement *updates = alloc_link_container_caller(p, cspace_to_use);
      std::vector<CCSTDeclaration*> __tmp_declarations = updates->getdeclarations();
      std::vector<CCSTStatement*> __tmp_statements = updates->getstatements();

      declarations.insert(declarations.end(), __tmp_declarations.begin(), __tmp_declarations.end());
      statements.insert(statements.end(), __tmp_statements.begin(), __tmp_statements.end());
    }
  }

  /* projection channel allocation */
  for (auto p : *r) {
    if (p->type_->num() == PROJECTION_TYPE
      || p->type_->num() == PROJECTION_CONSTRUCTOR_TYPE) {

      ProjectionType *pt = dynamic_cast<ProjectionType*>(p->type_);
      Assert(pt != 0x0, "Error: dynamic cast to projection type failed\n");
      std::vector<CCSTStatement*> tmp_statements = caller_allocate_channels(
        pt);
      statements.insert(statements.end(), tmp_statements.begin(),
        tmp_statements.end());
    }
  }

  // projection channel initialization
  for (auto p : *r) {
    if ((p->type_->num() == PROJECTION_TYPE
      || p->type_->num() == PROJECTION_CONSTRUCTOR_TYPE)
      && p->alloc_caller()) {

      ProjectionType *pt = dynamic_cast<ProjectionType*>(p->type_);
      Assert(pt != 0x0, "Error: dynamic cast to projection type failed\n");
      std::vector<CCSTStatement*> tmp_statements = caller_initialize_channels(
        pt);
      statements.insert(statements.end(), tmp_statements.begin(),
        tmp_statements.end());
    }
  }
  
  /* TODO: what about function pointers */
  // Get active channel of the unnamed scope
  Channel *c = r->getcurrentscope()->outer_scope_->getactiveChannel();
  if (c) {
    std::cout << "### Rpc " << r->name() << " ac:  " << c->chName << std::endl;
  } else {
    // Check if there exists a global module level channel
    c = m->module_scope()->activeChannel;
  }

  CCSTCompoundStatement *updates;
  if (c && c->getChannelType() == Channel::ChannelType::AsyncChannel) {
    updates = async_call(r, c, cspace_to_use);
  } else {
    updates = sync_call(r, m, cspace_to_use, c);
  }

  std::vector<CCSTDeclaration*> __tmp_declarations = updates->getdeclarations();
  std::vector<CCSTStatement*> __tmp_statements = updates->getstatements();

  declarations.insert(declarations.end(), __tmp_declarations.begin(), __tmp_declarations.end());
  statements.insert(statements.end(), __tmp_statements.begin(), __tmp_statements.end());

  /* Todo:  clear capability registers? */
  /* return value to caller */
  if(r->return_variable()->type()->num() != VOID_TYPE) {
    statements.push_back(new CCSTReturn(new CCSTPrimaryExprId(r->return_variable()->identifier())));
  } else {
    statements.push_back(new CCSTReturn());
  }
  
  return new CCSTCompoundStatement(declarations, statements);
}

CCSTCompoundStatement *async_call(Rpc *r, Channel *c, std::string &cspace_to_use)
{
  std::vector<CCSTDeclaration*> declarations;
  std::vector<CCSTStatement*> statements;
  std::vector<CCSTAssignExpr*> lcd_async_start_args;
  std::vector<CCSTAssignExpr*> async_fntype_args;
  std::vector<CCSTAssignExpr*> fipc_set_0;
  std::vector<CCSTDecSpecifier*> spec_fipcm;

  std::vector<CCSTInitDeclarator*> decs_req;
  std::vector<CCSTInitDeclarator*> decs_resp;

  spec_fipcm.push_back(
    new CCSTStructUnionSpecifier(struct_t, "fipc_message"));

  decs_resp.push_back(
    new CCSTInitDeclarator(
      new CCSTDeclarator(pointer(1), new CCSTDirectDecId("response"))));

  decs_req.push_back(
    new CCSTInitDeclarator(
      new CCSTDeclarator(pointer(1), new CCSTDirectDecId("request"))));

  declarations.push_back(
    new CCSTDeclaration(spec_fipcm, decs_req));

  declarations.push_back(
    new CCSTDeclaration(spec_fipcm, decs_resp));

  if (c->getChannelType() == Channel::ChannelType::AsyncChannel) {
    lcd_async_start_args.push_back(new CCSTPrimaryExprId(c->chName));
    lcd_async_start_args.push_back(
      new CCSTUnaryExprCastExpr(reference(),
        new CCSTPrimaryExprId("request")));

    statements.push_back(
      new CCSTExprStatement(
        new CCSTAssignExpr(new CCSTPrimaryExprId("ret"), equals(),
          new CCSTPostFixExprAssnExpr(
            new CCSTPrimaryExprId("async_msg_blocking_send_start"),
            lcd_async_start_args))));

    std::string *goto_fail_async = new std::string("fail_async");
    statements.push_back(
      if_cond_fail_goto(new CCSTPrimaryExprId("ret"),
        "failed to get a send slot", *goto_fail_async));

    async_fntype_args.push_back(new CCSTPrimaryExprId("request"));
    async_fntype_args.push_back(new CCSTPrimaryExprId(r->enum_name()));

    statements.push_back(
      new CCSTExprStatement(
        new CCSTPostFixExprAssnExpr(
          new CCSTPrimaryExprId("async_msg_set_fn_type"),
          async_fntype_args)));
  }

  /* marshal parameters */
  for (auto p : *r) {
    if(p->in()) {
      std::cout << " ASYNC marshal variable " << p->identifier() <<  " for function " <<  r->name() << std::endl;
      // TODO: Unify instances like this . Refer one more below
      CCSTCompoundStatement *updates = dynamic_cast<CCSTCompoundStatement*>(marshal_variable(p, "in", c->chType));

      std::vector<CCSTDeclaration*> __tmp_declarations = updates->getdeclarations();
      std::vector<CCSTStatement*> __tmp_statements = updates->getstatements();

      declarations.insert(declarations.end(), __tmp_declarations.begin(), __tmp_declarations.end());
      statements.insert(statements.end(), __tmp_statements.begin(), __tmp_statements.end());
    }
  }
  /* if it is a function pointer need to marshal hidden args */
  if (r->function_pointer_defined()) {
    std::vector<Parameter*> hidden_args = r->hidden_args_;
    for(std::vector<Parameter*>::iterator it = hidden_args.begin(); it != hidden_args.end(); it ++) {
      Parameter *p = *it;
      if(p->in()) {
  std::cout << " ASYNC going to marshal hidden arg " << p->identifier() << " for function " <<  r->name() << std::endl;
  CCSTCompoundStatement *updates = dynamic_cast<CCSTCompoundStatement*>(marshal_variable(p, "in", c->chType));

  std::vector<CCSTDeclaration*> __tmp_declarations = updates->getdeclarations();
  std::vector<CCSTStatement*> __tmp_statements = updates->getstatements();

  declarations.insert(declarations.end(), __tmp_declarations.begin(), __tmp_declarations.end());
  statements.insert(statements.end(), __tmp_statements.begin(), __tmp_statements.end());
      }
    }
  }
  std::vector<CCSTAssignExpr*> lcd_async_call_args;

  lcd_async_call_args.push_back(new CCSTPrimaryExprId(c->chName));
  lcd_async_call_args.push_back(new CCSTPrimaryExprId("request"));
  lcd_async_call_args.push_back(new CCSTUnaryExprCastExpr(reference(), new CCSTPrimaryExprId("response")));
    statements.push_back(
      new CCSTExprStatement(
        new CCSTAssignExpr(new CCSTPrimaryExprId("ret"), equals(),
          function_call("thc_ipc_call", lcd_async_call_args))));

  std::string *goto_ipc = new std::string("fail_ipc");
  statements.push_back(if_cond_fail_goto(new CCSTPrimaryExprId("ret"), "thc_ipc_call", *goto_ipc));

  for (auto p: *r) {
    if (p->type_->num() == VOID_TYPE) {
      std::cout << " SYNC send of void pointer " << p->identifier() << " for function " <<  r->name() << std::endl;
      statements.push_back(marshal_void_delayed(p));
    }
  }

  /* unmarshal appropriate parameters and return value */
  for (auto& p : *r) {
    if(p->type()->num() != VOID_TYPE) {
      if(p->out()) {
  std::vector<CCSTStatement*> tmp_stmts = unmarshal_variable_caller(p, c->chType);
  statements.insert(statements.end(), tmp_stmts.begin(), tmp_stmts.end());

  // unmarshal container things associated with this param
  tmp_stmts = unmarshal_container_refs_caller(p, c->chType);
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
  std::vector<CCSTStatement*> tmp_stmts = unmarshal_variable_caller(p, c->chType);
  statements.insert(statements.end(), tmp_stmts.begin(), tmp_stmts.end());
      }
    }
  }

  // if anything is marked dealloc. dealloc
  for (auto p : *r) {
    std::vector<CCSTStatement*> tmp_statements = dealloc_containers_caller(p,
      cspace_to_use, r->current_scope());
    statements.insert(statements.end(), tmp_statements.begin(),
      tmp_statements.end());
  }

  if(r->return_variable()->type()->num() != VOID_TYPE) {
    // declare return var.
    declarations.push_back(declare_variable(r->return_variable()));

    // unmarshal return var
    std::vector<CCSTStatement*> tmp_stmts = unmarshal_return_variable_no_check(r->return_variable(), c->chType);
    statements.insert(statements.end(), tmp_stmts.begin(), tmp_stmts.end());
  }
  std::vector<CCSTAssignExpr*> ipc_recv_end_args;
  std::vector<CCSTAssignExpr*> chnl_to_fipc_args;
  chnl_to_fipc_args.push_back(new CCSTPrimaryExprId(c->chName));
  ipc_recv_end_args.push_back(function_call("thc_channel_to_fipc", chnl_to_fipc_args));
  ipc_recv_end_args.push_back(new CCSTPrimaryExprId("response"));
  statements.push_back(new CCSTExprStatement(function_call("fipc_recv_msg_end", ipc_recv_end_args)));
  return new CCSTCompoundStatement(declarations, statements);
}

CCSTCompoundStatement *sync_call(Rpc *r, Module *m, std::string &cspace_to_use, Channel *c)
{
  std::vector<CCSTDeclaration*> declarations;
  std::vector<CCSTStatement*> statements;
  /* marshal parameters */
  for (auto p : *r) {
    if (p->in()) {
      std::cout << "going to marshal variable " << p->identifier()
        << " for function " << r->name() << std::endl;
      statements.push_back(marshal_variable(p, "in", c->chType));
    }
  }

  /* if it is a function pointer need to marshal hidden args */
  if (r->function_pointer_defined()) {
    std::vector<Parameter*> hidden_args = r->hidden_args_;
    for (std::vector<Parameter*>::iterator it = hidden_args.begin();
      it != hidden_args.end(); it++) {
      Parameter *p = *it;
      if (p->in()) {
        std::cout << "going to marshal hidden arg " << p->identifier()
          << " for function " << r->name() << std::endl;
        statements.push_back(marshal_variable(p, "in", c->chType));
      }
    }
  }

  /* marshal function tag */
  std::string *func_type = new std::string(r->name());
  std_string_toupper(*func_type);
  statements.push_back(marshal(new CCSTEnumConst(*func_type), 0, c->chType, "request"));

  /* make remote call using appropriate channel */

  std::vector<CCSTInitDeclarator*> err;
  err.push_back(new CCSTDeclarator(0x0, new CCSTDirectDecId("err")));
  declarations.push_back(new CCSTDeclaration(int_type(), err));

  std::vector<CCSTAssignExpr*> lcd_sync_call_args;

  lcd_sync_call_args.push_back(
    new CCSTPrimaryExprId(m->channels().at(0)->identifier())); // first channel
  statements.push_back(
    new CCSTExprStatement(
      new CCSTAssignExpr(new CCSTPrimaryExprId("err"), equals(),
        function_call("lcd_sync_call", lcd_sync_call_args))));

  statements.push_back(
    if_cond_fail(new CCSTPrimaryExprId("err"), "lcd_sync_call"));

  /* unmarshal appropriate parameters and return value */
  for (auto& p : *r) {
    if (p->type()->num() != VOID_TYPE) {
      if (p->out()) {
        std::vector<CCSTStatement*> tmp_stmts = unmarshal_variable_caller(p, c->chType);
        statements.insert(statements.end(), tmp_stmts.begin(),
          tmp_stmts.end());

        // unmarshal container things associated with this param
        tmp_stmts = unmarshal_container_refs_caller(p, c->chType);
        statements.insert(statements.end(), tmp_stmts.begin(),
          tmp_stmts.end());
      }
    }
  }

  /* if function pointer defined unmarshal hidden args*/
  if (r->function_pointer_defined()) {
    std::vector<Parameter*> hidden_args = r->hidden_args_;
    for (std::vector<Parameter*>::iterator it = hidden_args.begin();
      it != hidden_args.end(); it++) {
      Parameter *p = *it;
      if (p->out()) {
        std::vector<CCSTStatement*> tmp_stmts = unmarshal_variable_caller(p, c->chType);
        statements.insert(statements.end(), tmp_stmts.begin(),
          tmp_stmts.end());
      }
    }
  }

  // if anything is marked dealloc. dealloc
  for (auto p : *r) {
    std::vector<CCSTStatement*> tmp_statements = dealloc_containers_caller(p,
      cspace_to_use, r->current_scope());
    statements.insert(statements.end(), tmp_statements.begin(),
      tmp_statements.end());
  }

  if(r->return_variable()->type()->num() != VOID_TYPE) {
    // declare return var.
    declarations.push_back(declare_variable(r->return_variable()));

    // unmarshal return var
    std::vector<CCSTStatement*> tmp_stmts = unmarshal_variable_no_check(r->return_variable(), c->chType);
    statements.insert(statements.end(), tmp_stmts.begin(), tmp_stmts.end());
  }

  return new CCSTCompoundStatement(declarations, statements);
}
