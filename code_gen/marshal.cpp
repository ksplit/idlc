#include "code_gen.h"


CCSTStatement* marshal_void_pointer(Variable *v)
{
  std::vector<CCSTDeclaration*> declarations;
  std::vector<CCSTStatement*> statements;

  std::vector<CCSTDecSpecifier*> spec_ull;
  std::vector<CCSTSpecifierQual*> spec_ull2;
  std::vector<CCSTDecSpecifier*> spec_int;
  std::vector<CCSTDecSpecifier*> spec_cptr;

  std::vector<CCSTInitDeclarator*> decs_sz;
  std::vector<CCSTInitDeclarator*> decs_off;
  std::vector<CCSTInitDeclarator*> decs_cptr;
  std::vector<CCSTInitDeclarator*> decs_sret;

  spec_cptr.push_back(new CCSTTypedefName("cptr_t"));

  spec_ull.push_back(
    new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::UnsignedTypeSpec));
  spec_ull.push_back(
    new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::LongTypeSpec));
  spec_ull2.push_back(
    new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::UnsignedTypeSpec));
  spec_ull2.push_back(
    new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::LongTypeSpec));
  spec_int.push_back(
    new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::IntegerTypeSpec));

  decs_sret.push_back(
    new CCSTInitDeclarator(
      new CCSTDeclarator(NULL, new CCSTDirectDecId("sync_ret"))));
  decs_off.push_back(
    new CCSTInitDeclarator(
      new CCSTDeclarator(NULL,
        new CCSTDirectDecId(v->identifier() + "_offset"))));
  decs_cptr.push_back(
    new CCSTInitDeclarator(
      new CCSTDeclarator(NULL,
        new CCSTDirectDecId(v->identifier() + "_cptr"))));
  decs_sz.push_back(
    new CCSTInitDeclarator(
      new CCSTDeclarator(NULL,
        new CCSTDirectDecId(v->identifier() + "_mem_sz"))));

  declarations.push_back(new CCSTDeclaration(spec_int, decs_sret));
  declarations.push_back(new CCSTDeclaration(spec_ull, decs_sz));
  declarations.push_back(new CCSTDeclaration(spec_ull, decs_off));
  declarations.push_back(new CCSTDeclaration(spec_cptr, decs_cptr));

  std::vector<CCSTAssignExpr*> virt_cptr_args;
  std::vector<CCSTAssignExpr*> gva_args;

  gva_args.push_back(
    new CCSTCastExpr(new CCSTTypeName(spec_ull2, NULL),
      new CCSTPrimaryExprId(v->identifier())));
  virt_cptr_args.push_back(function_call("__gva", gva_args));

  virt_cptr_args.push_back(
    new CCSTUnaryExprCastExpr(reference(),
      new CCSTPrimaryExprId(v->identifier() + "_cptr")));

  virt_cptr_args.push_back(
    new CCSTUnaryExprCastExpr(reference(),
      new CCSTPrimaryExprId(v->identifier() + "_mem_sz")));

  virt_cptr_args.push_back(
    new CCSTUnaryExprCastExpr(reference(),
      new CCSTPrimaryExprId(v->identifier() + "_offset")));

  statements.push_back(
    new CCSTExprStatement(
      new CCSTAssignExpr(new CCSTPrimaryExprId("sync_ret"), equals(),
        function_call("lcd_virt_to_cptr", virt_cptr_args))));

  statements.push_back(
    if_cond_fail(new CCSTPrimaryExprId("sync_ret"), "virt to cptr failed"));

  return new CCSTCompoundStatement(declarations, statements);
}

CCSTStatement* marshal_void_delayed(Variable *v)
{
  std::vector<CCSTDeclaration*> declarations;
  std::vector<CCSTStatement*> statements;

  std::vector<CCSTAssignExpr*> sync_send_args;
  std::vector<CCSTAssignExpr*> set_cr0_args;
  std::vector<CCSTAssignExpr*> set_cr0_args2;
  std::vector<CCSTAssignExpr*> set_r0_args;
  std::vector<CCSTAssignExpr*> set_r1_args;

  sync_send_args.push_back(new CCSTPrimaryExprId("sync_ep"));

  std::vector<CCSTAssignExpr*> ilog2_args;

  ilog2_args.push_back(
    new CCSTShiftExpr(rightshift_t,
      new CCSTPrimaryExprId(v->identifier() + "_mem_sz"),
      new CCSTPrimaryExprId("PAGE_SHIFT")));

  set_r0_args.push_back(function_call("ilog2", ilog2_args));
  set_r1_args.push_back(new CCSTPrimaryExprId(v->identifier() + "_offset"));
  set_cr0_args.push_back(new CCSTPrimaryExprId(v->identifier() + "_cptr"));
  set_cr0_args2.push_back(new CCSTPrimaryExprId("CAP_CPTR_NULL"));

  statements.push_back(
    new CCSTExprStatement(
      new CCSTPostFixExprAssnExpr(new CCSTPrimaryExprId("lcd_set_r0"),
        set_r0_args)));

  statements.push_back(
    new CCSTExprStatement(
      new CCSTPostFixExprAssnExpr(new CCSTPrimaryExprId("lcd_set_r1"),
        set_r1_args)));

  statements.push_back(
    new CCSTExprStatement(
      new CCSTPostFixExprAssnExpr(new CCSTPrimaryExprId("lcd_set_cr0"),
        set_cr0_args)));

  statements.push_back(
    new CCSTExprStatement(
      new CCSTAssignExpr(new CCSTPrimaryExprId("sync_ret"), equals(),
        function_call("lcd_sync_send", sync_send_args))));

  statements.push_back(
    new CCSTExprStatement(
      new CCSTPostFixExprAssnExpr(new CCSTPrimaryExprId("lcd_set_cr0"),
        set_cr0_args2)));

  statements.push_back(
    if_cond_fail(new CCSTPrimaryExprId("sync_ret"), "failed to send"));

  return new CCSTCompoundStatement(declarations, statements);
}

/*
 * Want this code to be as dumb as possible.
 * The complexity for deciding what to marshal occurs previously.
 * when marshal params list is populated.
 * This function should never be called on a variable whose type is a projection
 */
CCSTStatement* marshal_variable(Variable *v, const std::string& direction, Channel::ChannelType type)
{
  std::vector<CCSTDeclaration*> declarations;
  std::vector<CCSTStatement*> statements;
  
  if (v->container() != 0x0) {
    // marshal container
    std::cout << "trying to marshal container " <<  v->container()->identifier() << std::endl;
    statements.push_back(marshal_variable(v->container(), direction, type));
  }

  if (v->type()->num() == FUNCTION_TYPE) {
    /// Nothing needs to be marshalled for a function pointer
    /// Function ptr's container will have to be marshalled and is done above
    return new CCSTCompoundStatement(declarations, statements);
  }

  if (v->type()->num() == PROJECTION_TYPE || v->type()->num() == PROJECTION_CONSTRUCTOR_TYPE) { // projection
    // loop through fields
    ProjectionType *pt = dynamic_cast<ProjectionType*>(v->type());
    Assert(pt != 0x0, "Error: dynamic cast to projection failed!\n");

    for (auto pf : *pt) {
      if ((direction == "in" && pf->in())
        || (direction == "out" && pf->out())) {

        /// if the specification is either bind or dealloc, the connection is
        /// already established. So, there is no need to pass my_ref. However,
        /// in the callee side, my_ref needs to be unmarshalled. So it would
        /// be marked as "in" anyways. To handle this, just skip if we meet
        /// the bind/dealloc specification and the proj_field is "my_ref"
        if (((v->bind_callee() && v->bind_callee()) || (v->dealloc_callee()))
          && pf->identifier() == "my_ref") {
          continue;
        }
        statements.push_back(marshal_variable(pf, direction, type));
      }
    }
  } else if (v->type()->num() == VOID_TYPE) {
    return marshal_void_pointer(v);
  } else {
    std::cout << "marshalling variable " <<  v->identifier() << std::endl;
    Assert(v->marshal_info() != 0x0, "Error: marshalling info is null\n");
    statements.push_back(marshal(access(v), v->marshal_info()->get_register(), type, "_request"));
  }
  return new CCSTCompoundStatement(declarations, statements);
}

std::vector<CCSTStatement*> marshal_variable_callee(Variable *v, Channel::ChannelType type)
{
  std::vector<CCSTStatement*> statements;

  if(v->container() != 0x0) {
    if(v->container()->out()) {
      std::vector<CCSTStatement*> tmp_stmts = marshal_variable_callee(v->container(), type);
      statements.insert(statements.end(), tmp_stmts.begin(), tmp_stmts.end());
    }
  }

  if (v->type()->num() == PROJECTION_TYPE || v->type()->num() == PROJECTION_CONSTRUCTOR_TYPE) { // projection
    ProjectionType *pt = dynamic_cast<ProjectionType*>(v->type());
    Assert(pt != 0x0, "Error: dynamic cast to projection type failed\n");

    for (auto pf : *pt) {
      if(pf->out()) {
	std::vector<CCSTStatement*> tmp_stmt = marshal_variable_callee(pf, type);
	statements.insert(statements.end(), tmp_stmt.begin(), tmp_stmt.end());
      }
    }
    
  } else {
    Assert(v->marshal_info() != 0x0, "Error: marshal info is null\n");
    statements.push_back(marshal(access(v), v->marshal_info()->get_register(), type, "_response"));
  }
  
  return statements;
}

std::vector<CCSTStatement*> marshal_variable_no_check(Variable *v, Channel::ChannelType type)
{
  std::vector<CCSTStatement*> statements;

  if(v->container() != 0x0) {
    std::vector<CCSTStatement*> tmp_stmts = marshal_variable_callee(v->container(), type);
    statements.insert(statements.end(), tmp_stmts.begin(), tmp_stmts.end());
  }

  if (v->type()->num() == PROJECTION_TYPE || v->type()->num() == PROJECTION_CONSTRUCTOR_TYPE) { // projection
    ProjectionType *pt = dynamic_cast<ProjectionType*>(v->type());
    Assert(pt != 0x0, "Error: dynamic cast to projection type failed\n");

    for (auto pf : *pt) {
      std::vector<CCSTStatement*> tmp_stmt = marshal_variable_callee(pf, type);
      statements.insert(statements.end(), tmp_stmt.begin(), tmp_stmt.end());
    }
    
  } else {
    Assert(v->marshal_info() != 0x0, "Error: marshal info is null\n");
    statements.push_back(marshal(access(v), v->marshal_info()->get_register(), type, "_response"));
  }
  
  return statements;
}

CCSTStatement* marshal(CCSTPostFixExpr *v, int reg, Channel::ChannelType type, const std::string &req_resp)
{
  std::string store_reg_func;
  std::vector<CCSTAssignExpr*> cptr_args;
  std::vector<CCSTAssignExpr*> store_reg_args;

  if (type == Channel::SyncChannel) {
    store_reg_func = store_register_mapping(reg);
  } else {
    store_reg_func = store_async_reg_mapping(reg);
    store_reg_args.push_back(new CCSTPrimaryExprId(req_resp));
    //cptr_args.push_back(v);
    //store_reg_args.push_back(function_call("cptr_val", cptr_args));
  }
  store_reg_args.push_back(v);
  
  return new CCSTExprStatement(function_call(store_reg_func, store_reg_args));
}
