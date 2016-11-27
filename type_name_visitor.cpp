#include "lcd_ast.h"

CCSTTypeName* TypeNameVisitor::visit(FloatType *ft)
{
  CCSTPointer *pt = 0x0;
  std::vector<CCSTSpecifierQual*> sq;
  sq.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::FloatTypeSpec));

  return new CCSTTypeName(sq, pt);
}

CCSTTypeName* TypeNameVisitor::visit(DoubleType *dt)
{
  CCSTPointer *pt = 0x0;
  std::vector<CCSTSpecifierQual*> sq;
  sq.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::DoubleTypeSpec));

  return new CCSTTypeName(sq, pt);
}

CCSTTypeName* TypeNameVisitor::visit(BoolType *bt)
{
  CCSTPointer *pt = 0x0;
  std::vector<CCSTSpecifierQual*> sq;
  sq.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::BoolTypeSpec));

  return new CCSTTypeName(sq, pt);
}

CCSTTypeName* TypeNameVisitor::visit(UnresolvedType *ut)
{
  Assert(1 == 0, "Error: no type name for unresolved type.\n");
  return NULL;
}

CCSTTypeName* TypeNameVisitor::visit(Typedef *td)
{
  return td->type()->accept(this);
}

CCSTTypeName* TypeNameVisitor::visit(Channel *c)
{
  std::cout << " type name visitor channel todo!\n";
  return 0x0;
}

CCSTTypeName* TypeNameVisitor::visit(VoidType *vt)
{
  CCSTPointer *pt = 0x0;
  std::vector<CCSTSpecifierQual*> sq;
  
  sq.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::VoidTypeSpec));
  
  return new CCSTTypeName(sq, pt);
}

CCSTTypeName* TypeNameVisitor::visit(IntegerType *it)
{
  CCSTPointer *pt = 0x0;
  std::vector<CCSTSpecifierQual*> sq;
  
  switch(it->int_type()){
  case pt_char_t: {
    sq.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::CharTypeSpec));
    break;
  }
  case pt_short_t: {
    sq.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::ShortTypeSpec));
    break;
  }
  case pt_int_t: {
    sq.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::IntegerTypeSpec));
    break;
  }
  case pt_long_t: {
    sq.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::LongTypeSpec));
    break;
  }
  case pt_longlong_t: {
    sq.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::LongTypeSpec));
    sq.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::LongTypeSpec));
    break;
  }
  case pt_capability_t: {
    sq.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::LongTypeSpec));
    break;
  }
  default: {
    Assert(1 == 0, "Error: unknown integer type.\n");
  } 
  }
  return new CCSTTypeName(sq, pt);
}

CCSTTypeName* TypeNameVisitor::visit(ProjectionType *proj)
{
  // make sure this is correct
  CCSTPointer *pt = 0x0;
  std::vector<CCSTSpecifierQual*> sq;
  sq.push_back(new CCSTStructUnionSpecifier(struct_t, proj->real_type()));
  
  return new CCSTTypeName(sq, pt);
}

CCSTTypeName* TypeNameVisitor::visit(Function *fp)
{
  std::cout << "type name visitor function pointer todo\n";
  return 0x0;
}

CCSTTypeName* TypeNameVisitor::visit(ProjectionConstructorType *pct)
{
  // make sure this is correct
  CCSTPointer *pt = 0x0;
  std::vector<CCSTSpecifierQual*> sq;
  sq.push_back(new CCSTStructUnionSpecifier(struct_t, pct->real_type()));
  
  return new CCSTTypeName(sq, pt);
}

CCSTTypeName* TypeNameVisitor::visit(InitializeType *it)
{
  Assert(1 == 0, "Error: no type name for initialize type\n");
  return NULL;
}

