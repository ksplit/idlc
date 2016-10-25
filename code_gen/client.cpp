#include "ccst.h"
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

CCSTFile* generate_client_header(File* f)
{
  
}

CCSTFile* generate_client_source(File* f)
{ 
  std::vector<CCSTExDeclaration*> definitions;
  for(std::vector<Rpc*>::iterator it = f->rpc_defs().begin(); it != f->rpc_defs().end(); it ++) {
    Rpc *r = (Rpc*) *it;
    definitions.push_back(create_function_definition(create_function_declaration(r)
						     , create_caller_body(r)));
  }
  
  return new CCSTFile(definitions);
}

CCSTCompoundStatement* create_caller_body(Rpc *r)
{
  std::vector<CCSTDeclaration*> declarations;
  std::vector<CCSTStatement*> statements = marshal_parameters(r->parameters());

  // implicit returns
  std::vector<CCSTStatement*> uirs = unmarshal_implicit_return(r->implicit_ret_marshal_info());
  statements.insert(statements.end(), uirs.begin(), uirs.end());
  
  if(r->explicit_return_type()->num() != 5) { // not void
    Marshal_type *ret_info = r->explicit_ret_marshal_info();
    
    
  }
  else {
    statements.push_back(new CCSTReturn());
  }

  return new CCSTCompoundstatement(declarations, statements);
  
}

std::vector<CCSTStatement*> marshal_parameters(std::vector<Parameter*> params)
{
  std::vector<CCSTStatement*> statements;
  
  for(std::vector<Parameter*>::iterator it = params.begin(); it != params.end(); it ++) {
    Parameter *p = (Parameter*) *it;
    statements.push_back(marshal_parameter(p));
  }
  
  return statements;
}

std::vector<CCSTStatement*> unmarshal_implicit_return(std::vector<Parameter*> implicit_returns)
{
  std::vector<CCSTStatement*> statements;
  for(std::vector<Parameter*>::iterator it = implicit_returns.begin(); it != implicit_returns.end(); it ++) {
    Parameter *p = *it;
    UnmarshalTypeVisitor *visitor = new UnmarshalTypeVisitor();
    statements.push_back(mt->accept(visitor));  
  }
  
  return statements;
}

CCSTStatement* unmarshal_explicit_return(Marshal_type* return_info)
{
  CCSTPointer *p = 0x0;
  if(r->explicit_return_type()->num() == 3) {
    p = new CCSTPointer();
  }
  std::vector<CCSTDecSpecifier*> ret_type = type(r->explicit_return_type());
  
  
  UnmarshalTypeVisitor *visitor = new UnmarshalTypeVisitor();

  std::vector<CCSTDeclaration*> d; std::vector<CCSTStatement*> s;
  s.push_back(ret_info->accept(visitor));
  s.push_back( new CCSTReturn(new CCSTPrimaryExprId("internal_ret")));
}





