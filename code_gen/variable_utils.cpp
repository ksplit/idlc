#include "ccst.h"
#include "code_gen.h"

/*
 * CAST code to declare a static variable
 */
CCSTDeclaration *declare_static_variable(Variable *gv) {
  std::vector<CCSTDecSpecifier *> specifier;
  specifier.push_back(new CCSTStoClassSpecifier(static_t));

  std::vector<CCSTSpecifierQual *> type_for_global = type(gv->type());
  specifier.insert(specifier.end(), type_for_global.begin(),
                   type_for_global.end());

  std::vector<CCSTInitDeclarator *> declarators;
  declarators.push_back(new CCSTDeclarator(
      pointer(gv->pointer_count()), new CCSTDirectDecId(gv->identifier())));

  return new CCSTDeclaration(specifier, declarators);
}

std::string * get_init_value(Variable * v){

  std::string * init_value;
  int num = v->type()->num();
  switch (num) {
  case TYPEDEF_TYPE: {
    break;
  }
  case INTEGER_TYPE: // int type case
  {
    IntegerType *it = dynamic_cast<IntegerType *>(v->type());
    Assert(it != 0x0, "Error: dynamic cast failed!\n");
    switch (it->int_type()) {
    case pt_char_t: {
	  init_value = new std::string("NULL");
      break;
    }
    case pt_short_t: {
	  init_value = new std::string("NULL");
      break;
    }
    case pt_int_t: {
	  init_value = new std::string("0");
      break;
    }
    case pt_long_t: {
	  init_value = new std::string("0");
      break;
    }
    case pt_longlong_t: {
	  init_value = new std::string("0");
      break;
    }
    case pt_capability_t: {
	  init_value = new std::string("0");
      break;
    }
    default: { Assert(1 == 0, "Error: unknown type\n"); }
    }
    return init_value;
  }
  case PROJECTION_TYPE: // struct
  {
	init_value = new std::string("NULL");
    return init_value;
  }
  case VOID_TYPE: {
	init_value = new std::string("NULL");
    return init_value;
  }
  case CHANNEL_TYPE: {
	init_value = new std::string("NULL");
    return init_value;
  }
  case FUNCTION_TYPE: {
	init_value = new std::string("NULL");
    break;
  }
  case UNRESOLVED_TYPE: {
	init_value = new std::string("NULL");
    Assert(1 == 0, "Error: unresolved type\n");
    break;
  }
  case PROJECTION_CONSTRUCTOR_TYPE: // struct
  {
	init_value = new std::string("NULL");
    return init_value;
  }
  case INITIALIZE_TYPE: {
    Assert(1 == 0, "Error: initialize type\n");
  }
  case BOOL_TYPE: {
	init_value = new std::string("false");
    return init_value;
  }
  case DOUBLE_TYPE: {
	init_value = new std::string("0");
    return init_value;
  }
  case FLOAT_TYPE: {
	init_value = new std::string("0");
    return init_value;
  }
  default: {
    Assert(1 == 0, "Error: Not a struct or integer type. \n");
  }
  }
  return init_value;
}


CCSTDeclaration *declare_variable(Variable *v) {
  std::vector<CCSTInitDeclarator *> decs;
  const std::string * init_value = get_init_value(v);
  decs.push_back(new CCSTInitDeclarator(new
		CCSTDeclarator(pointer(v->pointer_count()), new
		CCSTDirectDecId(v->identifier())), new CCSTInitializer(new
		CCSTPrimaryExprId(*init_value))));

  return new CCSTDeclaration(type2(v->type()), decs);
}

// recursively declare variables for callee glue code.
// declare container and the var, if the var has a container.
// otherwise declare the variable only and recurse.
std::vector<CCSTDeclaration *> declare_variables_callee(Variable *v) {
  std::vector<CCSTDeclaration *> declarations;

  if (v->container() != 0x0) {
    std::cout << "declaring container " << v->container()->identifier()
              << std::endl;
    declarations.push_back(declare_variable(v->container()));
    std::cout << "declaring variable " << v->identifier() << std::endl;
    declarations.push_back(declare_variable(v));
  } else {
    std::cout << "declaring variable " << v->identifier() << std::endl;
    declarations.push_back(declare_variable(v));
  }

  if (v->type()->num() == PROJECTION_TYPE ||
      v->type()->num() == PROJECTION_CONSTRUCTOR_TYPE) {
    ProjectionType *pt = dynamic_cast<ProjectionType *>(v->type());
    Assert(pt != 0x0, "Error: dynamic cast to projection type failed\n");

    for (auto pf : *pt) {
      std::vector<CCSTDeclaration *> tmp_declarations =
          declare_containers_only_callee(pf);
      declarations.insert(declarations.end(), tmp_declarations.begin(),
                          tmp_declarations.end());
    }
  }
  return declarations;
}

std::vector<CCSTDeclaration *> declare_containers_only_callee(Variable *v) {
  std::vector<CCSTDeclaration *> declarations;

  if (v->container() != 0x0) {
    declarations.push_back(declare_variable(v->container()));
  }

  if (v->type()->num() == PROJECTION_TYPE ||
      v->type()->num() == PROJECTION_CONSTRUCTOR_TYPE) {
    ProjectionType *pt = dynamic_cast<ProjectionType *>(v->type());
    Assert(pt != 0x0, "Error: dynamic cast to projection type failed\n");

    for (auto pf : *pt) {
      std::vector<CCSTDeclaration *> tmp_declarations =
          declare_containers_only_callee(pf);
      declarations.insert(declarations.end(), tmp_declarations.begin(),
                          tmp_declarations.end());
    }
  }
  return declarations;
}
