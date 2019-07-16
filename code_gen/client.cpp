#include "ccst.h"
#include "code_gen.h"
#include "utils.h"
#include <iostream>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

CCSTFile *generate_client_header(Module *mod) {
  std::vector<CCSTExDeclaration *> definitions;
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

CCSTFile *generate_client_source(Module *m, std::vector<Include *> includes) {
  std::vector<CCSTExDeclaration *> definitions;

  // Includes
  for (auto inc : includes) {
    definitions.push_back(
        new CCSTPreprocessor(inc->get_path(), inc->is_relative()));
  }

  // declare globals
  // if you don't do this you get use after free heap issues
  std::vector<GlobalVariable *> globals = m->channels();

  for (auto gv : globals) {
    definitions.push_back(declare_static_variable(gv));
  }

  // declare cspaces
  std::vector<GlobalVariable *> cspaces = m->cspaces_;
  for (auto gv : cspaces) {
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
      definitions.push_back(function_definition(function_declaration(r),
                                                caller_body(r, m, false)));
    }
  }
  return new CCSTFile(definitions);
}

std::vector<CCSTDeclaration *> declare_containers(Variable *v) {
  std::vector<CCSTDeclaration *> declarations;

  if (v->container()) {
    declarations.push_back(declare_variable(v->container()));
  }

  if (v->type()->num() == PROJECTION_TYPE ||
      v->type()->num() == PROJECTION_CONSTRUCTOR_TYPE) {

    ProjectionType *pt = dynamic_cast<ProjectionType *>(v->type());
    Assert(pt != 0x0, "Error: dynamic cast to projection type failed.\n");

    for (auto pf : *pt) {
      if (pf->container()) {
        std::vector<CCSTDeclaration *> tmp = declare_containers(pf);
        declarations.insert(declarations.end(), tmp.begin(), tmp.end());
      }
    }
  }

  return declarations;
}

/// use module to get things like channels and cspaces.
/// or add channel and cspace as a field to an rpc....
/// that way each rpc can have its own channel or something....
CCSTCompoundStatement *caller_body(Rpc *r, Module *m, bool user) {
  std::vector<CCSTDeclaration *> declarations;
  std::vector<CCSTStatement *> statements;
  std::vector<CCSTStatement *> lbl_statements;
  std::string cspace_to_use;

  if (r->function_pointer_defined()) {
    /// XXX: For function pointers, cspace is in the first arg of the
    /// hidden_args pointer. Is this the right way to access it?
    cspace_to_use =
        r->hidden_args_.at(0)->identifier() + "->" +
        dynamic_cast<ProjectionType *>(r->hidden_args_.at(0)->type())
            ->get_field("cspace")
            ->identifier();

    /// Insert this only for normal call.
    if (!user) {
      CCSTExpression *cond = new CCSTUnaryExprCastExpr(
          Not(), new CCSTPostFixExprAccess(new CCSTPrimaryExprId("current"),
                                           pointer_access_t, "ptstate"));

      std::vector<CCSTDeclaration *> if_body_declarations;
      std::vector<CCSTStatement *> if_body_statements;

      std::vector<CCSTAssignExpr *> liblcd_msg_args;
      std::vector<CCSTAssignExpr *> fncall_user_args;
      std::vector<CCSTDeclaration *> lcd_main_decls;
      std::vector<CCSTStatement *> lcd_main_stmnts;
      std::vector<CCSTAssignExpr *> lcd_main_args;
      std::vector<CCSTDeclaration *> lcdmain_decs;
      std::vector<CCSTStatement *> lcdmain_stmts;
      liblcd_msg_args.push_back(new CCSTString(
          "Calling from a non-LCD context! creating thc runtime!"));
      if_body_statements.push_back(
          new CCSTExprStatement(function_call("LIBLCD_MSG", liblcd_msg_args)));

      std::vector<Parameter *> r_parameters = r->parameters();
      std::vector<Parameter *> r_hidden_args = r->hidden_args_;

      for (auto p : r_parameters)
        fncall_user_args.push_back(new CCSTPrimaryExprId(p->identifier()));

      for (auto p : r_hidden_args)
        fncall_user_args.push_back(new CCSTPrimaryExprId(p->identifier()));

      if (r->return_variable()->type()->num() != VOID_TYPE) {
        lcdmain_stmts.push_back(new CCSTExprStatement(new CCSTAssignExpr(
            new CCSTPrimaryExprId("ret"), equals(),
            function_call(r->name() + "_user", fncall_user_args))));
      } else {
        lcdmain_stmts.push_back(new CCSTExprStatement(
            function_call(r->name() + "_user", fncall_user_args)));
      }

      if_body_statements.push_back(new CCSTMacro(
          "LCD_MAIN", new CCSTCompoundStatement(lcdmain_decs, lcdmain_stmts),
          true));

      if (r->return_variable()->type()->num() != VOID_TYPE) {
        if_body_statements.push_back(
            new CCSTReturn(new CCSTPrimaryExprId("ret")));
      } else {
        if_body_statements.push_back(new CCSTReturn());
      }

      CCSTCompoundStatement *if_body =
          new CCSTCompoundStatement(if_body_declarations, if_body_statements);
      statements.push_back(new CCSTIfStatement(cond, if_body));
    }
  } else {
    cspace_to_use = m->cspaces_.at(0)->identifier();
  }

  // for every parameter that has a container. declare containers. then alloc or
  // container of
  for (auto p : *r) {
    if (p->container()) {
      // declare containers
      std::vector<CCSTDeclaration *> tmp = declare_containers(p);
      declarations.insert(declarations.end(), tmp.begin(), tmp.end());

      CCSTCompoundStatement *updates =
          alloc_link_container_caller(p, cspace_to_use);
      std::vector<CCSTDeclaration *> __tmp_declarations =
          updates->getdeclarations();
      std::vector<CCSTStatement *> __tmp_statements = updates->getstatements();

      declarations.insert(declarations.end(), __tmp_declarations.begin(),
                          __tmp_declarations.end());
      statements.insert(statements.end(), __tmp_statements.begin(),
                        __tmp_statements.end());
    }
    if (p->type()->num() == FUNCTION_TYPE) {
      auto *p_container_type =
          dynamic_cast<ProjectionType *>(p->container()->type());
      Assert(p_container_type != 0x0,
             "Error: dynamic cast to projection type failed\n");
      auto *p_container_real_field =
          p_container_type->get_field(p->type()->name());
      Assert(p_container_real_field != 0x0,
             "Error: could not find field in structure\n");
      statements.push_back(new CCSTExprStatement(
          new CCSTAssignExpr(access(p_container_real_field), equals(),
                             new CCSTPrimaryExprId(p->type()->name()))));
    }
  }

  if (r->return_variable()->container()) {
    /// declare container variable for return value
    declarations.push_back(declare_variable(r->return_variable()->container()));

    /// Allocate only the top level container for the retun var projection
    CCSTCompoundStatement *updates =
        alloc_link_container_caller_top(r->return_variable(), cspace_to_use);
    std::vector<CCSTDeclaration *> __tmp_declarations =
        updates->getdeclarations();
    std::vector<CCSTStatement *> __tmp_statements = updates->getstatements();

    declarations.insert(declarations.end(), __tmp_declarations.begin(),
                        __tmp_declarations.end());
    statements.insert(statements.end(), __tmp_statements.begin(),
                      __tmp_statements.end());
  }

  /* projection channel allocation */
  for (auto p : *r) {
    if (p->type_->num() == PROJECTION_TYPE ||
        p->type_->num() == PROJECTION_CONSTRUCTOR_TYPE) {

      ProjectionType *pt = dynamic_cast<ProjectionType *>(p->type_);
      Assert(pt != 0x0, "Error: dynamic cast to projection type failed\n");
      std::vector<CCSTStatement *> tmp_statements =
          caller_allocate_channels(pt);
      statements.insert(statements.end(), tmp_statements.begin(),
                        tmp_statements.end());
    }
  }

  // projection channel initialization
  for (auto p : *r) {
    if ((p->type_->num() == PROJECTION_TYPE ||
         p->type_->num() == PROJECTION_CONSTRUCTOR_TYPE) &&
        p->alloc_caller()) {

      auto pt = dynamic_cast<ProjectionType *>(p->type_);
      Assert(pt != 0x0, "Error: dynamic cast to projection type failed\n");
      auto tmp_statements = caller_initialize_channels(pt);
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
    updates = async_call(r, c, cspace_to_use, user);
  } else {
    updates = sync_call(r, m, cspace_to_use, c);
  }

  std::vector<CCSTDeclaration *> __tmp_declarations =
      updates->getdeclarations();
  std::vector<CCSTStatement *> __tmp_statements = updates->getstatements();
  std::vector<CCSTStatement *> __tmp_lbl_statements =
      updates->getlblstatements();

  declarations.insert(declarations.end(), __tmp_declarations.begin(),
                      __tmp_declarations.end());
  statements.insert(statements.end(), __tmp_statements.begin(),
                    __tmp_statements.end());
  lbl_statements.insert(lbl_statements.end(), __tmp_lbl_statements.begin(),
                        __tmp_lbl_statements.end());
  if (user) {
    std::vector<CCSTAssignExpr *> lcd_exit_args;
    lcd_exit_args.push_back(new CCSTInteger(0));
    statements.push_back(
        new CCSTExprStatement(function_call("lcd_exit", lcd_exit_args)));
  }
  /* Todo:  clear capability registers? */
  /* return value to caller */
  if (r->return_variable()->type()->num() != VOID_TYPE) {
    statements.push_back(new CCSTReturn(
        new CCSTPrimaryExprId(r->return_variable()->identifier())));
  } else {
    statements.push_back(new CCSTReturn());
  }

  return new CCSTCompoundStatement(declarations, statements, lbl_statements);
}

CCSTCompoundStatement *async_call(Rpc *r, Channel *c,
                                  std::string &cspace_to_use, bool user) {
  std::vector<CCSTDeclaration *> declarations;
  std::vector<CCSTStatement *> statements;
  std::vector<CCSTStatement *> lbl_statements;
  std::vector<CCSTAssignExpr *> lcd_async_start_args;
  std::vector<CCSTAssignExpr *> dummy_args;
  std::vector<CCSTAssignExpr *> async_fntype_args;
  std::vector<CCSTAssignExpr *> fipc_set_0;
  std::vector<CCSTDecSpecifier *> spec_fipcm;

  std::vector<CCSTInitDeclarator *> decs_req;
  std::vector<CCSTInitDeclarator *> decs_resp;
  std::vector<CCSTInitDeclarator *> decs_ret;
  bool async_sync = false;

  decs_ret.push_back(
      new CCSTDeclarator(pointer(0), new CCSTDirectDecId("ret")));
  declarations.push_back(new CCSTDeclaration(int_type(), decs_ret));

  spec_fipcm.push_back(new CCSTStructUnionSpecifier(struct_t, "fipc_message"));

  decs_resp.push_back(new CCSTInitDeclarator(
      new CCSTDeclarator(pointer(1), new CCSTDirectDecId("_response"))));

  decs_req.push_back(new CCSTInitDeclarator(
      new CCSTDeclarator(pointer(1), new CCSTDirectDecId("_request"))));

  declarations.push_back(new CCSTDeclaration(spec_fipcm, decs_req));

  declarations.push_back(new CCSTDeclaration(spec_fipcm, decs_resp));

  if (c->getChannelType() == Channel::ChannelType::AsyncChannel) {
    if (r->function_pointer_defined()) {
      /// XXX: should we access this through the structure
      lcd_async_start_args.push_back(
          new CCSTPrimaryExprId("hidden_args->async_chnl"));
    } else {
      lcd_async_start_args.push_back(new CCSTPrimaryExprId(c->chName));
    }
    lcd_async_start_args.push_back(new CCSTUnaryExprCastExpr(
        reference(), new CCSTPrimaryExprId("_request")));

    if (user) {
      statements.push_back(
          new CCSTExprStatement(function_call("thc_init", dummy_args)));
    }

    statements.push_back(new CCSTExprStatement(new CCSTAssignExpr(
        new CCSTPrimaryExprId("ret"), equals(),
        new CCSTPostFixExprAssnExpr(
            new CCSTPrimaryExprId("async_msg_blocking_send_start"),
            lcd_async_start_args))));

    std::string *goto_fail_async = new std::string("fail_async");
    statements.push_back(if_cond_fail_goto(new CCSTPrimaryExprId("ret"),
                                           "failed to get a send slot",
                                           *goto_fail_async));

    lbl_statements.push_back(
        new CCSTPlainLabelStatement(*goto_fail_async, NULL));

    async_fntype_args.push_back(new CCSTPrimaryExprId("_request"));
    async_fntype_args.push_back(new CCSTPrimaryExprId(r->enum_name()));

    statements.push_back(new CCSTExprStatement(new CCSTPostFixExprAssnExpr(
        new CCSTPrimaryExprId("async_msg_set_fn_type"), async_fntype_args)));
  }

  /* marshal parameters */
  for (auto p : *r) {
    if (p->in()) {
      std::cout << " ASYNC marshal variable " << p->identifier()
                << " for function " << r->name() << std::endl;
      // TODO: Unify instances like this . Refer one more below
      CCSTCompoundStatement *updates = dynamic_cast<CCSTCompoundStatement *>(
          marshal_variable(p, "in", c->chType));

      std::vector<CCSTDeclaration *> __tmp_declarations =
          updates->getdeclarations();
      std::vector<CCSTStatement *> __tmp_statements = updates->getstatements();

      declarations.insert(declarations.end(), __tmp_declarations.begin(),
                          __tmp_declarations.end());
      statements.insert(statements.end(), __tmp_statements.begin(),
                        __tmp_statements.end());
    }
    /// if there is a void pointer, we need to perform an async send,
    /// followed by sync send, followed by async recv.
    /// set async_sync to true to facilitate this
    if (p->type()->num() == VOID_TYPE && !async_sync) {
      async_sync = true;
      std::vector<CCSTDecSpecifier *> spec_reqc;
      std::vector<CCSTInitDeclarator *> decs_reqc;

      spec_reqc.push_back(new CCSTSimpleTypeSpecifier(
          CCSTSimpleTypeSpecifier::UnsignedTypeSpec));
      spec_reqc.push_back(new CCSTSimpleTypeSpecifier(
          CCSTSimpleTypeSpecifier::IntegerTypeSpec));

      decs_reqc.push_back(new CCSTInitDeclarator(new CCSTDeclarator(
          pointer(0), new CCSTDirectDecId("request_cookie"))));

      declarations.push_back(new CCSTDeclaration(spec_reqc, decs_reqc));
    }
  }

  /// If ret is a container, its my_ref cptr needs to be marshalled as well
  if (r->return_variable()->container()) {
    CCSTCompoundStatement *updates = dynamic_cast<CCSTCompoundStatement *>(
        marshal_variable(r->return_variable(), "in", c->chType));

    std::vector<CCSTDeclaration *> __tmp_declarations =
        updates->getdeclarations();
    std::vector<CCSTStatement *> __tmp_statements = updates->getstatements();

    declarations.insert(declarations.end(), __tmp_declarations.begin(),
                        __tmp_declarations.end());
    statements.insert(statements.end(), __tmp_statements.begin(),
                      __tmp_statements.end());
  }

  /* if it is a function pointer need to marshal hidden args */
  if (r->function_pointer_defined()) {
    std::vector<Parameter *> hidden_args = r->hidden_args_;
    for (auto p : hidden_args) {
      if (p->in()) {
        std::cout << " ASYNC going to marshal hidden arg " << p->identifier()
                  << " for function " << r->name() << std::endl;
        CCSTCompoundStatement *updates = dynamic_cast<CCSTCompoundStatement *>(
            marshal_variable(p, "in", c->chType));

        std::vector<CCSTDeclaration *> __tmp_declarations =
            updates->getdeclarations();
        std::vector<CCSTStatement *> __tmp_statements =
            updates->getstatements();

        declarations.insert(declarations.end(), __tmp_declarations.begin(),
                            __tmp_declarations.end());
        statements.insert(statements.end(), __tmp_statements.begin(),
                          __tmp_statements.end());
      }
    }
  }
  std::vector<CCSTAssignExpr *> lcd_async_call_args;

  if (r->function_pointer_defined()) {
    lcd_async_call_args.push_back(
        new CCSTPrimaryExprId("hidden_args->async_chnl"));
  } else {
    lcd_async_call_args.push_back(new CCSTPrimaryExprId(c->chName));
  }
  lcd_async_call_args.push_back(new CCSTPrimaryExprId("_request"));

  std::string ipc_call;

  /// async send/call
  if (async_sync) {
    lcd_async_call_args.push_back(new CCSTUnaryExprCastExpr(
        reference(), new CCSTPrimaryExprId("request_cookie")));
    ipc_call.append("thc_ipc_send_request");
  } else {
    lcd_async_call_args.push_back(new CCSTUnaryExprCastExpr(
        reference(), new CCSTPrimaryExprId("_response")));
    ipc_call.append("thc_ipc_call");
  }

  if (user) {
    std::vector<CCSTStatement *> async_stmnts;
    std::vector<CCSTDeclaration *> decs;
    std::vector<CCSTAssignExpr *> lcd_async_macro_args;
    std::vector<CCSTDeclaration *> ipcall_decs;
    std::vector<CCSTStatement *> ipcall_stmts;

    ipcall_stmts.push_back(new CCSTExprStatement(
        new CCSTAssignExpr(new CCSTPrimaryExprId("ret"), equals(),
                           function_call(ipc_call, lcd_async_call_args))));

    async_stmnts.push_back(new CCSTMacro(
        "ASYNC", new CCSTCompoundStatement(ipcall_decs, ipcall_stmts), true));

    statements.push_back(new CCSTMacro(
        "DO_FINISH", new CCSTCompoundStatement(decs, async_stmnts), true));
  } else {
    statements.push_back(new CCSTExprStatement(
        new CCSTAssignExpr(new CCSTPrimaryExprId("ret"), equals(),
                           function_call(ipc_call, lcd_async_call_args))));
  }

  std::string *goto_ipc = new std::string("fail_ipc");
  statements.push_back(
      if_cond_fail_goto(new CCSTPrimaryExprId("ret"), ipc_call, *goto_ipc));

  lbl_statements.push_back(new CCSTPlainLabelStatement(*goto_ipc, NULL));

  for (auto p : *r) {
    if (p->type_->num() == VOID_TYPE) {
      std::cout << " SYNC send of void pointer " << p->identifier()
                << " for function " << r->name() << std::endl;
      statements.push_back(marshal_void_delayed(p));
    }
  }

  /// ipc_recv
  if (async_sync) {
    std::vector<CCSTAssignExpr *> lcd_async_recv_args;
    if (r->function_pointer_defined()) {
      lcd_async_recv_args.push_back(
          new CCSTPrimaryExprId("hidden_args->async_chnl"));
    } else {
      lcd_async_recv_args.push_back(new CCSTPrimaryExprId(c->chName));
    }

    lcd_async_recv_args.push_back(new CCSTPrimaryExprId("request_cookie"));
    lcd_async_recv_args.push_back(new CCSTUnaryExprCastExpr(
        reference(), new CCSTPrimaryExprId("_response")));

    statements.push_back(new CCSTExprStatement(new CCSTAssignExpr(
        new CCSTPrimaryExprId("ret"), equals(),
        function_call("thc_ipc_recv_response", lcd_async_recv_args))));
  }

  /* unmarshal appropriate parameters and return value */
  for (auto &p : *r) {
    if (p->type()->num() != VOID_TYPE) {
      if (p->out()) {
        std::vector<CCSTStatement *> tmp_stmts =
            unmarshal_variable_caller(p, c->chType);
        statements.insert(statements.end(), tmp_stmts.begin(), tmp_stmts.end());

        // unmarshal container things associated with this param
        tmp_stmts = unmarshal_container_refs_caller(p, c->chType);
        statements.insert(statements.end(), tmp_stmts.begin(), tmp_stmts.end());
      }
    }
  }

  /* if function pointer defined unmarshal hidden args*/
  if (r->function_pointer_defined()) {
    std::vector<Parameter *> hidden_args = r->hidden_args_;
    for (auto p : hidden_args) {
      if (p->out()) {
        std::vector<CCSTStatement *> tmp_stmts =
            unmarshal_variable_caller(p, c->chType);
        statements.insert(statements.end(), tmp_stmts.begin(), tmp_stmts.end());
      }
    }
  }

  // if anything is marked dealloc. dealloc
  for (auto p : *r) {
    std::vector<CCSTStatement *> tmp_statements =
        dealloc_containers_caller(p, cspace_to_use, r->current_scope());
    statements.insert(statements.end(), tmp_statements.begin(),
                      tmp_statements.end());
  }

  if (r->return_variable()->type()->num() != VOID_TYPE) {
    // declare return var.
    declarations.push_back(declare_variable(r->return_variable()));

    if (r->return_variable()->container()) {
      std::vector<CCSTStatement *> tmp_stmts = unmarshal_variable_caller(
          r->return_variable()->container(), c->chType);
      statements.insert(statements.end(), tmp_stmts.begin(), tmp_stmts.end());
    } else {
      // unmarshal return var
      std::vector<CCSTStatement *> tmp_stmts =
          unmarshal_return_variable_no_check(r->return_variable(), c->chType);
      statements.insert(statements.end(), tmp_stmts.begin(), tmp_stmts.end());
    }
  }
  std::vector<CCSTAssignExpr *> ipc_recv_end_args;
  std::vector<CCSTAssignExpr *> chnl_to_fipc_args;

  if (r->function_pointer_defined()) {
    chnl_to_fipc_args.push_back(
        new CCSTPrimaryExprId("hidden_args->async_chnl"));
  } else {
    chnl_to_fipc_args.push_back(new CCSTPrimaryExprId(c->chName));
  }

  ipc_recv_end_args.push_back(
      function_call("thc_channel_to_fipc", chnl_to_fipc_args));
  ipc_recv_end_args.push_back(new CCSTPrimaryExprId("_response"));
  statements.push_back(new CCSTExprStatement(
      function_call("fipc_recv_msg_end", ipc_recv_end_args)));
  return new CCSTCompoundStatement(declarations, statements, lbl_statements);
}

CCSTCompoundStatement *sync_call(Rpc *r, Module *m, std::string &cspace_to_use,
                                 Channel *c) {
  std::vector<CCSTDeclaration *> declarations;
  std::vector<CCSTStatement *> statements;
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
    for (auto p : r->hidden_args_) {
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
  statements.push_back(
      marshal(new CCSTEnumConst(*func_type), 0, c->chType, "request"));

  /* make remote call using appropriate channel */

  std::vector<CCSTInitDeclarator *> err;
  err.push_back(new CCSTDeclarator(0x0, new CCSTDirectDecId("err")));
  declarations.push_back(new CCSTDeclaration(int_type(), err));

  std::vector<CCSTAssignExpr *> lcd_sync_call_args;

  lcd_sync_call_args.push_back(new CCSTPrimaryExprId(
      m->channels().at(0)->identifier())); // first channel
  statements.push_back(new CCSTExprStatement(
      new CCSTAssignExpr(new CCSTPrimaryExprId("err"), equals(),
                         function_call("lcd_sync_call", lcd_sync_call_args))));

  statements.push_back(
      if_cond_fail(new CCSTPrimaryExprId("err"), "lcd_sync_call"));

  /* unmarshal appropriate parameters and return value */
  for (auto &p : *r) {
    if (p->type()->num() != VOID_TYPE) {
      if (p->out()) {
        std::vector<CCSTStatement *> tmp_stmts =
            unmarshal_variable_caller(p, c->chType);
        statements.insert(statements.end(), tmp_stmts.begin(), tmp_stmts.end());

        // unmarshal container things associated with this param
        tmp_stmts = unmarshal_container_refs_caller(p, c->chType);
        statements.insert(statements.end(), tmp_stmts.begin(), tmp_stmts.end());
      }
    }
  }

  /* if function pointer defined unmarshal hidden args*/
  if (r->function_pointer_defined()) {
    std::vector<Parameter *> hidden_args = r->hidden_args_;
    for (auto p : hidden_args) {
      if (p->out()) {
        std::vector<CCSTStatement *> tmp_stmts =
            unmarshal_variable_caller(p, c->chType);
        statements.insert(statements.end(), tmp_stmts.begin(), tmp_stmts.end());
      }
    }
  }

  // if anything is marked dealloc. dealloc
  for (auto p : *r) {
    std::vector<CCSTStatement *> tmp_statements =
        dealloc_containers_caller(p, cspace_to_use, r->current_scope());
    statements.insert(statements.end(), tmp_statements.begin(),
                      tmp_statements.end());
  }

  if (r->return_variable()->type()->num() != VOID_TYPE) {
    // declare return var.
    declarations.push_back(declare_variable(r->return_variable()));

    // unmarshal return var
    std::vector<CCSTStatement *> tmp_stmts =
        unmarshal_variable_no_check(r->return_variable(), c->chType);
    statements.insert(statements.end(), tmp_stmts.begin(), tmp_stmts.end());
  }

  return new CCSTCompoundStatement(declarations, statements);
}
