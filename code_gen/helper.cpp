#include "ccst.h"
#include "code_gen.h"

CCSTExDeclaration* construct_enum(Module *m)
{
  std::string enum_name("dispatch_t");
  std::vector<Rpc*> rpcs = m->rpc_definitions();
  CCSTEnumeratorList *el = construct_enumlist(rpcs);
  CCSTEnumSpecifier *e = new CCSTEnumSpecifier(enum_name, el);
  std::vector<CCSTDecSpecifier*> tmp;
  tmp.push_back(e);
  std::vector<CCSTInitDeclarator*> empty;
  CCSTDeclaration *declaration = new CCSTDeclaration(tmp, empty);
  
  return declaration;
}

CCSTEnumeratorList* construct_enumlist(std::vector<Rpc *> rpcs)
{
  // list of functions to put in enum.
  std::vector<CCSTEnumerator*>* list = new std::vector<CCSTEnumerator*>();
  for (auto r : rpcs) {
    list->push_back(new CCSTEnumerator(r->enum_name()));
  }
  CCSTEnumeratorList *enum_list = new CCSTEnumeratorList(list);
  return enum_list;
}

/* Creates a function definition
 * from a function declaration
 * and a body
 */
CCSTFuncDef* function_definition(CCSTDeclaration* function_declaration, CCSTCompoundStatement *body)
{
  Assert(function_declaration->decs_.size() == 1, "Error: More than one initializer/declarator in function declaration");
  
  CCSTDeclarator *func = dynamic_cast<CCSTDeclarator*>(function_declaration->decs_.at(0));
  Assert(func != 0, "Error: dynamic cast from CCSTInitDeclarator to CCSTDeclarator has failed!");
  
  std::vector<CCSTDeclaration*> decs; // not sure what this is for. unnecessary?

  return new CCSTFuncDef(function_declaration->specifier_, func, decs, body, function_declaration->attributes_);
}

CCSTParamTypeList* parameter_list(std::vector<Parameter*> params)
{
  std::vector<CCSTParamDeclaration*> param_decs;
  for (auto p : params) {
    /// Handle function pointers separately
    if (p->type()->num() == FUNCTION_TYPE) {
      Function *f = dynamic_cast<Function*>(p->type());
      std::vector<CCSTDecSpecifier*> new_fp_return_type = type2(
        f->return_var_->type());

      /// loop through rpc parameters and add them to the parameters for the new fp
      std::vector<CCSTParamDeclaration*> func_pointer_params;

      for (auto p : *f) {
        std::vector<CCSTDecSpecifier*> fp_param_tmp = type2(p->type());
        func_pointer_params.push_back(
          new CCSTParamDeclaration(fp_param_tmp,
            new CCSTDeclarator(pointer(p->pointer_count()),
              new CCSTDirectDecId(""))));
      }

      CCSTDeclarator *dec = new CCSTDeclarator(NULL,
        new CCSTDirectDecParamTypeList(
          new CCSTDirectDecDec(
            new CCSTDeclarator(new CCSTPointer(),
              new CCSTDirectDecId(f->name()))),
          new CCSTParamList(func_pointer_params)));
      param_decs.push_back(new CCSTParamDeclaration(new_fp_return_type, dec));

    } else {
      /// Handle all other types
      param_decs.push_back(
        new CCSTParamDeclaration(type2(p->type()),
          new CCSTDeclarator(pointer(p->pointer_count()),
            new CCSTDirectDecId(p->identifier()))));
    }
  }

  return new CCSTParamList(param_decs);
}

/* creates a function declaration from an rpc
 */
CCSTDeclaration* function_declaration(Rpc* r)
{
  std::vector<CCSTDecSpecifier*> specifier = type2(r->return_variable()->type());
  
  std::vector<CCSTInitDeclarator*> func;
  CCSTPointer *p = pointer(r->return_variable()->pointer_count());
  
  CCSTDirectDecId *name = new CCSTDirectDecId(r->name());
  CCSTParamTypeList *param_list = parameter_list(r->parameters());

  CCSTDirectDecParamTypeList *name_params = new CCSTDirectDecParamTypeList(name, param_list);
  
  func.push_back(new CCSTDeclarator(p, name_params));
  
  return new CCSTDeclaration(specifier, func);
}

CCSTDeclaration* function_pointer_function_declaration(Rpc *r, const std::string &_postfix)
{
  std::vector<CCSTDecSpecifier*> specifier = type2(r->return_variable()->type());
  
  std::vector<CCSTInitDeclarator*> func;
  CCSTPointer *p = pointer(r->return_variable()->pointer_count());
  
  CCSTDirectDecId *name = new CCSTDirectDecId(r->name() + _postfix);

  std::vector<Parameter*> all_params;

  std::vector<Parameter*> r_parameters = r->parameters();
  std::vector<Parameter*> r_hidden_args = r->hidden_args_;

  all_params.insert(all_params.end(), r_parameters.begin(), r_parameters.end());
  all_params.insert(all_params.end(), r_hidden_args.begin(), r_hidden_args.end());
  
  CCSTParamTypeList *param_list = parameter_list(all_params);

  CCSTDirectDecParamTypeList *name_params = new CCSTDirectDecParamTypeList(name, param_list);
  
  func.push_back(new CCSTDeclarator(p, name_params));
  
  return new CCSTDeclaration(specifier, func);
  
}

// 
CCSTPostFixExpr* access(Variable *v)
{
  if(v->accessor() == 0x0) {
    int pc = v->pointer_count();
    if (pc > 1) {
      int tmp = pc-1;
      CCSTCastExpr* init = new CCSTPrimaryExprId(v->identifier());
      while(tmp > 0) {
	init = dereference(init);
	tmp -= 1;
      }
      return new CCSTPrimaryExpr(init);
    } else {
      return new CCSTPrimaryExprId(v->identifier());
    }
  } else {
    if (v->accessor()->pointer_count() == 0) {
      int pc = v->pointer_count();
      if (pc > 1) {
	int tmp = pc-1;
        CCSTCastExpr * init = new CCSTPostFixExprAccess(access(v->accessor()), object_access_t, v->identifier());
	while(tmp > 0) {
	  init = dereference(init);
	  tmp -= 1;
	}
	return new CCSTPrimaryExpr(init);
	
      } else {
	return new CCSTPostFixExprAccess(access(v->accessor()), object_access_t, v->identifier());
      }
    } else {
      int pc = v->pointer_count();
      if (pc > 1) {
	int tmp = pc-1;
	CCSTCastExpr *init = new CCSTPostFixExprAccess(access(v->accessor()), pointer_access_t, v->identifier());
	while(tmp > 0) {
	  init = dereference(init);
	  tmp -= 1;
	}
	return new CCSTPrimaryExpr(init);
      } else {
	return new CCSTPostFixExprAccess(access(v->accessor()), pointer_access_t, v->identifier());
      }
    }
  }
}

CCSTIfStatement* if_cond_fail(CCSTExpression *cond, const std::string& err_msg)
{
  std::vector<CCSTDeclaration*> if_body_declarations;
  std::vector<CCSTStatement*> if_body_statements;
  
  std::vector<CCSTAssignExpr*> liblcd_err_args;
  liblcd_err_args.push_back(new CCSTString(err_msg));
  if_body_statements.push_back(new CCSTExprStatement( function_call("LIBLCD_ERR", liblcd_err_args)));

  std::vector<CCSTAssignExpr*> lcd_exit_args;
  lcd_exit_args.push_back(new CCSTInteger(-1));
  if_body_statements.push_back(new CCSTExprStatement( function_call("lcd_exit", lcd_exit_args)));

  CCSTCompoundStatement *if_body = new CCSTCompoundStatement(if_body_declarations, if_body_statements);
  return new CCSTIfStatement(cond, if_body);
}

CCSTIfStatement* if_cond_fail_goto(CCSTExpression *cond, const std::string& err_msg, const std::string& goto_label)
{
  std::vector<CCSTDeclaration*> if_body_declarations;
  std::vector<CCSTStatement*> if_body_statements;
  
  std::vector<CCSTAssignExpr*> liblcd_err_args;
  liblcd_err_args.push_back(new CCSTString(err_msg));
  if_body_statements.push_back(new CCSTExprStatement( function_call("LIBLCD_ERR", liblcd_err_args)));

  if_body_statements.push_back(new CCSTGoto(goto_label));

  CCSTCompoundStatement *if_body = new CCSTCompoundStatement(if_body_declarations, if_body_statements);
  return new CCSTIfStatement(cond, if_body);
}
