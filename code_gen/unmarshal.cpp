#include "code_gen.h"

/*
 * Want this code to be as dumb as possible.
 * The complexity for deciding what to unmarshal occurs previously.
 * Assume structure a variable belongs to has been previously allocated, at the obvious name.
 * This function should never be called on a variable whose type is a projection.
 */

std::vector<CCSTStatement*> unmarshal_return_variable_no_check(Variable *v, Channel::ChannelType type)
{
  std::vector<CCSTStatement*> statements;
  

    std::cout << " Unmarshal: var name " << v->identifier() << std::endl;
    Assert(v->marshal_info() != 0x0, "Error: no marshal info\n");
    int reg = v->marshal_info()->get_register();
    std::string func_name;
    std::vector<CCSTAssignExpr*> access_reg_args;

    if (type == Channel::SyncChannel) {
      func_name = access_register_mapping(reg);
    } else {
      func_name = load_async_reg_mapping(reg);
      access_reg_args.push_back(new CCSTPrimaryExprId("response"));
    }

    statements.push_back(new CCSTExprStatement( new CCSTAssignExpr (access(v)
                    , equals()
                    , function_call(func_name, access_reg_args))));


  return statements;
}

std::vector<CCSTStatement*> unmarshal_variable_no_check(Variable *v, Channel::ChannelType type)
{
  std::vector<CCSTStatement*> statements;

  if (v->type()->num() == PROJECTION_TYPE || v->type()->num() == PROJECTION_CONSTRUCTOR_TYPE) {
    // loop through fields
    ProjectionType *pt = dynamic_cast<ProjectionType*>(v->type());
    Assert(pt != 0x0, "Error: dynamic cast to projection failed!\n");

    for (auto pf : *pt) {
      std::vector<CCSTStatement*> tmp_stmts = unmarshal_variable_no_check(pf, type);
      statements.insert(statements.end(), tmp_stmts.begin(), tmp_stmts.end());
    }

  } else {
    std::cout << " Unmarshal: var name " << v->identifier() << std::endl;
    Assert(v->marshal_info() != 0x0, "Error: no marshal info\n");
    int reg = v->marshal_info()->get_register();
    std::string func_name;
    std::vector<CCSTAssignExpr*> access_reg_args;

    if (type == Channel::SyncChannel) {
      func_name = access_register_mapping(reg);
    } else {
      func_name = load_async_reg_mapping(reg);
      access_reg_args.push_back(new CCSTPrimaryExprId("response"));
    }

    statements.push_back(new CCSTExprStatement( new CCSTAssignExpr (access(v)
								    , equals()
								    , function_call(func_name, access_reg_args))));
  }

  return statements;
}

CCSTPostFixExprAssnExpr* unmarshal_variable(Variable *v, Channel::ChannelType type, const std::string& req_resp)
{
  Assert(v->marshal_info() != 0x0, "Error: no marshal info\n");
  int reg = v->marshal_info()->get_register();

  std::string func_name;
  std::vector<CCSTAssignExpr*> access_reg_args;

  if (type == Channel::SyncChannel) {
    func_name = access_register_mapping(reg);
  } else {
    func_name = load_async_reg_mapping(reg);
    access_reg_args.push_back(new CCSTPrimaryExprId(req_resp));
  }
  return function_call(func_name, access_reg_args);
}

// unmarshals variable. 
std::vector<CCSTStatement*> unmarshal_variable_caller(Variable *v, Channel::ChannelType type)
{
  std::vector<CCSTStatement*> statements;

  if(v->type()->num() == PROJECTION_TYPE || v->type()->num() == PROJECTION_CONSTRUCTOR_TYPE) {
    ProjectionType *pt = dynamic_cast<ProjectionType*>(v->type());
    Assert(pt != 0x0, "Error: dynamic cast to projection failed!\n");
    
    for (auto pf : *pt) {
      if(pf->out()) {
	std::vector<CCSTStatement*> tmp_stmt = unmarshal_variable_caller(pf, type);
	statements.insert(statements.end(), tmp_stmt.begin(), tmp_stmt.end());
      }
    }
    
  } else {
    Assert(v->marshal_info() != 0x0, "Error: variable has no marshalling information\n");
    
    int reg = v->marshal_info()->get_register();
    std::string func_name;
    if (type == Channel::SyncChannel)
      func_name = access_register_mapping(reg);
    else
      func_name = load_async_reg_mapping(reg);

    std::vector<CCSTAssignExpr*> reg_func_args_empty;
    
    statements.push_back(new CCSTExprStatement(new CCSTAssignExpr(access(v)
								  , equals()
								  , function_call(func_name
										  , reg_func_args_empty))));
  }
  
  return statements;
}

std::vector<CCSTStatement*> unmarshal_void_pointer(Variable *v)
{
  std::vector<CCSTStatement*> statements;
  std::vector<CCSTAssignExpr*> cptr_alloc_args;
  std::vector<CCSTAssignExpr*> sync_recv_args;
  std::vector<CCSTAssignExpr*> set_cr0_args;
  std::vector<CCSTAssignExpr*> set_cr0_args2;
  std::vector<CCSTAssignExpr*> map_virt_args;
  std::vector<CCSTAssignExpr*> empty_args;

  cptr_alloc_args.push_back(
    new CCSTUnaryExprCastExpr(reference(),
      new CCSTPrimaryExprId(v->identifier() + "_cptr")));

  sync_recv_args.push_back(new CCSTPrimaryExprId("sync_ep"));

  set_cr0_args.push_back(new CCSTPrimaryExprId(v->identifier() + "_cptr"));

  set_cr0_args2.push_back(new CCSTPrimaryExprId("CAP_CPTR_NULL"));

  map_virt_args.push_back(new CCSTPrimaryExprId(v->identifier() + "_cptr"));
  map_virt_args.push_back(new CCSTPrimaryExprId("mem_order"));
  map_virt_args.push_back(
    new CCSTUnaryExprCastExpr(reference(),
      new CCSTPrimaryExprId(v->identifier() + "_gva")));

  statements.push_back(
    new CCSTExprStatement(
      new CCSTAssignExpr(new CCSTPrimaryExprId("sync_ret"), equals(),
        function_call("lcd_cptr_alloc", cptr_alloc_args))));

  statements.push_back(
    if_cond_fail(new CCSTPrimaryExprId("sync_ret"), "failed to get cptr"));

  statements.push_back(
    new CCSTExprStatement(
      new CCSTPostFixExprAssnExpr(new CCSTPrimaryExprId("lcd_set_cr0"),
        set_cr0_args)));

  statements.push_back(
    new CCSTExprStatement(
      new CCSTAssignExpr(new CCSTPrimaryExprId("sync_ret"), equals(),
        function_call("lcd_sync_recv", sync_recv_args))));

  statements.push_back(
    new CCSTExprStatement(
      new CCSTPostFixExprAssnExpr(new CCSTPrimaryExprId("lcd_set_cr0"),
        set_cr0_args2)));

  statements.push_back(
    if_cond_fail(new CCSTPrimaryExprId("sync_ret"), "failed to recv"));

  statements.push_back(
    new CCSTExprStatement(
      new CCSTAssignExpr(new CCSTPrimaryExprId("mem_order"), equals(),
        function_call("lcd_r0", empty_args))));

  statements.push_back(
    new CCSTExprStatement(
      new CCSTAssignExpr(new CCSTPrimaryExprId(v->identifier() + "_offset"),
        equals(), function_call("lcd_r1", empty_args))));

  statements.push_back(
    new CCSTExprStatement(
      new CCSTAssignExpr(new CCSTPrimaryExprId("sync_ret"), equals(),
        function_call("lcd_map_virt", map_virt_args))));

  // TODO: Add code to delete the mapping
  statements.push_back(
    if_cond_fail(new CCSTPrimaryExprId("sync_ret"),
      "failed to map void *" + v->identifier()));

  return statements;
}

// unmarshals variable. and sets in container if there is a container.
std::vector<CCSTStatement*> unmarshal_variable_callee(Variable *v, Channel::ChannelType type)
{
  std::vector<CCSTStatement*> statements;

  if(v->type()->num() == FUNCTION_TYPE) {
    /// Nothing needs to be unmarshalled for a function pointer
    /// Function ptr's container will have to be marshalled and is done
    /// allocation of its container
    return statements;
  }


  if(v->type()->num() == PROJECTION_TYPE || v->type()->num() == PROJECTION_CONSTRUCTOR_TYPE) {
    ProjectionType *pt = dynamic_cast<ProjectionType*>(v->type());
    
    for (auto pf : *pt) {
      if(pf->in()) {
	std::vector<CCSTStatement*> tmp_stmt = unmarshal_variable_callee(pf, type);
	statements.insert(statements.end(), tmp_stmt.begin(), tmp_stmt.end());
      }
    }
    
  } else if(v->type()->num() == VOID_TYPE) {
    std::vector<CCSTStatement*> tmp_stmt = unmarshal_void_pointer(v);
    statements.insert(statements.end(), tmp_stmt.begin(), tmp_stmt.end());
  } else {
    Assert(v->marshal_info() != 0x0, "Error: variable has no marshalling information\n");
    
    int reg = v->marshal_info()->get_register();
    std::vector<CCSTAssignExpr*> reg_func_args_empty;

    std::string reg_func;
    if (type == Channel::SyncChannel) {
      reg_func= access_register_mapping(reg);
    } else {
      reg_func = load_async_reg_mapping(reg);
      reg_func_args_empty.push_back(new CCSTPrimaryExprId("request"));
    }


    if (v->container() != 0x0) {
      ProjectionType *v_container_type = dynamic_cast<ProjectionType*>(v->container()->type());
      Assert(v_container_type != 0x0, "Error: dynamic cast to projection type failed\n");
      ProjectionField *v_container_real_field = v_container_type->get_field(v->type()->name());
      Assert(v_container_real_field != 0x0, "Error: could not find field in structure\n");
      

      statements.push_back(new CCSTExprStatement(new CCSTAssignExpr(access(v_container_real_field)
								    , equals()
								    , function_call(reg_func
										    , reg_func_args_empty))));
    } else {
      // no container
      statements.push_back(new CCSTExprStatement(new CCSTAssignExpr(access(v)
								    , equals()
								    , function_call(reg_func
										    , reg_func_args_empty))));
    }
  }
  return statements;
}

std::vector<CCSTStatement*> unmarshal_container_refs_caller(Variable *v, Channel::ChannelType type)
{
  std::vector<CCSTStatement*> statements;

  if(v->container() != 0x0) {
    if(v->alloc_callee()) { // except a remote ref
      statements.push_back(set_remote_ref(v, type));
    }
  }
  
  if( v->type()->num() == PROJECTION_TYPE || v->type()->num() == PROJECTION_CONSTRUCTOR_TYPE) {
    ProjectionType *pt = dynamic_cast<ProjectionType*>(v->type());
    Assert(pt != 0x0, "Error: dynamic cast to projection type failed\n");
    
    for (auto pf : *pt) {
      if(pf->out()) {
	std::vector<CCSTStatement*> tmp_stmts = unmarshal_container_refs_caller(pf, type);
	statements.insert(statements.end(), tmp_stmts.begin(), tmp_stmts.end());
      }
    }
  }
  
  return statements;
}
