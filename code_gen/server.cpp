#include "ccst.h"

CCSTDeclaration* create_dispatch_function_declaration()
{
  std::vector<CCSTDecSpecifier*>specifier;
  specifier.push_back(new CCSTSimpleTypeSpecifier(void_t));
  
  std::vector<CCSTInitDeclarator*> decs;
  
  std::vector<CCSTParamDeclaration*> empty;
  decs.push_back(new CCSTDeclarator(0x0
				    , new CCSTDirectDecParamTypeList( new CCSTDirectDecId("dispatch_loop")
								      , new CCSTParamList(empty))));
  return new CCSTDeclaration(specifier, decs);
}

CCSTCompoundStatement* create_dispatch_loop_body(std::vector<Rpc*> rps)
{  
  
  std::vector<CCSTDeclaration*> decs_in_body;
  // int ret; 
  std::vector<CCSTDecSpecifier*> s;// = new std::vector<CCSTDecSpecifier*>();
  std::vector<CCSTInitDeclarator*> d;// = new std::vector<CCSTInitDeclarator*>();
  s.push_back(new CCSTSimpleTypeSpecifier(int_t));
  d.push_back(new CCSTDeclarator(0x0, new CCSTDirectDecId("ret")));
  decs_in_body.push_back(new CCSTDeclaration(s,d));
  // Declare a variable of type int with name ret
  
  
  /* body statement, switch*/


  /*  lcd_recv(manufacturer_interface_cap) */
  /* check ret value, begin */
  std::vector<CCSTAssignExpr*> args;
  args.push_back( new CCSTPrimaryExprId("manufacturer_interface_cap"));
  CCSTAssignExpr* init_ret = new CCSTAssignExpr(new CCSTPrimaryExprId("ret")
						 , new CCSTAssignOp(equal_t)
						 , new CCSTPostFixExprAssnExpr(new CCSTPrimaryExprId("lcd_recv")
									       ,args));
  /* check ret value, end */

  /* if ret begin */
  CCSTIfStatement* if_stmt = new CCSTIfStatement(new CCSTPrimaryExprId("ret")
						 , new CCSTGoto("out"));
  /* if ret end */


  
  /* switch statement begin */
  std::vector<CCSTStatement*> cases;
  for(std::vector<Rpc*>::iterator it = rps.begin(); it != rps.end(); it ++)
    {
      Rpc* r_tmp = (Rpc*) *it;
      
      std::vector<CCSTDeclaration*> case_dec_empty;

      std::vector<CCSTStatement*> case_body_stmts;
      std::vector<CCSTAssignExpr*> msg_args;
      msg_args.push_back(new CCSTString("FILL IN MESSAGE"));
      case_body_stmts.push_back(new CCSTExprStatement( new CCSTPostFixExprAssnExpr(new CCSTPrimaryExprId("LCD_MSG")
										   , msg_args)));

      // make call to callee. 
      std::vector<CCSTAssignExpr*> args_empty;
      case_body_stmts.push_back(new CCSTExprStatement( new CCSTAssignExpr(new CCSTPrimaryExprId("ret")
						   , new CCSTAssignOp(equal_t)
						   , new CCSTPostFixExprAssnExpr(new CCSTPrimaryExprId(r_tmp->callee_name())
										 , args_empty))));

      case_body_stmts.push_back(new CCSTBreak());
      CCSTCompoundStatement* case_body = new CCSTCompoundStatement(case_dec_empty
								   , case_body_stmts);
      CCSTCaseStatement* tmp_case =  new CCSTCaseStatement(new CCSTPrimaryExprId(r_tmp->enum_name())
							   , case_body);
      cases.push_back(tmp_case);
    }
  /* adding a default case */

  std::vector<CCSTDeclaration*> default_dec_empty;
  std::vector<CCSTStatement*> default_stmts;
  std::vector<CCSTAssignExpr*> lcd_msg_args;
  lcd_msg_args.push_back(new CCSTString("Error unknown function"));
  default_stmts.push_back(new CCSTExprStatement( new CCSTPostFixExprAssnExpr(new CCSTPrimaryExprId("LCD_MSG")
									     , lcd_msg_args)));
  default_stmts.push_back(new CCSTGoto("out"));
  CCSTCompoundStatement* default_body =  new CCSTCompoundStatement(default_dec_empty
								   , default_stmts);
  CCSTDefaultLabelStatement* default_case = new CCSTDefaultLabelStatement( new CCSTExprStatement( new CCSTString("finish") ));
  cases.push_back(default_case);
  /* end of adding default case */

  std::vector<CCSTDeclaration*> switch_dec_empty;
  CCSTCompoundStatement* switch_body = new CCSTCompoundStatement(switch_dec_empty, cases);
  CCSTSwitchStatement* dispatch = new CCSTSwitchStatement(new CCSTPrimaryExprId("ret")
							  , switch_body);
  /* switch statement end */

  /* error checking if begin */
  CCSTIfStatement* error_if = new CCSTIfStatement(new CCSTPrimaryExprId("ret")
						  , new CCSTGoto("out"));
  /* error checking if end */

    /* for loop begin */
  std::vector<CCSTDeclaration*> for_declarations_empty;
  std::vector<CCSTStatement*> for_body_statements;
  for_body_statements.push_back(new CCSTExprStatement( init_ret));
  for_body_statements.push_back(if_stmt);
  for_body_statements.push_back(dispatch);
  for_body_statements.push_back(error_if);
  CCSTCompoundStatement *for_body = new CCSTCompoundStatement(for_declarations_empty, for_body_statements);
  CCSTForLoop* for_loop = new CCSTForLoop(0x0, 0x0, 0x0, for_body); 

  /* for loop end */

  /* labeled statement, out, begin */
  CCSTPlainLabelStatement* out_label = new CCSTPlainLabelStatement("out", new CCSTReturn());
  // doesn't return anything;
  /* labeled statement, out , end */
  
  /* put body together */
  std::vector<CCSTStatement*> body_statements;
  body_statements.push_back(for_loop);
  body_statements.push_back(out_label);
  return new CCSTCompoundStatement(decs_in_body, body_statements);
  /* body complete */
}

/* body for a callee function
 * does unmarshaling, real function call, etc
 */
CCSTCompoundStatement* create_callee_body(Rpc *r)
{
  // unmarshal parameters based on marshal data.
  // which says where params are stored.
  
  std::vector<CCSTDeclaration*> declarations;
  std::vector<CCSTStatement*> statements;
  
  // loop through params, declare a tmp and pull out marshal value
  std::vector<Parameter*> params = r->parameters();
  std::vector<char*> param_names;

  std::vector<CCSTAssignExpr*> unmarshalled_args;
  for(std::vector<Parameter*>::iterator it = params.begin(); it != params.end(); it ++)
    {
      Parameter *p = (Parameter*) *it;
      
      statements.push_back(unmarshal_parameter(p));
      CCSTPrimaryExprId *t = new CCSTPrimaryExprId(p->name());
      unmarshalled_args.push_back(t);
    }

  // make real call and get return value if there is one

  if(r->explicit_return_type()->num() != 5) // not void
    {
      Marshal_type *ret_info = r->explicit_ret_marshal_info();

      CCSTPointer *p = 0x0;
      if(r->explicit_return_type()->num() == 3)
	p = new CCSTPointer();

      std::vector<CCSTDecSpecifier*> ret_type = type(r->explicit_return_type());
      std::vector<CCSTInitDeclarator*> inits;
      inits.push_back(new CCSTInitDeclarator(new CCSTDeclarator(p, new CCSTDirectDecId(ret_info->get_name()))
					      , new CCSTInitializer( new CCSTPostFixExprAssnExpr(new CCSTPrimaryExprId(r->name()) , unmarshalled_args))));

      std::vector<CCSTDeclaration*> cd;
      std::vector<CCSTStatement*> cs;
      
      cd.push_back(new CCSTDeclaration(ret_type, inits));
      MarshalTypeVisitor *visitor = new MarshalTypeVisitor();
      cs.push_back(ret_info->accept(visitor));
      statements.push_back(new CCSTCompoundStatement(cd, cs));
    }
  else
    {
      statements.push_back(new CCSTPostFixExprAssnExpr(new CCSTPrimaryExprId(r->name()) ,unmarshalled_args));
    }
  
  // implicit returns
  std::vector<Marshal_type*> implicit_ret_info = r->implicit_ret_marshal_info();
  for(std::vector<Marshal_type*>::iterator it = implicit_ret_info.begin(); it != implicit_ret_info.end(); it ++)
    {
      Marshal_type *mt = *it;
      MarshalTypeVisitor *visitor = new MarshalTypeVisitor();
      statements.push_back(mt->accept(visitor));
    }

  std::vector<CCSTAssignExpr*> empty_args;
  statements.push_back(new CCSTPostFixExprAssnExpr(new CCSTPrimaryExprId("reply") , empty_args));
  
  return new CCSTCompoundStatement(declarations, statements);
}


CCSTFile* generate_server_header(File *file)
{
  // #ifndef
  // #define
  // #endif
  // enum-specifier: enum id 
  std::vector<CCSTExDeclaration*> definitions; // = new std::vector<CCSTExDeclaration*>();
  // check if there are rpcs
  if(!file->rpc_defs().empty())
    {
      printf("rpc not empty\n");
      definitions.push_back(construct_enum(file));
      // function callee function declarations
      std::vector<Rpc*> rpcs = file->rpc_defs();
      for(std::vector<Rpc*>::iterator it = rpcs.begin(); it != rpcs.end(); it ++)
	{
	  definitions.push_back(construct_callee_declaration((Rpc*) *it));
	}
    }
  definitions.push_back(create_dispatch_function_declaration());
  CCSTFile *c_file = new CCSTFile(definitions);
  return c_file;
}

CCSTDeclaration* construct_callee_declaration(Rpc* r)
{
  std::vector<CCSTDecSpecifier*> specifier; // = new std::vector<CCSTDecSpecifier*>();
  specifier.push_back(new CCSTSimpleTypeSpecifier(int_t));
  char * callee_name = (char*) malloc((strlen(r->name())+strlen("_callee")+1)*sizeof(char));
  sprintf(callee_name, "%s%s", r->name(), "_callee");
  CCSTDirectDecId* id = new CCSTDirectDecId(callee_name);
  
  std::vector<CCSTDecSpecifier*> *s = new std::vector<CCSTDecSpecifier*>();
  s->push_back(new  CCSTSimpleTypeSpecifier(void_t));
  CCSTParamDeclaration *parameter = new CCSTParamDeclaration(s);
  
  std::vector<CCSTParamDeclaration*> p_decs; // = new std::vector<CCSTParamDeclaration*>();
  p_decs.push_back(parameter);
  CCSTParamList *param_list = new CCSTParamList(p_decs);

  CCSTDirectDecParamTypeList *params = new CCSTDirectDecParamTypeList(id, param_list); 
    
  CCSTDeclarator* declarator = new CCSTDeclarator(NULL, params);
  std::vector<CCSTInitDeclarator*> init_declarator; // = new std::vector<CCSTInitDeclarator*>();
  init_declarator.push_back(declarator);
  CCSTDeclaration *func_declaration = new CCSTDeclaration(specifier, init_declarator);
  
  return func_declaration;
}



/* ServerCCSTSourceVisitor */


CCSTFile* generate_server_source(File *file)
{
  printf("In generate_server_source\n");
  // <function-definition> is CCSTFuncDef
  // CCSTDecSpecifier* is <type-specifier> is CCSTTypeSpecifier
  // <declarator> is CCSTDeclarator
  // <declaration> what is this for is CCSTDeclaration
  // <compound-statement> is CCSTCompoundStatement
  // CCSTDeclaration for body
  // CCSTStatement for body

  // see notes in notebook

  /*  
  std::vector<CCSTExDeclaration*> defs;
  CCSTFuncDef* exec_loop = create_exec_loop(file->rpc_defs());
  defs.push_back(exec_loop);
  return new CCSTFile(defs);
  */

  std::vector<CCSTExDeclaration*> definitions;
  
  // dispatch
  /*
  create_function_definition(create_function_declaration()
			     ,create_dispatch_loop_body(file->rpc_defs()));
  */
  std::vector<Rpc*> rps = file->rpc_defs();
  for(std::vector<Rpc*>::iterator it = rps.begin(); it != rps.end(); it ++)
     {
       Rpc* r_tmp = (Rpc*) *it;
       definitions.push_back( create_function_definition(construct_callee_declaration(r_tmp)
							,create_callee_body(r_tmp)));
     }
   
  definitions.push_back(create_function_definition(create_dispatch_function_declaration()
						   , create_dispatch_loop_body(rps)));
   CCSTFile *c_file = new CCSTFile(definitions);
   printf("in server source gen\n");
   return c_file;
}



CCSTCompoundStatement* create_callee_body(Rpc *r)
{
  std::vector<Parameter*> parameters = r->parameters();
  for(std::vector<Parameter*>::iterator it = parameters.begin(); it != parameters.end(); it ++) {
    Parameter *p = (Parameter*) *it;
    if(p->alloc()) {
      AllocateVariableVisitor *worker = new AllocateVariableVisitor();
      p->type()->accept(worker); // allocates space if needed. 
    } else { // if not alloc must be bind?
      // grab from some function. 
    }

    UnmarshalVariableVisitor *worker = new UnmarshalVariableVisitor();
    p->accept(worker);
    
  }

  // make real call.
  
  // for each implicit return, marshal

  // marshal explicit return
  
}
