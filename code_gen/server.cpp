#include "ccst.h"
#include "code_gen.h"
// for std::map
#include <algorithm>

CCSTDeclaration* dispatch_sync_function_declaration()
{
  std::vector<CCSTDecSpecifier*> specifier;
  specifier.push_back(
    new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::VoidTypeSpec));

  std::vector<CCSTInitDeclarator*> decs;

  std::vector<CCSTParamDeclaration*> empty;
  decs.push_back(
    new CCSTDeclarator(0x0,
      new CCSTDirectDecParamTypeList(
        new CCSTDirectDecId("dispatch_sync_loop"),
        new CCSTParamList(empty))));
  return new CCSTDeclaration(specifier, decs);
}

CCSTDeclaration* dispatch_async_function_declaration(Module *mod)
{
  std::vector<CCSTDecSpecifier*> specifier;
  specifier.push_back(
    new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::IntegerTypeSpec));

  std::vector<CCSTInitDeclarator*> decs;
  std::vector<CCSTParamDeclaration*> empty;
  int err;
  std::vector<CCSTParamDeclaration*> p_decs;

  CCSTDirectDecId* id = new CCSTDirectDecId("dispatch_async_loop");

  p_decs.push_back(
    new CCSTParamDeclaration(
      type2(mod->module_scope()->lookup("thc_channel", &err)),
      new CCSTDeclarator(new CCSTPointer(), new CCSTDirectDecId("channel"))));
  p_decs.push_back(
    new CCSTParamDeclaration(
      type2(mod->module_scope()->lookup("fipc_message", &err)),
      new CCSTDeclarator(new CCSTPointer(), new CCSTDirectDecId("request"))));
  p_decs.push_back(
    new CCSTParamDeclaration(
      type2(mod->module_scope()->lookup("cspace", &err)),
      new CCSTDeclarator(new CCSTPointer(), new CCSTDirectDecId("cspace"))));
  p_decs.push_back(
    new CCSTParamDeclaration(type2(mod->module_scope()->lookup("cptr", &err)),
      new CCSTDeclarator(NULL, new CCSTDirectDecId("sync_ep"))));

  CCSTParamList *param_list = new CCSTParamList(p_decs);

  CCSTDirectDecParamTypeList *params = new CCSTDirectDecParamTypeList(id,
    param_list);

  CCSTDeclarator* declarator = new CCSTDeclarator(NULL, params);
  std::vector<CCSTInitDeclarator*> init_declarator;
  init_declarator.push_back(declarator);
  CCSTDeclaration *func_declaration = new CCSTDeclaration(specifier,
    init_declarator);

  return func_declaration;
}

CCSTCompoundStatement* dispatch_sync_loop_body(Module *mod, const std::string &type)
{
  std::vector<CCSTDeclaration*> decs_in_body;
  std::vector<CCSTDecSpecifier*> spec;
  std::vector<CCSTInitDeclarator*> dec;
  spec.push_back(
    new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::IntegerTypeSpec));
  dec.push_back(new CCSTDeclarator(NULL, new CCSTDirectDecId("fn_type")));
  decs_in_body.push_back(new CCSTDeclaration(spec, dec));

  std::vector<CCSTAssignExpr*> empty_args;
  CCSTAssignExpr* init_ret = new CCSTAssignExpr(
    new CCSTPrimaryExprId("fn_type"), new CCSTAssignOp(equal_t),
    new CCSTPostFixExprAssnExpr(new CCSTPrimaryExprId("lcd_r0"), empty_args));

  /// switch statement begins
  std::vector<CCSTStatement*> cases;
  for (auto rpc : *mod) {
    std::vector<CCSTDeclaration*> case_dec_empty;

    std::vector<CCSTStatement*> case_body_stmts;
    std::vector<CCSTAssignExpr*> msg_args;
    std::string *lcd_msg = new std::string("Calling function ");
    lcd_msg->append(rpc->name());
    msg_args.push_back(new CCSTString(*lcd_msg));
    case_body_stmts.push_back(
      new CCSTExprStatement(
        new CCSTPostFixExprAssnExpr(new CCSTPrimaryExprId("LIBLCD_MSG"),
          msg_args)));

    /// make call to callee.
    std::vector<CCSTAssignExpr*> args_empty;
    case_body_stmts.push_back(
      new CCSTReturn(
        new CCSTPostFixExprAssnExpr(
          new CCSTPrimaryExprId(rpc->callee_name()), args_empty)));

    case_body_stmts.push_back(new CCSTBreak());

    CCSTCompoundStatement* case_body = new CCSTCompoundStatement(
      case_dec_empty, case_body_stmts);
    CCSTCaseStatement* tmp_case = new CCSTCaseStatement(
      new CCSTPrimaryExprId(rpc->enum_name()), case_body);
    cases.push_back(tmp_case);
  }

  /// adding a default case
  std::vector<CCSTDeclaration*> default_dec_empty;
  std::vector<CCSTStatement*> default_stmts;
  std::vector<CCSTAssignExpr*> lcd_msg_args;
  lcd_msg_args.push_back(new CCSTString("unexpected function type"));
  default_stmts.push_back(
    new CCSTExprStatement(
      new CCSTPostFixExprAssnExpr(new CCSTPrimaryExprId("LIBLCD_MSG"),
        lcd_msg_args)));
  default_stmts.push_back(new CCSTReturn(new CCSTPrimaryExprId("-EINVAL")));

  std::vector<CCSTDeclaration*> switch_dec_empty;
  CCSTCompoundStatement* switch_body = new CCSTCompoundStatement(
    switch_dec_empty, cases);
  CCSTSwitchStatement* dispatch = new CCSTSwitchStatement(
    new CCSTPrimaryExprId("fn_type"), switch_body);
  /// switch statement end

  /// put body together
  std::vector<CCSTStatement*> body_statements;
  body_statements.push_back(new CCSTExprStatement(init_ret));
  body_statements.push_back(dispatch);
  body_statements.push_back(new CCSTReturn(new CCSTInteger(0)));
  /// body complete
  return new CCSTCompoundStatement(decs_in_body, body_statements);
}

CCSTCompoundStatement* dispatch_async_loop_body(Module *mod, const std::string &type)
{
  std::vector<CCSTDeclaration*> decs_in_body;
  std::vector<CCSTDecSpecifier*> spec;
  std::vector<CCSTInitDeclarator*> dec;
  spec.push_back(
    new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::IntegerTypeSpec));
  dec.push_back(new CCSTDeclarator(NULL, new CCSTDirectDecId("fn_type")));
  decs_in_body.push_back(new CCSTDeclaration(spec, dec));

  std::vector<CCSTAssignExpr*> async_args;
  async_args.push_back(new CCSTPrimaryExprId("message"));
  CCSTAssignExpr* init_ret = new CCSTAssignExpr(
    new CCSTPrimaryExprId("fn_type"), new CCSTAssignOp(equal_t),
    new CCSTPostFixExprAssnExpr(
      new CCSTPrimaryExprId("async_msg_get_fn_type"), async_args));

  /// switch statement begin
  std::vector<CCSTStatement*> cases;
  for (auto rpc : *mod) {

    /// Skip rpcs that are not function pointers as they need to be in
    /// the dispatch loop of the server
    /// Skip rpcs that are function pointers during server code gen
    if ((type == "client" && !rpc->function_pointer_defined())
      || (type == "server" && rpc->function_pointer_defined())) {
      continue;
    }

    std::vector<CCSTDeclaration*> case_dec_empty;
    std::vector<CCSTStatement*> case_body_stmts;
    std::vector<CCSTAssignExpr*> msg_args;
    std::string *lcd_msg = new std::string("Calling function ");
    lcd_msg->append(rpc->name());
    msg_args.push_back(new CCSTString(*lcd_msg));
    case_body_stmts.push_back(
      new CCSTExprStatement(
        new CCSTPostFixExprAssnExpr(new CCSTPrimaryExprId("LIBLCD_MSG"),
          msg_args)));

    /// make call to callee/caller
    std::vector<CCSTAssignExpr*> callee_args;
    callee_args.push_back(new CCSTPrimaryExprId("message"));
    callee_args.push_back(new CCSTPrimaryExprId("channel"));
    callee_args.push_back(new CCSTPrimaryExprId("cspace"));
    callee_args.push_back(new CCSTPrimaryExprId("sync_ep"));
    case_body_stmts.push_back(
      new CCSTReturn(
        new CCSTPostFixExprAssnExpr(
          new CCSTPrimaryExprId(rpc->callee_name()), callee_args)));

    case_body_stmts.push_back(new CCSTBreak());
    CCSTCompoundStatement* case_body = new CCSTCompoundStatement(
      case_dec_empty, case_body_stmts);
    CCSTCaseStatement* tmp_case = new CCSTCaseStatement(
      new CCSTPrimaryExprId(rpc->enum_name()), case_body);
    cases.push_back(tmp_case);
  }

  /// adding a default case
  std::vector<CCSTDeclaration*> default_dec_empty;
  std::vector<CCSTStatement*> default_stmts;
  std::vector<CCSTAssignExpr*> lcd_msg_args;
  lcd_msg_args.push_back(new CCSTString("Error unknown function"));
  default_stmts.push_back(
    new CCSTExprStatement(
      new CCSTPostFixExprAssnExpr(new CCSTPrimaryExprId("LIBLCD_MSG"),
        lcd_msg_args)));
  default_stmts.push_back(new CCSTReturn(new CCSTPrimaryExprId("-EINVAL")));

  std::vector<CCSTDeclaration*> switch_dec_empty;
  CCSTCompoundStatement* switch_body = new CCSTCompoundStatement(
    switch_dec_empty, cases);
  CCSTSwitchStatement* dispatch = new CCSTSwitchStatement(
    new CCSTPrimaryExprId("fn_type"), switch_body);
  /// switch statement end

  /// put body together
  std::vector<CCSTStatement*> body_statements;
  body_statements.push_back(new CCSTExprStatement(init_ret));
  body_statements.push_back(dispatch);
  body_statements.push_back(new CCSTReturn(new CCSTInteger(0)));
  /// body complete
  return new CCSTCompoundStatement(decs_in_body, body_statements);
}

/* body for a callee function
 * does unmarshaling, real function call, etc
 */
CCSTCompoundStatement* callee_body(Rpc *r, Module *m)
{
  std::vector<CCSTDeclaration*> declarations;
  std::vector<CCSTStatement*> statements;
  // allocate necessary container things
  
  /* code that loops through parameters and allocates/initializes whatever necessary before marshalling*/

  // loop through params, declare a tmp and pull out marshal value
  
  // declare containers and variables
  for (auto p : *r) {
    std::vector<CCSTDeclaration*> tmp_decs = declare_variables_callee(p);
    declarations.insert(declarations.end(), tmp_decs.begin(), tmp_decs.end());
  }

  if(r->function_pointer_defined()) {
    std::vector<Parameter*> hidden_args = r->hidden_args_;
    for(std::vector<Parameter*>::iterator it = hidden_args.begin(); it != hidden_args.end(); it ++) {
//      Parameter *p = *it;
      // declare hiiden args.
    }
  }

  Channel *c = r->getcurrentscope()->outer_scope_->getactiveChannel();
  if (c) {
    std::cout << "### " << __func__ << " Rpc " << r->name() << " ac:  " << c->chName << std::endl;
  } else {
    // Check if there exists a global module level channel
    c = m->module_scope()->activeChannel;
  }
  Channel::ChannelType type;
  if (c) {
    type = c->chType;
  }

  if (type == Channel::AsyncChannel) {
    std::vector<CCSTDecSpecifier*> spec_fipcm;
    std::vector<CCSTInitDeclarator*> decs_resp;
    std::vector<CCSTDecSpecifier*> spec_uint;
    std::vector<CCSTInitDeclarator*> decs_req_cookie;
    std::vector<CCSTAssignExpr*> lcd_req_cook_args;

    decs_resp.push_back(
      new CCSTInitDeclarator(
        new CCSTDeclarator(pointer(1), new CCSTDirectDecId("response"))));

    decs_req_cookie.push_back(
      new CCSTInitDeclarator(
        new CCSTDeclarator(pointer(0), new CCSTDirectDecId("request_cookie"))));

    spec_fipcm.push_back(
      new CCSTStructUnionSpecifier(struct_t, "fipc_message"));

    spec_uint.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::UnsignedTypeSpec));
    spec_uint.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::IntegerTypeSpec));

    declarations.push_back(
      new CCSTDeclaration(spec_fipcm, decs_resp));
    declarations.push_back(
          new CCSTDeclaration(spec_uint, decs_req_cookie));

    lcd_req_cook_args.push_back(new CCSTPrimaryExprId("request"));

    statements.push_back(
      new CCSTExprStatement(
        new CCSTAssignExpr(new CCSTPrimaryExprId("request_cookie"), equals(),
          function_call("thc_get_request_cookie", lcd_req_cook_args))));
    std::vector<CCSTAssignExpr*> ipc_recv_end_args;
    std::vector<CCSTAssignExpr*> chnl_to_fipc_args;
    chnl_to_fipc_args.push_back(new CCSTPrimaryExprId(c->chName));
    ipc_recv_end_args.push_back(function_call("thc_channel_to_fipc", chnl_to_fipc_args));
    ipc_recv_end_args.push_back(new CCSTPrimaryExprId("request"));
    statements.push_back(new CCSTExprStatement(function_call("fipc_recv_msg_end", ipc_recv_end_args)));
  }
  // TODO: unmarshal channel refs;

  // allocate/initiliaze and link these
  for (auto p : *r) {
    std::cout << "Parameter is " << p->identifier() << std::endl;
    statements.push_back(
      allocate_and_link_containers_callee(p,
        m->cspaces_.at(0)->identifier(), type));
  }

  // allocate things which are not containers
  for (auto p : *r) {
    statements.push_back(allocate_non_container_variables(p));
  }

  /* As it stands now, only need to allocate hidden args structures if the 
   * parameter is alloc callee and contains function pointers
   */

  // declare hidden args structures;
  for (auto p : *r) {
    if( p->type()->num() == PROJECTION_TYPE || p->type()->num() == PROJECTION_CONSTRUCTOR_TYPE) {
      ProjectionType *pt = dynamic_cast<ProjectionType*>(p->type());
      Assert(pt != 0x0, "Error: dynamic cast to projection type failed\n");
      
      std::vector<CCSTDeclaration*> tmp_decs = declare_hidden_args_structures(pt, r->current_scope());
      declarations.insert(declarations.end(), tmp_decs.begin(), tmp_decs.end());
      
      // allocate hidden args structures
      Variable* cspace;
      if(p->type()->num() == PROJECTION_CONSTRUCTOR_TYPE) {
	ProjectionConstructorType *pct = dynamic_cast<ProjectionConstructorType*>(p->type());
	Assert(pct != 0x0, "Error: dynamic cast to projection constructor failed\n");

	cspace = pct->channel_params_.at(0).second;
      } else if (pt->channels_.size() > 0) {
        cspace = pt->channels_.at(0);
      } else {
	cspace = m->cspaces_.at(0);
      }
      
      if(p->alloc_callee()) {
	statements.push_back(alloc_init_hidden_args_struct(pt, p, r->current_scope(), m->cspaces_.at(0)));
      }
    }
  }

  // unmarshal rest of parameters. rest means not a container reference.
  for (auto p : *r) {
    if(p->in()) {
      std::vector<CCSTStatement*> tmp_stmts = unmarshal_variable_callee(p, type);
      statements.insert(statements.end(), tmp_stmts.begin(), tmp_stmts.end());
    }
  }

  
  /* build up real call params */
  std::vector<CCSTAssignExpr*> real_call_params;
  for (auto p : *r) {
    if(p->container()) {
      ProjectionType *p_container_type = dynamic_cast<ProjectionType*>(p->container()->type());
      Assert(p_container_type != 0x0, "Error: dynamic cast to projection type failed\n");
      ProjectionField *p_container_real_field = p_container_type->get_field(p->type()->name());
      Assert(p_container_real_field != 0x0, "Error: could not find field in structure\n");
      /// XXX: Is this enough to handle all the cases?
      if (p->pointer_count() == 1) {
        real_call_params.push_back(new CCSTPrimaryExpr(new CCSTUnaryExprCastExpr(reference(), access(p_container_real_field))));
      } else {
        real_call_params.push_back(access(p_container_real_field));
      }
    } else {
      real_call_params.push_back(access(p));
    }
   
  }

  // declare return variable
  if(r->return_variable()->type()->num() != VOID_TYPE) {
    std::vector<CCSTDeclaration*> tmp_decs = declare_variables_callee(r->return_variable());
    declarations.insert(declarations.end(), tmp_decs.begin(), tmp_decs.end());
  }

  // allocate return variable // return value cannot have a container?
  statements.push_back(allocate_and_link_containers_callee(r->return_variable()
							   , m->cspaces_.at(0)->identifier(), type));
  statements.push_back(allocate_non_container_variables(r->return_variable()));
  
  // real call
  if(r->return_variable()->type()->num() == VOID_TYPE) {
    statements.push_back(new CCSTExprStatement(function_call(r->name()
							     , real_call_params)));
  } else {
    statements.push_back(new CCSTExprStatement(new CCSTAssignExpr(access(r->return_variable())
								  , equals()
								  , function_call(r->name()
										  , real_call_params))));
  }

  /* dealloc containers and contents of containers if necessary */
  for (auto& p : *r) {
    std::vector<CCSTStatement*> tmp_stmts = dealloc_containers_callee(p, m->cspaces_.at(0)->identifier(), r->current_scope());
    statements.insert(statements.end(), tmp_stmts.begin(), tmp_stmts.end());
  }

  if (type == Channel::AsyncChannel){
     std::vector<CCSTDeclaration*> if_body_declarations;
     std::vector<CCSTStatement*> if_body_statements;

     std::vector<CCSTAssignExpr*> liblcd_err_args;
     liblcd_err_args.push_back(new CCSTString("error getting response msg"));
     if_body_statements.push_back(new CCSTExprStatement( function_call("LIBLCD_ERR", liblcd_err_args)));

     std::vector<CCSTAssignExpr*> lcd_exit_args;
     lcd_exit_args.push_back(new CCSTInteger(-1));
     // TODO: Change this to return -EIO;
     if_body_statements.push_back(new CCSTReturn(new CCSTPrimaryExprId("-EIO")));
       //new CCSTExprStatement(function_call("lcd_exit", lcd_exit_args)));

     CCSTCompoundStatement *if_body = new CCSTCompoundStatement(if_body_declarations, if_body_statements);

     std::vector<CCSTAssignExpr*> lcd_async_start_args;
     lcd_async_start_args.push_back(new CCSTPrimaryExprId(c->chName));
     lcd_async_start_args.push_back(
       new CCSTUnaryExprCastExpr(reference(),
         new CCSTPrimaryExprId("response")));

     CCSTStatement *iif = new CCSTIfStatement(
       new CCSTPostFixExprAssnExpr(
         new CCSTPrimaryExprId("async_msg_blocking_send_start"),
         lcd_async_start_args), dynamic_cast<CCSTStatement*>(if_body));

     statements.push_back(iif);
   }

  /* marshal return params and val */
  for (auto& p : *r) {
    if(p->out()) {
      std::vector<CCSTStatement*> tmp_stmts = marshal_variable_callee(p, type);
      statements.insert(statements.end(), tmp_stmts.begin(), tmp_stmts.end());
    }
  }

  // marshal return val;
  if(r->return_variable()->type()->num() != VOID_TYPE) {
    std::vector<CCSTStatement*> tmp_stmts = marshal_variable_no_check(r->return_variable(), type);
    statements.insert(statements.end(), tmp_stmts.begin(), tmp_stmts.end());
  }

  if (type == Channel::SyncChannel) {
    /* make IPC return call */
    // err = lcd_sync_reply();
    // if (err) { ...}
    std::vector<CCSTAssignExpr*> lcd_sync_reply_args_empty;
    statements.push_back(new CCSTExprStatement( new CCSTAssignExpr(new CCSTPrimaryExprId("err")
                   , equals()
                   , function_call("lcd_sync_reply"
                       , lcd_sync_reply_args_empty))));
    statements.push_back(if_cond_fail(new CCSTPrimaryExprId("err")
              , "lcd_sync_reply"));
  } else {
    std::vector<CCSTAssignExpr*> ipc_reply_args;
    ipc_reply_args.push_back(new CCSTPrimaryExprId(c->chName));
    ipc_reply_args.push_back(new CCSTPrimaryExprId("request_cookie"));
    ipc_reply_args.push_back(new CCSTPrimaryExprId("response"));
    statements.push_back(new CCSTExprStatement(function_call("thc_ipc_reply", ipc_reply_args)));
  }

  statements.push_back(new CCSTReturn(new CCSTPrimaryExprId("ret")));
  return new CCSTCompoundStatement(declarations, statements);
}


CCSTFile* generate_server_header(Module *module)
{
  std::vector<CCSTExDeclaration*> definitions;

  definitions.push_back(new CCSTPreprocessor("../glue_helper.h", true));

  /// check if there are rpcs
  if(!module->rpc_definitions().empty())
    {
      /// TODO: This enum list has IDs for both caller and callee. This
      /// should go to a common header (glue_helper.h)
      definitions.push_back(construct_enum(module));
      /// function callee function declarations
      for (auto r : *module) {
        /// Print declaration only for callee functions.
        /// Function pointer callee functions are declared in caller's header
        if (!r->function_pointer_defined()) {
          definitions.push_back(callee_declaration((Rpc*)r));
        }
      }
    }
  /// generate function declaration for sync, async loop
  definitions.push_back(dispatch_sync_function_declaration());
  definitions.push_back(dispatch_async_function_declaration(module));
  return new CCSTFile(definitions);
}

CCSTDeclaration* callee_declaration(Rpc* r)
{
  std::vector<CCSTDecSpecifier*> specifier; // = new std::vector<CCSTDecSpecifier*>();
  specifier.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::IntegerTypeSpec));
  CCSTDirectDecId* id = new CCSTDirectDecId(new_name(r->name(), "_callee"));
  
  std::vector<CCSTDecSpecifier*> s;
  s.push_back(new  CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::VoidTypeSpec));

  int err;
  std::vector<CCSTParamDeclaration*> p_decs;
  p_decs.push_back(
      new CCSTParamDeclaration(
          type2(r->current_scope()->lookup("fipc_message", &err)),
          new CCSTDeclarator(new CCSTPointer(),
              new CCSTDirectDecId("request"))));
  p_decs.push_back(
      new CCSTParamDeclaration(
          type2(r->current_scope()->lookup("thc_channel", &err)),
          new CCSTDeclarator(new CCSTPointer(),
              new CCSTDirectDecId("channel"))));
  p_decs.push_back(
      new CCSTParamDeclaration(
          type2(r->current_scope()->lookup("cspace", &err)),
          new CCSTDeclarator(new CCSTPointer(),
              new CCSTDirectDecId("cspace"))));
  p_decs.push_back(
      new CCSTParamDeclaration(
          type2(r->current_scope()->lookup("cptr", &err)),
          new CCSTDeclarator(NULL,
              new CCSTDirectDecId("sync_ep"))));

  CCSTParamList *param_list = new CCSTParamList(p_decs);

  CCSTDirectDecParamTypeList *params = new CCSTDirectDecParamTypeList(id, param_list); 
    
  CCSTDeclarator* declarator = new CCSTDeclarator(NULL, params);
  std::vector<CCSTInitDeclarator*> init_declarator; // = new std::vector<CCSTInitDeclarator*>();
  init_declarator.push_back(declarator);
  CCSTDeclaration *func_declaration = new CCSTDeclaration(specifier, init_declarator);
  
  return func_declaration;
}



/* ServerCCSTSourceVisitor */


/*
 * generates the source file for the provided module/interface
 */
CCSTFile* generate_server_source(Module *m, std::vector<Include*> includes)
{
  std::vector<CCSTExDeclaration*> definitions;

  for(std::vector<Include*>::iterator it = includes.begin(); it != includes.end(); it ++) {
    Include *inc = *it;
    definitions.push_back(new CCSTPreprocessor(inc->get_path(), inc->is_relative()));
  }

  // need to print containers. but scopes container possibly duplicate projections definitions.

  // this feels mildy "hacky"
  // for each projection look up its container int the environment and print its declaration
  // if no container int the environment, don't print
  std::map<std::string, Type*> module_types = m->module_scope()->all_type_definitions();
  for(std::map<std::string, Type*>::iterator it = module_types.begin(); it != module_types.end(); it ++) {
    Type *t = (Type*) it->second;

    if(t->num() == PROJECTION_TYPE || t->num() == PROJECTION_CONSTRUCTOR_TYPE) {
      ProjectionType *pt = dynamic_cast<ProjectionType*>(t);
      Assert(pt != 0x0, "Error: dynamic cast to projection type failed!\n");
      
      if(module_types.find(container_name(pt->name())) != module_types.end()) { // found the container
	ProjectionType *pt_container = dynamic_cast<ProjectionType*>(module_types[container_name(pt->name())]);
	Assert(pt_container != 0x0, "Error: dynamic cast to projection type failed\n");

	std::vector<CCSTDecSpecifier*> specifier;
	specifier.push_back(struct_declaration(pt_container));
	std::vector<CCSTInitDeclarator*> empty;
	definitions.push_back(new CCSTDeclaration(specifier, empty));
      }
    } else if (t->num() == FUNCTION_TYPE) {
      Function *f = dynamic_cast<Function*>(t);
      Assert(f != 0x0, "Error: dynamic cast to projection type failed!\n");

      if(module_types.find(container_name(f->name())) != module_types.end()) { // found the container
  ProjectionType *pt_container = dynamic_cast<ProjectionType*>(module_types[container_name(f->name())]);
  Assert(pt_container != 0x0, "Error: dynamic cast to projection type failed\n");
  std::vector<CCSTDecSpecifier*> specifier;
  specifier.push_back(struct_declaration(pt_container));
  std::vector<CCSTInitDeclarator*> empty;
  definitions.push_back(new CCSTDeclaration(specifier, empty));

      }
    }
  }
  
  // print trampoline structs
  for (auto rpc : *m) {
    if (rpc->function_pointer_defined()) {
      int err;
      Type *t = rpc->current_scope()->lookup("trampoline_hidden_args", &err);
      Assert(t != 0x0, "Error: failure looking up type\n");
      ProjectionType *pt = dynamic_cast<ProjectionType*>(t);
      Assert(t != 0x0, "Error: dynamic cast to projection type failed!\n");

      std::vector<CCSTDecSpecifier*> specifier;
      specifier.push_back(struct_declaration(pt));
      std::vector<CCSTInitDeclarator*> empty;
      definitions.push_back(new CCSTDeclaration(specifier, empty));

      /// XXX: struct trampoline_hidden_args need to be defined only
      /// once despite the number of function pointers.
      /// So, break after once!
      break;
    }
  }
  

  // globals.
  std::vector<GlobalVariable*> globals = m->channels();
  for(std::vector<GlobalVariable*>::iterator it = globals.begin(); it != globals.end(); it ++) {
    GlobalVariable *gv = *it;
    definitions.push_back(declare_static_variable(gv));
  }

  for (auto rpc : *m) {
    if (rpc->function_pointer_defined()) {
      std::cout << "doing function pointer def\n";
      definitions.push_back(
        function_definition(function_pointer_function_declaration(rpc),
          caller_body(rpc, m)));

      definitions.push_back(trampoline_data_macro(rpc));

      definitions.push_back(
        function_definition(trampoline_function_declaration(rpc),
          trampoline_function_body(rpc)));
    } else {
      std::cout << "doing callee_declaration\n";
      definitions.push_back(
        function_definition(callee_declaration(rpc), callee_body(rpc, m)));
    }
  }

  CCSTFile *c_file = new CCSTFile(definitions);
  std::cout << "in server source gen" << std::endl;
  return c_file;
}

CCSTFile *generate_callee_lds(Module *mod)
{
  std::vector<Rpc*> rpcs = mod->rpc_definitions();
  std::vector<CCSTExDeclaration*> statements;
  for (std::vector<Rpc*>::iterator it = rpcs.begin(); it != rpcs.end(); it++) {
    Rpc *r = *it;
    if (r->function_pointer_defined()) {
      std::vector<CCSTAssignExpr*> data_args;
      data_args.push_back(new CCSTPrimaryExprId(trampoline_func_name(r->name())));
      statements.push_back(
          new CCSTMacro("LCD_TRAMPOLINE_LINKER_SECTION", data_args, false));
    }
  }
  return new CCSTFile(statements);
}

CCSTFile* generate_glue_source(Module *m)
{
  std::vector<CCSTExDeclaration*> statements;
  std::map<std::string, std::pair<std::string,Type*>> types_map;
  std::vector<CCSTEnumerator*>* list = new std::vector<CCSTEnumerator*>();

  std::vector<LexicalScope*> inner_scopes = m->module_scope()->inner_scopes();

  for (std::vector<LexicalScope*>::iterator it = inner_scopes.begin(); it != inner_scopes.end(); ++it) {
    auto type_defs = (*it)->type_definitions();
    for (std::pair<std::string, Type*> type : type_defs) {
      Type *ty = type.second;
      if (ty->name().find("_container") != std::string::npos) {
        std::string enum_name = ty->name();
        std_string_toupper(enum_name);
        types_map[ty->name()] = std::pair<std::string,Type*>(enum_name, ty);
      }
    }
  }

  for (auto t : types_map) {
    list->push_back(new CCSTEnumerator(t.second.first));
  }

  CCSTEnumeratorList *enum_list = new CCSTEnumeratorList(list);
  CCSTEnumSpecifier *e = new CCSTEnumSpecifier("glue_type", enum_list);
  std::vector<CCSTDecSpecifier*> tmp;
  tmp.push_back(e);
  std::vector<CCSTInitDeclarator*> empty;
  CCSTDeclaration *declaration = new CCSTDeclaration(tmp, empty);
  statements.push_back(declaration);

  return new CCSTFile(statements);
}

inline CCSTExDeclaration *prehook_include() {
  return new CCSTPreprocessor("lcd_config/pre_hook.h", false);
}

inline CCSTExDeclaration *posthook_include() {
  return new CCSTPreprocessor("lcd_config/post_hook.h", false);
}

inline CCSTExDeclaration *liblcd_include() {
  return new CCSTPreprocessor("liblcd/liblcd.h", false);
}

inline CCSTExDeclaration *calleeh_include(const std::string &id) {
  return new CCSTPreprocessor("../" + id + "_callee.h", true);
}

inline CCSTExDeclaration *callerh_include(const std::string &id) {
  return new CCSTPreprocessor("../" + id + "_caller.h", true);
}

CCSTFile* generate_dispatch(Module *m, const std::string &type)
{
  std::vector<CCSTExDeclaration*> statements;

  statements.push_back(prehook_include());
  statements.push_back(liblcd_include());

  if (type == "client") {
    statements.push_back(callerh_include(m->identifier()));
  } else if (type == "server") {
    statements.push_back(calleeh_include(m->identifier()));
  }

  statements.push_back(posthook_include());

  /// XXX: As of now, all the functions use async mechanism by default.
  /// In case if some functions need sync communication, it has to be
  /// marked sync explicitly or some other mechanism has to be found out
  /// to solve this. Until that, don't generate any code for sync
  /// dispatch loop
  if (0) {
    statements.push_back(
      function_definition(dispatch_sync_function_declaration(),
        dispatch_sync_loop_body(m, type)));
  }

  statements.push_back(
    function_definition(dispatch_async_function_declaration(m),
      dispatch_async_loop_body(m, type)));

  return new CCSTFile(statements);
}
