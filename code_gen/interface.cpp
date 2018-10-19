#include "ccst.h"
#include "code_gen.h"

/*
 * code related to interface initialization and tear-down.
 */


/*
 * 1. Always returns an int.
 * 2. name is glue_interfacename_init
 * 3. Params TODO. What params are always required, what is taken from IDL
 * for params, user specifies in IDL how many channels and their names
 * for each channel a cspace is declared and created with name chnlname_cspace.
 * Params are cptr_t for each channel, with specified name, 
 * There is an extra param not mentioned in idl "lcd_sync_channel_group". 
 */

//ah note - interface.cpp in code gen folder contributes to generating output for callee code (tested for fail1)

CCSTDeclaration* interface_init_function_declaration(Module *m)
{
  std::vector<CCSTDecSpecifier*> specifier;
  specifier.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::IntegerTypeSpec));
  CCSTDirectDecId *name = new CCSTDirectDecId(glue_name(init_name(m->identifier())));

  CCSTParamTypeList *param_list = new CCSTParamList();
  CCSTDirectDecParamTypeList *declaration = new CCSTDirectDecParamTypeList(name, param_list);

  std::vector<CCSTInitDeclarator*> func;
  func.push_back(new CCSTDeclarator(NULL, declaration));
  
  return new CCSTDeclaration(specifier, func);
}

/*
 *
 */
CCSTCompoundStatement* callee_interface_init_function_body(Module *m)
{
  std::vector<CCSTDeclaration*> body_declarations;
  std::vector<CCSTStatement*> body_statements;

  // declare ret
  std::vector<CCSTInitDeclarator*> decs;
  decs.push_back(new CCSTDeclarator(pointer(0)
				    , new CCSTDirectDecId("ret")));
  body_declarations.push_back(new CCSTDeclaration(int_type(), decs)); // TODO: add int ret;

  std::vector<GlobalVariable*> channels = m->channels();
  
  for (auto chan : channels) {
    std::vector<CCSTAssignExpr*> sync_channel_args;
    sync_channel_args.push_back(new CCSTPrimaryExprId(m->channel_group->identifier())); // &channel_group
    sync_channel_args.push_back(new CCSTPrimaryExprId(chan->identifier())); // channel 
    sync_channel_args.push_back(new CCSTInteger(1)); // 1 , per call? or?
    sync_channel_args.push_back(new CCSTPrimaryExprId("dispatch_todo")); // function poitner. dispatch.
    body_statements.push_back(new CCSTExprStatement( new CCSTAssignExpr(new CCSTPrimaryExprId("ret")
									, equals()
									, function_call("lcd_sync_channel_group_item_init"
											, sync_channel_args))));

    // do error checking
    body_statements.push_back(if_cond_fail_goto(new CCSTUnaryExprCastExpr(Not(), new CCSTPrimaryExprId("ret"))
						, "init channel item", "fail1"));
  }
  // TODO
  return new CCSTCompoundStatement(body_declarations, body_statements);
}

/* 
 * takes the vector of global variables which also provides the parameters to the function.
 * 1. What about the generalization of this?
 */
CCSTCompoundStatement* caller_interface_init_function_body(Module *m)
{
  // set each global variable to a parameter.
  std::vector<CCSTDeclaration*> body_declarations;
  std::vector<CCSTStatement*> body_statements;


  // declare ret
  std::vector<CCSTInitDeclarator*> decs;
  decs.push_back(new CCSTDeclarator(pointer(0)
				    , new CCSTDirectDecId("ret")));
  body_declarations.push_back(new CCSTDeclaration(int_type(), decs)); // TODO: add int ret;

  std::vector<GlobalVariable*> channels = m->channels();

  // init cap code..
  std::vector<CCSTAssignExpr*> cap_init_args_empty;
  body_statements.push_back(new CCSTExprStatement( new CCSTAssignExpr(new CCSTPrimaryExprId("ret")
								      , equals()
								      , function_call("glue_cap_init"
										      , cap_init_args_empty))));
  // do error checking
  //ah to resolve - this is in the caller_interface init function, so why is "cap_init" being output in nullnet_callee.c? Is this a bug?
  body_statements.push_back(if_cond_fail_goto(new CCSTPrimaryExprId("ret")
					      , "cap init", "fail1"));

  // initialize data stores.
  std::vector<GlobalVariable*> cspaces = m->cspaces_;
  for (auto gv : cspaces) {
    std::vector<CCSTAssignExpr*> cap_create_args;
    cap_create_args.push_back(new CCSTUnaryExprCastExpr(reference()
							, new CCSTPrimaryExprId( gv->identifier())));
    body_statements.push_back(new CCSTExprStatement( new CCSTAssignExpr(new CCSTPrimaryExprId("ret")
									, equals()
									, function_call("glue_cap_create"
											, cap_create_args))));
    
    // do error checking
    body_statements.push_back(if_cond_fail_goto(new CCSTPrimaryExprId("ret")
						, "cap create", "fail2"));
  }
  

  body_statements.push_back(new CCSTReturn(new CCSTInteger(0)));

  // failures
  std::vector<CCSTAssignExpr*> cap_exit_args_empty;
  body_statements.push_back(new CCSTPlainLabelStatement("fail2"
							, new CCSTExprStatement( function_call("glue_cap_exit"
											       , cap_exit_args_empty))));

  body_statements.push_back(new CCSTPlainLabelStatement("fail1"
						       , new CCSTReturn(new CCSTPrimaryExprId("ret"))));
  return new CCSTCompoundStatement(body_declarations, body_statements);
}

/*
 * 
 */
CCSTDeclaration* interface_exit_function_declaration(Module *m)
{
  /* returns void */
  std::vector<CCSTDecSpecifier*> specifier;
  specifier.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::VoidTypeSpec));

  CCSTDirectDecId *name = new CCSTDirectDecId(glue_name(exit_name(m->identifier())));
  CCSTParamList *param_list = new CCSTParamList(); // empty
  
  CCSTDirectDecParamTypeList *declaration = new CCSTDirectDecParamTypeList(name, param_list);

  std::vector<CCSTInitDeclarator*> func;
  func.push_back(new CCSTDeclarator(0x0, declaration));

  return new CCSTDeclaration(specifier, func);
}

/*
 * 
 * 1. Need to know what to tear-down.
 */
CCSTCompoundStatement* caller_interface_exit_function_body(Module *m)
{
  std::vector<CCSTDeclaration*> body_declarations;
  std::vector<CCSTStatement*> body_statements;

  // tear down all of the cspaces
  std::vector<GlobalVariable*> cspaces = m->cspaces_;
  for (auto gv : cspaces) {
    std::vector<CCSTAssignExpr*> cap_destroy_args;
    cap_destroy_args.push_back(new CCSTPrimaryExprId(gv->identifier()));
    body_statements.push_back(
      new CCSTExprStatement(
        function_call("glue_cap_destroy", cap_destroy_args)));
  }
  // vfs cap exit
  std::vector<CCSTAssignExpr*> cap_exit_args;
  body_statements.push_back(
    new CCSTExprStatement(function_call("glue_cap_exit", cap_exit_args)));

  return new CCSTCompoundStatement(body_declarations, body_statements);
}

CCSTCompoundStatement* callee_interface_exit_function_body(Module *m)
{
  std::vector<CCSTDeclaration*> body_declarations;
  std::vector<CCSTStatement*> body_statements;

  // tear down cspaces we declared
  std::vector<GlobalVariable*> cspaces = m->cspaces_;
  for (auto gv : cspaces) {
    std::vector<CCSTAssignExpr*> cap_destroy_args;
    cap_destroy_args.push_back(new CCSTPrimaryExprId(gv->identifier()));
    body_statements.push_back(new CCSTExprStatement( function_call(cap_destroy_name(m->identifier())
								   , cap_destroy_args)));
  }
  
  // remove channels from group item thign
  // callee code has a disptach loop, caller does not.
  // remove the one group from the group_item thing
  std::vector<CCSTAssignExpr*> lcd_sync_channel_group_remove_args;
  lcd_sync_channel_group_remove_args.push_back(new CCSTPrimaryExprId(m->channel_group->identifier()));
  lcd_sync_channel_group_remove_args.push_back(new CCSTUnaryExprCastExpr(reference()
									 , new CCSTPrimaryExprId("group_item")));
  body_statements.push_back(new CCSTExprStatement( function_call("lcd_sync_channel_group_remove_args"
								 , lcd_sync_channel_group_remove_args)));


  // tear down cap code
  std::vector<CCSTAssignExpr*> cap_exit_args;
  body_statements.push_back(new CCSTExprStatement( function_call(cap_exit_name(m->identifier())
								 , cap_exit_args)));
  
  return new CCSTCompoundStatement(body_declarations, body_statements);
}
