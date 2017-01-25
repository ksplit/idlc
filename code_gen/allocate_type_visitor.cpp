#include "lcd_ast.h"
#include "code_gen.h"

AllocateTypeVisitor::AllocateTypeVisitor()
{
}

CCSTStatement* AllocateTypeVisitor::visit(FloatType *ft, Variable *v)
{
  std::vector<CCSTDeclaration*> declarations;
  std::vector<CCSTStatement*> statements;

  std::vector<CCSTSpecifierQual*> void_star_quals;
  void_star_quals.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::VoidTypeSpec));

  CCSTTypeName *void_star = new CCSTTypeName(void_star_quals, new CCSTPointer());

  std::vector<CCSTSpecifierQual*> float_type_quals = type(ft);
  CCSTTypeName *float_type = new CCSTTypeName(float_type_quals, 0x0);
  
  int p_count_save = v->pointer_count(); // total pointer count
  int p_count = v->pointer_count();

  while(p_count > 1) {
    std::vector<CCSTAssignExpr*> kzalloc_args;
    kzalloc_args.push_back(new CCSTUnaryExprSizeOf(void_star));
    kzalloc_args.push_back(new CCSTEnumConst("GFP_KERNEL"));
    
    v->set_pointer_count(p_count_save-p_count+1); // 0 then 1. when its 1 it isnt derefing
    statements.push_back(new CCSTExprStatement( new CCSTAssignExpr(access(v), equals(), function_call("kzalloc", kzalloc_args))));

    /* do error checking */
    statements.push_back(if_cond_fail(new CCSTUnaryExprCastExpr(Not(), access(v))
				      , "kzalloc"));
    
    p_count -= 1;
  }
  v->set_pointer_count(p_count_save - p_count +1); // p count will either be 1 or will be p coutn save
  std::vector<CCSTAssignExpr*> kzalloc_args;

  /* do sizeof(*access(v))  */
  kzalloc_args.push_back(new CCSTUnaryExprSizeOf(float_type));
  kzalloc_args.push_back(new CCSTEnumConst("GFP_KERNEL"));

  statements.push_back(new CCSTExprStatement( new CCSTAssignExpr(access(v), equals(), function_call("kzalloc", kzalloc_args))));

  /* do error checking */
  statements.push_back(if_cond_fail(new CCSTUnaryExprCastExpr(Not(), access(v))
				    , "kzalloc"));


  v->set_pointer_count(p_count_save); // set back to what it was before
  return new CCSTCompoundStatement(declarations, statements);
}

CCSTStatement* AllocateTypeVisitor::visit(DoubleType *dt, Variable *v)
{
  std::vector<CCSTDeclaration*> declarations;
  std::vector<CCSTStatement*> statements;

  std::vector<CCSTSpecifierQual*> void_star_quals;
  void_star_quals.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::VoidTypeSpec));

  CCSTTypeName *void_star = new CCSTTypeName(void_star_quals, new CCSTPointer());

  std::vector<CCSTSpecifierQual*> double_type_quals = type(dt);
  CCSTTypeName *double_type = new CCSTTypeName(double_type_quals, 0x0);
  
  int p_count_save = v->pointer_count(); // total pointer count
  int p_count = v->pointer_count();

  while(p_count > 1) {
    std::vector<CCSTAssignExpr*> kzalloc_args;
    kzalloc_args.push_back(new CCSTUnaryExprSizeOf(void_star));
    kzalloc_args.push_back(new CCSTEnumConst("GFP_KERNEL"));
    
    v->set_pointer_count(p_count_save-p_count+1); // 0 then 1. when its 1 it isnt derefing
    statements.push_back(new CCSTExprStatement( new CCSTAssignExpr(access(v), equals(), function_call("kzalloc", kzalloc_args))));

    /* do error checking */
    statements.push_back(if_cond_fail(new CCSTUnaryExprCastExpr(Not(), access(v))
				      , "kzalloc"));
    
    p_count -= 1;
  }
  v->set_pointer_count(p_count_save - p_count +1); // p count will either be 1 or will be p coutn save
  std::vector<CCSTAssignExpr*> kzalloc_args;

  /* do sizeof(*access(v))  */
  kzalloc_args.push_back(new CCSTUnaryExprSizeOf(double_type));
  kzalloc_args.push_back(new CCSTEnumConst("GFP_KERNEL"));

  statements.push_back(new CCSTExprStatement( new CCSTAssignExpr(access(v), equals(), function_call("kzalloc", kzalloc_args))));

  /* do error checking */
  statements.push_back(if_cond_fail(new CCSTUnaryExprCastExpr(Not(), access(v))
				    , "kzalloc"));


  v->set_pointer_count(p_count_save); // set back to what it was before
  return new CCSTCompoundStatement(declarations, statements);
}

CCSTStatement* AllocateTypeVisitor::visit(BoolType *bt, Variable *v)
{
  std::vector<CCSTDeclaration*> declarations;
  std::vector<CCSTStatement*> statements;

  std::vector<CCSTSpecifierQual*> void_star_quals;
  void_star_quals.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::VoidTypeSpec));

  CCSTTypeName *void_star = new CCSTTypeName(void_star_quals, new CCSTPointer());

  std::vector<CCSTSpecifierQual*> bool_type_quals = type(bt);
  CCSTTypeName *bool_type = new CCSTTypeName(bool_type_quals, 0x0);
  
  int p_count_save = v->pointer_count(); // total pointer count
  int p_count = v->pointer_count();

  while(p_count > 1) {
    std::vector<CCSTAssignExpr*> kzalloc_args;
    kzalloc_args.push_back(new CCSTUnaryExprSizeOf(void_star));
    kzalloc_args.push_back(new CCSTEnumConst("GFP_KERNEL"));
    
    v->set_pointer_count(p_count_save-p_count+1); // 0 then 1. when its 1 it isnt derefing
    statements.push_back(new CCSTExprStatement( new CCSTAssignExpr(access(v), equals(), function_call("kzalloc", kzalloc_args))));

    /* do error checking */
    statements.push_back(if_cond_fail(new CCSTUnaryExprCastExpr(Not(), access(v))
				      , "kzalloc"));
    
    p_count -= 1;
  }
  v->set_pointer_count(p_count_save - p_count +1); // p count will either be 1 or will be p coutn save
  std::vector<CCSTAssignExpr*> kzalloc_args;

  /* do sizeof(*access(v))  */
  kzalloc_args.push_back(new CCSTUnaryExprSizeOf(bool_type));
  kzalloc_args.push_back(new CCSTEnumConst("GFP_KERNEL"));

  statements.push_back(new CCSTExprStatement( new CCSTAssignExpr(access(v), equals(), function_call("kzalloc", kzalloc_args))));

  /* do error checking */
  statements.push_back(if_cond_fail(new CCSTUnaryExprCastExpr(Not(), access(v))
				    , "kzalloc"));


  v->set_pointer_count(p_count_save); // set back to what it was before
  return new CCSTCompoundStatement(declarations, statements);
}

CCSTStatement* AllocateTypeVisitor::visit(Channel *c, Variable *v)
{
  Assert(1 == 0, "Error: allocation for channel not implemented\n");
  return NULL;
}

CCSTStatement* AllocateTypeVisitor::visit(UnresolvedType *ut, Variable *v)
{
  Assert(1 == 0, "Error: cannot allocate an unresolved type\n");
  return NULL;
}

CCSTStatement* AllocateTypeVisitor::visit(Function *fp, Variable *v)
{
  // Assert(1 == 0, "Error: allocation for function pointer not implemented\n");
  std::vector<CCSTDeclaration*> declarations;
  std::vector<CCSTStatement*> statements;

  return new CCSTCompoundStatement(declarations, statements);
}

CCSTStatement* AllocateTypeVisitor::visit(Typedef *td, Variable *v)
{
  return td->type()->accept(this, v);
}

/// Void pointer marshalling for callee.
/// Declares cptr, and other variables for receiving a void pointer
/// TODO: See if we can somehow squash this and the marshalling code for
/// caller
CCSTStatement* AllocateTypeVisitor::visit(VoidType *vt, Variable *v)
{
  std::vector<CCSTDeclaration*> declarations;
  std::vector<CCSTStatement*> statements;

  std::vector<CCSTDecSpecifier*> spec_ull;
  std::vector<CCSTDecSpecifier*> spec_int;
  std::vector<CCSTDecSpecifier*> spec_cptr;
  std::vector<CCSTDecSpecifier*> spec_gva;

  std::vector<CCSTInitDeclarator*> decs_order;
  std::vector<CCSTInitDeclarator*> decs_off;
  std::vector<CCSTInitDeclarator*> decs_cptr;
  std::vector<CCSTInitDeclarator*> decs_gva;
  std::vector<CCSTInitDeclarator*> decs_sret;

  spec_cptr.push_back(new CCSTTypedefName("cptr_t"));
  spec_gva.push_back(new CCSTTypedefName("gva_t"));

  spec_ull.push_back(
    new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::UnsignedTypeSpec));
  spec_ull.push_back(
    new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::LongTypeSpec));
  spec_int.push_back(
    new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::IntegerTypeSpec));

  decs_sret.push_back(
    new CCSTInitDeclarator(
      new CCSTDeclarator(NULL, new CCSTDirectDecId("sync_ret"))));
  decs_order.push_back(
    new CCSTInitDeclarator(
      new CCSTDeclarator(NULL, new CCSTDirectDecId("mem_order"))));
  decs_off.push_back(
    new CCSTInitDeclarator(
      new CCSTDeclarator(NULL,
        new CCSTDirectDecId(v->identifier() + "_offset"))));
  decs_cptr.push_back(
    new CCSTInitDeclarator(
      new CCSTDeclarator(NULL,
        new CCSTDirectDecId(v->identifier() + "_cptr"))));
  decs_gva.push_back(
    new CCSTInitDeclarator(
      new CCSTDeclarator(NULL,
        new CCSTDirectDecId(v->identifier() + "_gva"))));

  declarations.push_back(new CCSTDeclaration(spec_int, decs_sret));
  declarations.push_back(new CCSTDeclaration(spec_ull, decs_order));
  declarations.push_back(new CCSTDeclaration(spec_ull, decs_off));
  declarations.push_back(new CCSTDeclaration(spec_cptr, decs_cptr));
  declarations.push_back(new CCSTDeclaration(spec_gva, decs_gva));

  return new CCSTCompoundStatement(declarations, statements);
}

/* Assumes something with the name v->identifier(), has already been declared */
CCSTStatement* AllocateTypeVisitor::visit(IntegerType *it, Variable *v)
{
  std::vector<CCSTDeclaration*> declarations;
  std::vector<CCSTStatement*> statements;

  std::vector<CCSTSpecifierQual*> int_ptr_quals = type(it);
  CCSTTypeName *int_ptr = new CCSTTypeName(int_ptr_quals, new CCSTPointer());

  std::vector<CCSTSpecifierQual*> int_type_quals = type(it);
  CCSTTypeName *int_type = new CCSTTypeName(int_type_quals, NULL);

  int p_count_save = v->pointer_count();
  int p_count = v->pointer_count();

  while (p_count > 1) {
    std::vector<CCSTAssignExpr*> kzalloc_args;
    kzalloc_args.push_back(new CCSTUnaryExprSizeOf(int_ptr));
    kzalloc_args.push_back(new CCSTEnumConst("GFP_KERNEL"));

    v->set_pointer_count(p_count_save - p_count + 1);
    statements.push_back(
      new CCSTExprStatement(
        new CCSTAssignExpr(access(v), equals(),
          function_call("kzalloc", kzalloc_args))));

    /// do error checking
    statements.push_back(
      if_cond_fail(new CCSTUnaryExprCastExpr(Not(), access(v)), "kzalloc"));

    p_count -= 1;
  }
  /// p count will either be 1 or will be p_count_save
  v->set_pointer_count(p_count_save - p_count + 1);
  std::vector<CCSTAssignExpr*> kzalloc_args;

  /// do sizeof(*access(v))
  kzalloc_args.push_back(new CCSTUnaryExprSizeOf(int_type));
  kzalloc_args.push_back(new CCSTEnumConst("GFP_KERNEL"));

  statements.push_back(
    new CCSTExprStatement(
      new CCSTAssignExpr(access(v), equals(),
        function_call("kzalloc", kzalloc_args))));

  /// do error checking
  statements.push_back(
    if_cond_fail(new CCSTUnaryExprCastExpr(Not(), access(v)), "kzalloc"));

  /// set back to what it was before
  v->set_pointer_count(p_count_save);
  return new CCSTCompoundStatement(declarations, statements);
}

/* Assume variable at name v->identifier() has already been declared */
CCSTStatement* AllocateTypeVisitor::visit(ProjectionType *pt, Variable *v)
{
  std::vector<CCSTDeclaration*> declarations;
  std::vector<CCSTStatement*> statements;

  std::vector<CCSTSpecifierQual*> void_star_quals;
  void_star_quals.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::VoidTypeSpec));
  CCSTTypeName *void_star = new CCSTTypeName(void_star_quals, new CCSTPointer());

  int p_count_save = v->pointer_count();
  int p_count = v->pointer_count();

  while(p_count > 1) {
    std::vector<CCSTAssignExpr*> kzalloc_args;
    kzalloc_args.push_back(new CCSTUnaryExprSizeOf(void_star));
    kzalloc_args.push_back(new CCSTEnumConst("GFP_KERNEL"));
    
    v->set_pointer_count(p_count_save-p_count+1);
    statements.push_back(new CCSTExprStatement( new CCSTAssignExpr(access(v), equals(), function_call("kzalloc", kzalloc_args))));
    /* do error checking */
    std::string *goto_alloc = new std::string("fail_alloc" + std::to_string(p_count));
    statements.push_back(if_cond_fail_goto(new CCSTUnaryExprCastExpr(Not(), access(v))
				    , "kzalloc", *goto_alloc));

    p_count -= 1; 
  }
  
  v->set_pointer_count(p_count_save); // access object to take size
  /* allocate the actual structure now */
  std::vector<CCSTAssignExpr*> kzalloc_args;

  /* do sizeof(struct thing) */
  kzalloc_args.push_back(dereference(new CCSTUnaryExprSizeOf(access(v))));
  kzalloc_args.push_back(new CCSTEnumConst("GFP_KERNEL"));

  v->set_pointer_count(p_count_save-p_count+1);

  std::string *goto_alloc = new std::string("fail_alloc");
  statements.push_back(new CCSTExprStatement( new CCSTAssignExpr(access(v), equals(), function_call("kzalloc", kzalloc_args))));

  /* do error checking */
  statements.push_back(if_cond_fail_goto(new CCSTUnaryExprCastExpr(Not(), access(v))
				    , "kzalloc", *goto_alloc));

  v->set_pointer_count(p_count_save);

  /* Now need to allocate fields */
  for (auto pf : *pt) {
    if (pf->pointer_count() > 0) {
      statements.push_back(pf->type()->accept(this, pf));
    }
  }

  return new CCSTCompoundStatement(declarations, statements);
}

CCSTStatement* AllocateTypeVisitor::visit(ProjectionConstructorType *pt, Variable *v)
{
  std::vector<CCSTDeclaration*> declarations;
  std::vector<CCSTStatement*> statements;

  std::vector<CCSTSpecifierQual*> void_star_quals;
  void_star_quals.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::VoidTypeSpec));
  CCSTTypeName *void_star = new CCSTTypeName(void_star_quals, new CCSTPointer());

  int p_count_save = v->pointer_count();
  int p_count = v->pointer_count();

  while(p_count > 1) {
    std::vector<CCSTAssignExpr*> kzalloc_args;
    kzalloc_args.push_back(new CCSTUnaryExprSizeOf(void_star));
    kzalloc_args.push_back(new CCSTEnumConst("GFP_KERNEL"));
    
    v->set_pointer_count(p_count_save-p_count+1);
    statements.push_back(new CCSTExprStatement( new CCSTAssignExpr(access(v), equals(), function_call("kzalloc", kzalloc_args))));
    /* do error checking */
    statements.push_back(if_cond_fail(new CCSTUnaryExprCastExpr(Not(), access(v))
				    , "kzalloc"));

    p_count -= 1; 
  }
  
  v->set_pointer_count(p_count_save);
  /* allocate the actual structure now */
  std::vector<CCSTAssignExpr*> kzalloc_args;

  /* do sizeof(struct thing) */
  kzalloc_args.push_back(dereference(new CCSTUnaryExprSizeOf(access(v))));
  kzalloc_args.push_back(new CCSTEnumConst("GFP_KERNEL"));

  v->set_pointer_count(p_count_save-p_count+1);
  statements.push_back(new CCSTExprStatement( new CCSTAssignExpr(access(v), equals(), function_call("kzalloc", kzalloc_args))));

  /* do error checking */
  statements.push_back(if_cond_fail(new CCSTUnaryExprCastExpr(Not(), access(v))
				    , "kzalloc"));
  
  
  v->set_pointer_count(p_count_save);

  /* Now need to allocate fields */
  for (auto pf : *pt) {
    if (pf->pointer_count() > 0) {
      statements.push_back(pf->type()->accept(this, pf));
    }
  }

  return new CCSTCompoundStatement(declarations, statements);
}

CCSTStatement* AllocateTypeVisitor::visit(InitializeType *it, Variable *v)
{
  Assert( 1 == 0, "Error: cannot call allocate type on initialize type\n");
  return NULL;
}
