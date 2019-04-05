#include "ccst.h"
#include "code_gen.h"

const std::string init_value("0");
const std::string init_ptr_value("NULL");
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

const std::string get_init_value(Variable *v) {
  int num = v->type()->num();

  switch (num) {
  case INTEGER_TYPE: // int type case
  case DOUBLE_TYPE:
  case BOOL_TYPE:
  case FLOAT_TYPE:
    return init_value;
  case PROJECTION_TYPE:             // struct
  case PROJECTION_CONSTRUCTOR_TYPE: // struct
  case FUNCTION_TYPE:
  case VOID_TYPE:
  case UNRESOLVED_TYPE:
    return init_ptr_value;
  default:
    Assert(1 == 0, "Error: Cannot determine init value for %d\n", num);
  }
  return init_ptr_value;
}

CCSTDeclaration *declare_variable(Variable *v) {
  std::vector<CCSTInitDeclarator *> decs;
  const std::string init_value = get_init_value(v);

  decs.push_back(new CCSTInitDeclarator(
      new CCSTDeclarator(pointer(v->pointer_count()),
                         new CCSTDirectDecId(v->identifier())),
      new CCSTInitializer(new CCSTPrimaryExprId(init_value))));

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
