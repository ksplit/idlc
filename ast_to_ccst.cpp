#include "ccst.h"
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include "assert.h"

/* example
   
   #ifndef LCD_PROTOTYPE_MANUFACTURER_IDL_H
   #define LCD_PROTOTYPE_MANUFACTURER_IDL_H
   
   enum manufacturer_interface_enum {
   MANUFACTURER_MK_ENGINE,
   MANUFACTURER_MK_AUTOMOBILE,
   MANUFACTURER_FREE_ENGINE,
   MANUFACTURER_FREE_AUTOMOBILE,
   MANUFACTURER_DIE,
   };
   
   Locations of manufacturer's boot cptrs 
   #define MANUFACTURER_DEALER_INTERFACE_CAP 31
   
   #endif
*/

/* producing:
   
   declaration --> declaration-specifier --> type-specifier --> enum-specifier
   
   enum-specifier: enum id { enumerator-list };
   
   enumerator-list = enumerator
   | enumerator-list , enumerator
   
   enumerator = id
   | id = constant-expression
*/
CCSTFile* generate_client_header(File* f)
{
}

CCSTFile* generate_client_source(File* f)
{
  
  std::vector<CCSTExDeclaration*> definitions;
  
  
  CCSTFile *c_file = new CCSTFile(definitions);
  return c_file;
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
  
  std::vector<CCSTParamDeclaration*> *p_decs = new std::vector<CCSTParamDeclaration*>();
  p_decs->push_back(parameter);
  CCSTParamList *param_list = new CCSTParamList(p_decs);

  CCSTDirectDecParamTypeList *params = new CCSTDirectDecParamTypeList(id, param_list); 
    
  CCSTDeclarator* declarator = new CCSTDeclarator(NULL, params);
  std::vector<CCSTInitDeclarator*> init_declarator; // = new std::vector<CCSTInitDeclarator*>();
  init_declarator.push_back(declarator);
  CCSTDeclaration *func_declaration = new CCSTDeclaration(specifier, init_declarator);
  
  return func_declaration;
}

CCSTExDeclaration* construct_enum(File *f)
{
  const char* enum_name = "todo";
  CCSTEnumeratorList *el = construct_enumlist(f->rpc_defs());
  CCSTEnumSpecifier *e = new CCSTEnumSpecifier(enum_name, el);
  std::vector<CCSTDecSpecifier*> tmp; // = new std::vector<CCSTDecSpecifier*>();
  tmp.push_back(e);
  std::vector<CCSTInitDeclarator*> empty;
  CCSTDeclaration *declaration = new CCSTDeclaration(tmp, empty);
  
  return declaration;
}

const char* construct_enum_name()
{
  return "todo";
}

CCSTEnumeratorList* construct_enumlist(std::vector<Rpc *> rps)
{
  // list of functions to put in enum.
  std::vector<CCSTEnumerator*>* list = new std::vector<CCSTEnumerator*>();
  for(std::vector<Rpc*>::iterator it = rps.begin(); it != rps.end(); it ++)
    {
      Rpc *r = *it;
      char* upper_name = string_to_upper(r->name());
      char* enum_name = (char*)malloc((sizeof(upper_name)+9+1)*sizeof(char));
      sprintf(enum_name, "%s_CALLEE_T", upper_name);
      list->push_back(new CCSTEnumerator(enum_name));
    }
  CCSTEnumeratorList *enum_list = new CCSTEnumeratorList(list);
  return enum_list;
}

char* string_to_upper(char* str)
{
  char* ret = (char*) malloc((sizeof(str)+1)*sizeof(char));
  int i;
  for(i = 0; i < sizeof(str); i ++)
    {
      char tmp = str[i];
      ret[i] = toupper(tmp);
    }
  ret[i] = '\0';
  return ret;
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
   
   CCSTFile *c_file = new CCSTFile(definitions);
   printf("in server source gen\n");
   return c_file;
}

/* ------------------------ new ---------------------------- */



int count_nested_pointer(Type *p)
{
  if(p->num() != 3)
    return 0;
  else
    {
      PointerType *tmp = dynamic_cast<PointerType*>(p);
      return 1 + count_nested_pointer(tmp->type());
    }
}

/* creates a pointer, or pointer to pointer, etc*/
/* may need to be changed if want to do something like
 *    int * const name
 * int * volatile name
 */
CCSTPointer* create_pointer(int p_count)
{
  if(p_count == 0)
    return 0x0;
  
  if(p_count == 1)
    return new CCSTPointer();
  
  return new CCSTPointer(create_pointer(p_count - 1));
}

/* Creates a function definition
 * from a function declaration
 * and a body
 */
CCSTFuncDef* create_function_definition(CCSTDeclaration* function_declaration, CCSTCompoundStatement *body)
{
  Assert(function_declaration->decs_.size() == 1, "Error: More than one initializer/declarator in function declaration");
  
  CCSTDeclarator *func = dynamic_cast<CCSTDeclarator*>(function_declaration->decs_.at(0));
  Assert(func != 0, "Error: dynamic cast from CCSTInitDeclarator to CCSTDeclarator has failed!");
  
  std::vector<CCSTDeclaration*> decs; // not sure what this is for. unnecessary?
  
  return new CCSTFuncDef(function_declaration->specifier_, func, decs, body);
}

CCSTParamTypeList* create_parameter_list()
{
  
}

/* creates a function declaration
 * from an rpc, not callee
 */
CCSTDeclaration* create_function_declaration(Rpc* r)
{
  std::vector<CCSTDecSpecifier*> specifier = get_type(r->return_type());
  
  std::vector<CCSTInitDeclarator*> func; // = new std::vector<CCSTInitDeclarator*>(); // pointer name, params
  int pointer_count = count_nested_pointer(r->return_type());
  
  CCSTPointer *p = create_pointer(pointer_count);
  
  CCSTDirectDecId *name = new CCSTDirectDecId(r->name());
  CCSTParamTypeList *param_list = create_parameter_list();
  CCSTDirectDecParamTypeList *name_params = new CCSTDirectDecParamTypeList(name, param_list);
  
  func.push_back(new CCSTDeclarator(p, name_params));
  
  return new CCSTDeclaration(specifier, func);
}

/* create a function declaration
 * not from an rpc
 * need return type
 * need name
 * need params
 */


CCSTDeclaration* create_function_declaration()
{
}

/* specific function body creators */

CCSTCompoundStatement* create_dispatch_loop_body(std::vector<Rpc*>* rps)
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
  for(std::vector<Rpc*>::iterator it = rps->begin(); it != rps->end(); it ++)
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
      const char* param_tmp_name = "_param1";
      CCSTCompoundStatement *tmp = unmarshal_parameter(p);
      
      CCSTPrimaryExprId *t = new CCSTPrimaryExprId(param_tmp_name);
      unmarshalled_args.push_back(t);
      statements.push_back(tmp);
    }

  // pulled out params make function call

  CCSTInitializer *call = new CCSTInitializer( new CCSTPostFixExprAssnExpr( new CCSTPrimaryExprId(r->name())
									    ,unmarshalled_args ) );
  CCSTPointer *pp = 0x0;
  if(r->return_type()->num() == 3)
    {
      pp = create_pointer(count_nested_pointer(r->return_type()));
    }
  
  const char *ret_name = "ret";
  CCSTDeclarator *ret_value_name = new CCSTDeclarator(pp
						      , new CCSTDirectDecId(ret_name) );

  std::vector<CCSTDecSpecifier*> real_call_specifier;
  std::vector<CCSTInitDeclarator*> real_call_decs;
  
  real_call_specifier = get_type(r->return_type());
  real_call_decs.push_back(new CCSTInitDeclarator(ret_value_name
						  , call));
  
  CCSTDeclaration *ret_value = new CCSTDeclaration(real_call_specifier
						   , real_call_decs);
  
  declarations.push_back(ret_value);

  // marshal return value
  std::vector<CCSTAssignExpr*> ret_arg;
  ret_arg.push_back(new CCSTPrimaryExprId(ret_name));
  CCSTPostFixExprAssnExpr *marshal_ret = new CCSTPostFixExprAssnExpr(new CCSTPrimaryExprId("func_name")
								     ,ret_arg);
  statements.push_back(new CCSTExprStatement(marshal_ret));
  return new CCSTCompoundStatement(declarations, statements);
}

CCSTCompoundStatement* unmarshal_projection_parameter(Parameter *param, ProjectionType *pt)
{
  
}

CCSTTypeName* type_cast(Type *t)
{
  
}

CCSTCompoundStatement* unmarshal_pointer_parameter(Parameter *param, PointerType *pt)
{
  CCSTPointer *__p = create_pointer(count_nested_pointer(pt));
  
  std::vector<CCSTAssignExpr*> args;
  
  CCSTTypeName *type_name = type_cast(pt);
  args.push_back(new CCSTUnaryExprSizeOf(type_name));

  const char *param_name = param->name();
  
  
  // malloc space for the pointer
  CCSTDeclarator *name = new CCSTDeclarator(__p, new CCSTDirectDecId(param_name));
  CCSTInitializer *init = new CCSTInitializer(new CCSTCastExpr(type_name
							       , new CCSTPostFixExprAssnExpr(new CCSTPrimaryExprId("malloc")
											     , args)));
  CCSTInitDeclarator *get_value = new CCSTInitDeclarator(name
							 ,init);
  // malloced space.

  // now put value in pointer
  std::vector<CCSTAssignExpr*> assn_exprs;

  // *name
  CCSTUnaryExprCastExpr *set_value = new CCSTUnaryExprCastExpr(new CCSTUnaryOp(unary_mult_t)
							       , new CCSTPrimaryExprId(param_name));

  // value
  // (type) lcd_r0()
  std::vector<CCSTAssignExpr*> args2;
  CCSTCastExpr *value = new CCSTCastExpr( type_cast(pt->type())
					  , new CCSTPostFixExprAssnExpr(new CCSTPrimaryExprId("func_to_call")
									, args2) ); 
  
  // combining them
  // *name = value
  CCSTAssignExpr *param_eq_value = new CCSTAssignExpr(set_value, new CCSTAssignOp(equal_t)
						      , value);
  assn_exprs.push_back(param_eq_value);
  CCSTExprStatement *expr_stmt = new CCSTExprStatement(new CCSTExpression(assn_exprs));
  // 


  std::vector<CCSTInitDeclarator*> init_decs;
  init_decs.push_back(get_value);
  
  CCSTDeclaration *declaration = new CCSTDeclaration(get_type(pt)
						     , init_decs);
  
  std::vector<CCSTDeclaration*> comp_stmt_decs;
  comp_stmt_decs.push_back(declaration);

  std::vector<CCSTStatement*> comp_stmt_stmts;
  comp_stmt_stmts.push_back(expr_stmt);
  
  return new CCSTCompoundStatement(comp_stmt_decs, comp_stmt_stmts);
}

CCSTCompoundStatement* unmarshal_parameter(Parameter *p)
{
  Type *t = p->type();
  
  switch(t->num())
    {
    case 3:
      {
	PointerType *pt = dynamic_cast<PointerType*>(t);
	return unmarshal_pointer_parameter(p, pt);
      }
    case 4:
      {
	ProjectionType *pt = dynamic_cast<ProjectionType*>(t);
	return unmarshal_projection_parameter(p, pt);
      }
    default:
      {
	
      }
    }

  // int reg_where_marshalled = t->


}

/* body for a caller function
 * does marshaling, ipc, etc
 */
CCSTCompoundStatement* create_caller_body(Rpc *r)
{
  // marshal
  // ipc call

  // unmarshal
  // return result
}


/*
 * Takes a Type and returns the CAST equivalent
 * Didn't feel it was necessary to write a visitor
 * When this will do essentially the same thing.
 * Slightly grosser than visitor because must do casting
 */
std::vector<CCSTDecSpecifier*> get_type(Type *t)
{
  std::vector<CCSTDecSpecifier*> ret;
  switch (t->num())
    {
    case 1: // typedef
      {
	// Choice.  use "real type" or use user defined type
	// and put typedef at top of file.
	// go with this, will make code more readable
	
	Typedef *td = dynamic_cast<Typedef*>(t);
	CCSTTypedefName *tmp = new CCSTTypedefName(td->alias());
	ret.push_back(tmp);
	return ret;
      }
    case 2: // Integer type
      {
	IntegerType *it = dynamic_cast<IntegerType*>(t);
	return get_integer_type(it);
      }
    case 3: // Pointer Type
      {
	// don't "care" about pointer at this point,
	// return type that it is a pointer to
	PointerType *pt = dynamic_cast<PointerType*>(t);
	return get_type(pt->type());
      }
    case 4: // Projection Type
      {
	
	break;
      }
    case 5: // Void Type
      {
	ret.push_back( new CCSTSimpleTypeSpecifier(void_t));
	return ret;
      }
    default:
      {
	printf("error");
      }
    }
}

std::vector<CCSTDecSpecifier*> get_integer_type(IntegerType *it)
{
  std::vector<CCSTDecSpecifier*> ret;
  if(it->is_unsigned())
    {
      ret.push_back(new CCSTSimpleTypeSpecifier(unsigned_t));
    }
  
  switch (it->int_type())
    {
    case pt_char_t:
      {
	ret.push_back(new CCSTSimpleTypeSpecifier(char_t));
	break;
      }
    case pt_short_t:
      {
	ret.push_back(new CCSTSimpleTypeSpecifier(short_t));
	break;
      }
    case pt_int_t:
      {
	ret.push_back(new CCSTSimpleTypeSpecifier(int_t));
	break;
      }
    case pt_long_t:
      {
	ret.push_back(new CCSTSimpleTypeSpecifier(long_t));
	break;
      }
    case pt_longlong_t:
      {
	ret.push_back(new CCSTSimpleTypeSpecifier(long_t));
	ret.push_back(new CCSTSimpleTypeSpecifier(long_t));
	break;
      }
    case pt_capability_t:
      {
	ret.push_back(new CCSTTypedefName("capability_t"));
	break;
      }
    default:
      {
	printf("todo");
      }
    }
  return ret;
}




/*
// move to new file
CCSTDeclaration* container_struct_declaration()
{
  // needs to contain actual struct
  // needs to contain a capability
  // needs special asm code
 
  // what else
}
*/
