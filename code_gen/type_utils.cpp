#include "ccst.h"
#include "code_gen.h"

std::vector<CCSTSpecifierQual*> type(Type *t)
{
  std::vector<CCSTSpecifierQual*> specifier;

  int num = t->num();
  switch(num)
    {
    case TYPEDEF_TYPE:
      {
	specifier.push_back(new CCSTTypedefName(t->name()));
	break;
      }
    case INTEGER_TYPE: // int type case
      {
	IntegerType *it = dynamic_cast<IntegerType*>(t);
	Assert(it != 0x0, "Error: dynamic cast failed!\n");
	switch (it->int_type())
	  {
	  case pt_char_t:
	    {
	      specifier.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::CharTypeSpec));
	      break;
	    }
	  case pt_short_t:
	    {
	      specifier.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::ShortTypeSpec));
	      break;
	    }
	  case pt_int_t:
	    {
	      specifier.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::IntegerTypeSpec));
	      break;
	    }
	  case pt_long_t:
	    {
	      specifier.push_back( new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::LongTypeSpec));
	      break;
	    }
	  case pt_longlong_t:
	    {
	      specifier.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::LongTypeSpec));
	      specifier.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::LongTypeSpec));
	      break;
	    }
	  case pt_capability_t:
	    {
	      specifier.push_back(new CCSTTypedefName("capability_t"));
	      break;
	    }
	  default:
	    {
	      Assert(1 == 0, "Error: unknown type\n");
	    }
	  }
	return specifier;
      }
    case PROJECTION_TYPE: // struct
      {
	ProjectionType *pt = dynamic_cast<ProjectionType*>(t);
	Assert(pt != 0x0, "Error: dynamic cast failed!\n");
	specifier.push_back(new CCSTStructUnionSpecifier(struct_t, pt->real_type()));
	return specifier;
      }
    case VOID_TYPE:
      {
	// void type
	// todo
	specifier.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::VoidTypeSpec));
	return specifier;
      }
    case CHANNEL_TYPE:
      {
	// channel type
	// cptr_t define this somewhere for easy change
	specifier.push_back(new CCSTTypedefName("cptr_t"));
	return specifier;
      }
    case FUNCTION_TYPE:
      {
	// function pointer type
	// todo
	// where is support in grammar
	break;
      }
    case UNRESOLVED_TYPE:
      {
	Assert(1 == 0, "Error: unresolved type\n");
	break;
      }
    case PROJECTION_CONSTRUCTOR_TYPE: // struct
      {
	ProjectionType *pt = dynamic_cast<ProjectionType*>(t);
	Assert(pt != 0x0, "Error: dynamic cast failed!\n");
	specifier.push_back(new CCSTStructUnionSpecifier(struct_t, pt->real_type()));
	return specifier;
      }
    case INITIALIZE_TYPE:
      {
	Assert(1 == 0, "Error: initialize type\n");
      }
    case BOOL_TYPE:
      {
	specifier.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::BoolTypeSpec));
	return specifier;
      }
    case DOUBLE_TYPE:
      {
	specifier.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::DoubleTypeSpec));
	return specifier;
      }
    case FLOAT_TYPE:
      {
	specifier.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::FloatTypeSpec));
	return specifier;
      }
    default:
      {
	std::cout << "Received " << type_number_to_name(num) << " with name " <<  t->name() << std::endl;
	Assert(1 == 0, "Error: Not a struct or integer type. \n");
      }
    }
}


CCSTTypeName* type_cast(Type *t, int pointer_count)
{
  std::cout << "in type+cast\n";
  CCSTAbstDeclarator *pointers = new CCSTAbstDeclarator( pointer(pointer_count), 0x0);
  std::vector<CCSTSpecifierQual*> spec_quals;

  if(t == 0x0) {
    std::cout << "t is null\n";
  }

  switch(t->num())
    {
    case TYPEDEF_TYPE: // typedef
      {
	Typedef *td = dynamic_cast<Typedef*>(t);
	const char* name = td->alias();
	spec_quals.push_back( new CCSTTypedefName(name) );
	break;
      }
      case INTEGER_TYPE: // integer
      {
	IntegerType *it = dynamic_cast<IntegerType*>(t);
	spec_quals = integer_type_cast(it);
	break;
      }
    case PROJECTION_TYPE: // projection
      {
	std::cout << "here in type cast\n";
	ProjectionType *pt = dynamic_cast<ProjectionType*>(t);
	const char* name = pt->real_type();

	std::cout << "projection name: " <<  name << std::endl;
	spec_quals.push_back( new CCSTStructUnionSpecifier(struct_t, name) );
	break;
      }
    case VOID_TYPE: // void
      { // does this even happen?
	std::cout << "Warning: casting something as void\n";
	spec_quals.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::VoidTypeSpec) );
	break;
      }
    case BOOL_TYPE:
      {
	spec_quals.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::BoolTypeSpec));
	break;
      }
    case DOUBLE_TYPE:
      {
	spec_quals.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::DoubleTypeSpec));
	break;
      }
    case FLOAT_TYPE:
      {
	spec_quals.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::FloatTypeSpec));
	break;
      }
    default:
      {
	Assert(1 == 0, "Error: Should never get here\n");
      }
    }
  return new CCSTTypeName(spec_quals, pointers);
}


std::vector<CCSTSpecifierQual*> integer_type_cast(IntegerType *it)
{
  std::vector<CCSTSpecifierQual*> spec_quals;

  if(it->is_unsigned())
    {
      spec_quals.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::UnsignedTypeSpec));
    }
  switch (it->int_type())
    {
    case pt_char_t:
      {
	spec_quals.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::CharTypeSpec));
	break;
      }
    case pt_short_t:
      {
	spec_quals.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::ShortTypeSpec));
	break;
      }
    case pt_int_t:
      {
	spec_quals.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::IntegerTypeSpec));
	break;
      }
    case pt_long_t:
      {
	spec_quals.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::LongTypeSpec));
	break;
      }
    case pt_longlong_t:
      {
	spec_quals.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::LongTypeSpec));
	spec_quals.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::LongTypeSpec));
	break;
      }
    case pt_capability_t:
      {
	spec_quals.push_back(new CCSTTypedefName("capability_t"));
	break;
      }
    default:
      {
	std::cout << "todo\n";
      }
    }
  return spec_quals;
}

// constructs a type declaration from a name instead of a type object
std::vector<CCSTDecSpecifier*> struct_type(const char *type_name)
{
  std::vector<CCSTDecSpecifier*> specifier;
  specifier.push_back(new CCSTStructUnionSpecifier(struct_t, type_name));
  return specifier;
}

std::vector<CCSTDecSpecifier*> int_type()
{
  std::vector<CCSTDecSpecifier*>specifier;
  specifier.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::IntegerTypeSpec));
  
  return specifier;
}

std::vector<CCSTDecSpecifier*> type2(Type *t)
{
  std::vector<CCSTDecSpecifier*>specifier;
  int num = t->num();
  switch(num)  {
  case TYPEDEF_TYPE:
    {
      // typdef 
      // todo
    }
  case INTEGER_TYPE: // int type case
    {
      IntegerType *it = dynamic_cast<IntegerType*>(t);
      Assert(it != 0x0, "Error: dynamic cast failed!\n");
      switch (it->int_type())
	{
	case pt_char_t:
	  {
	    specifier.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::CharTypeSpec));
	    break;
	  }
	case pt_short_t:
	  {
	    specifier.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::ShortTypeSpec));
	    break;
	  }
	case pt_int_t:
	  {
	    specifier.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::IntegerTypeSpec));
	    break;
	  }
	case pt_long_t:
	  {
	    specifier.push_back( new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::LongTypeSpec));
	    break;
	  }
	case pt_longlong_t:
	  {
	    specifier.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::LongTypeSpec));
	    specifier.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::LongTypeSpec));
	    break;
	  }
	case pt_capability_t:
	  {
	    specifier.push_back(new CCSTTypedefName("capability_t"));
	    break;
	  }
	default:
	  {
	    Assert(1 == 0, "Error: unknown type\n");
	  }
	}
      return specifier;
    }
  case PROJECTION_TYPE: // struct
    {
      ProjectionType *pt = dynamic_cast<ProjectionType*>(t);
      Assert(pt != 0x0, "Error: dynamic cast failed!\n");
      specifier.push_back(new CCSTStructUnionSpecifier(struct_t, pt->real_type()));
      return specifier;
    }
  case VOID_TYPE:
    {
      specifier.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::VoidTypeSpec));
      return specifier;
    }
  case CHANNEL_TYPE:
    {
      specifier.push_back(new CCSTTypedefName("cptr_t"));
      return specifier;
    }
  case FUNCTION_TYPE:
    {
      // function pointer type
      // todo
      break;
    }
  case BOOL_TYPE:
    {
      specifier.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::BoolTypeSpec));
      return specifier;
    }
  case DOUBLE_TYPE:
    {
      specifier.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::DoubleTypeSpec));
      return specifier;
    }
  case FLOAT_TYPE:
    {
      specifier.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::FloatTypeSpec));
      return specifier;
    }
  default:
    {
      std::cout << "Received " <<  type_number_to_name(num) << " instead of struct or integer" << std::endl;
      Assert(1 == 0, "Error: Not a struct or integer type.\n");
    }
  }
}

/* 
 * given a projection, returns a struct declaration for the projection name 
 * including the projection fields.
 */
CCSTStructUnionSpecifier* struct_declaration(ProjectionType *pt)
{
  std::vector<CCSTStructDeclaration*> field_decs;
  std::vector<CCSTInitDeclarator*> decs;
  std::vector<CCSTSpecifierQual*>specifier;
  std::vector<Parameter*> parameters;
  std::vector<CCSTStructDeclarator*> struct_decs;
  std::vector<ProjectionField*> fields = pt->fields();
  std::vector<CCSTParamDeclaration*> func_pointer_params;

  for (auto pf : *pt) {
    if (pf->type()->num() == FUNCTION_TYPE) {
      Function *f = dynamic_cast<Function*>(pf->type());
      std::vector<CCSTDecSpecifier*> new_fp_return_type = type2(f->return_var_->type());
      specifier.push_back(new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::IntegerTypeSpec)); //type2(f->return_var_->type());
      //specifier = new_fp_return_type;
      parameters = f->parameters_;

      for (auto p : *f) {
        std::vector<CCSTDecSpecifier*> fp_param_tmp = type2(p->type());
        func_pointer_params.push_back(new CCSTParamDeclaration(fp_param_tmp
                     , new CCSTDeclarator(pointer(p->pointer_count()), new CCSTDirectDecId(""))));
      }
      struct_decs.push_back(new CCSTStructDeclarator(
            new CCSTDeclarator(NULL,
            new CCSTDirectDecParamTypeList(
            new CCSTDirectDecDec(
            new CCSTDeclarator(
            new CCSTPointer(),
            new CCSTDirectDecId(f->name()))),
            new CCSTParamList(func_pointer_params)))));

      field_decs.push_back( new CCSTStructDeclaration(specifier, new CCSTStructDecList(struct_decs)));
    } else {
      field_decs.push_back(new CCSTStructDeclaration( type(pf->type())
                  , new CCSTStructDeclarator(
                      new CCSTDeclarator(
                          pointer(pf->pointer_count()),
                          new CCSTDirectDecId(pf->identifier())))));
    }
  }
  
  return new CCSTStructUnionSpecifier(struct_t, pt->name(), field_decs);
}

/*
 * looks up structure and returns a declaration for a variable 
 * with name var_name that is a pointer to struct with name struct_name
 */
CCSTDeclaration* struct_pointer_declaration(const char* struct_name, const char* var_name, LexicalScope *ls)
{
  int err;
  Type *struct_tmp = ls->lookup(struct_name, &err); // fix
  std::cout << "Looking up " <<  struct_name << " in environment" << std::endl;
  Assert(struct_tmp != 0x0, "Error: could not find container in environment\n");
  ProjectionType *struct_ = dynamic_cast<ProjectionType*>(struct_tmp);
  Assert(struct_ != 0x0, "Error: dynamic cast to Projection type failed!\n");
  
  std::vector<CCSTInitDeclarator*> decs;
  decs.push_back(new CCSTDeclarator(new CCSTPointer(), new CCSTDirectDecId(var_name)));
  return new CCSTDeclaration(type2(struct_), decs);
}

CCSTStatement* kzalloc_structure(const char* struct_name, const char* var_name)
{
  // alloc
  std::vector<CCSTAssignExpr*> kzalloc_args;
  kzalloc_args.push_back(new CCSTUnaryExprSizeOf(new CCSTUnaryExprCastExpr(new CCSTUnaryOp(unary_mult_t)
									   , new CCSTPrimaryExprId(struct_name))));
  kzalloc_args.push_back(new CCSTEnumConst("GFP_KERNEL"));
  return new CCSTExprStatement( new CCSTAssignExpr(new CCSTPrimaryExprId(var_name), equals(), function_call("kzalloc", kzalloc_args)));
}
