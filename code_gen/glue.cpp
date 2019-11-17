#include "ccst.h"
#include "code_gen.h"
#include <algorithm>
#include <list>
#include <string>

std::list<ProjectionType *> get_unique_projections(Interface *m) {
  auto all_types = m->module_scope()->all_type_definitions();
  std::list<ProjectionType *> uniq_projs;

  std::list<std::string> seen;

  /// Go through the type definitions to find out all the container
  /// data structures.
  for (auto it : all_types) {
    /// get the Type pointer
    auto *t = it.second;
    ProjectionType *proj_type;
    std::string rtype;

    /// scopes may possibly have duplicate projections definitions
    /// to print only one container struct for a projection, collect its
    /// real type
    if (t->num() == PROJECTION_TYPE ||
        t->num() == PROJECTION_CONSTRUCTOR_TYPE) {
      proj_type = dynamic_cast<ProjectionType *>(t);
      rtype = proj_type->real_type();
    }

    if (t->num() == PROJECTION_TYPE ||
        t->num() == PROJECTION_CONSTRUCTOR_TYPE || t->num() == FUNCTION_TYPE) {
      /// find its container
      if (all_types.find(container_name(t->name())) != all_types.end()) {
        auto *type = all_types[container_name(t->name())];
        auto *pt_container = dynamic_cast<ProjectionType *>(type);
        Assert(pt_container != NULL,
               "Error: dynamic cast to projection type failed\n");

        /// check if we have already generated container for
        /// the structure by comparing the real types
        if (std::find(seen.begin(), seen.end(), rtype) == seen.end()) {
          uniq_projs.push_back(pt_container);
          seen.push_back(rtype);
        }
      }
    }
  }
  return uniq_projs;
}

CCSTFile *generate_common_header(Interface *m) {
  std::vector<CCSTExDeclaration *> definitions;
  auto proj_list = get_unique_projections(m);

  /// generate container structures for unique projections in our module
  for (auto container : proj_list) {
    std::vector<CCSTDecSpecifier *> specifier;
    specifier.push_back(struct_declaration(container));
    std::vector<CCSTInitDeclarator *> empty;
    definitions.push_back(new CCSTDeclaration(specifier, empty));
  }

  /// print trampoline structs
  for (auto rpc : *m) {
    if (rpc->function_pointer_defined()) {
      int err;
      auto *t = rpc->current_scope()->lookup("trampoline_hidden_args", &err);
      Assert(t != NULL, "Error: failure looking up type\n");
      auto *pt = dynamic_cast<ProjectionType *>(t);
      Assert(pt != NULL, "Error: dynamic cast to projection type failed!\n");

      std::vector<CCSTDecSpecifier *> specifier;
      specifier.push_back(struct_declaration(pt));
      std::vector<CCSTInitDeclarator *> empty;
      definitions.push_back(new CCSTDeclaration(specifier, empty));

      /// XXX: struct trampoline_hidden_args need to be defined only once
      /// despite the number of function pointers.
      /// So, break after writing once.
      break;
    }
  }
  /// Don't really have to be separate. Just for some neatness in the
  /// generated code
  for (auto container : proj_list) {
    definitions.push_back(fn_decl_insert(container, m));
  }

  for (auto container : proj_list) {
    definitions.push_back(fn_decl_lookup(container, m));
  }

  return new CCSTFile(definitions);
}

std::vector<CCSTExDeclaration *> generate_enum_list(Interface *m) {
  std::vector<CCSTExDeclaration *> statements;
  std::vector<CCSTEnumerator *> *list = new std::vector<CCSTEnumerator *>();

  auto uniq_projs = get_unique_projections(m);

  for (auto type : uniq_projs) {
    std::string enum_name = type->name();
    std_string_toupper(enum_name);
    list->push_back(new CCSTEnumerator("GLUE_TYPE_" + enum_name));
  }
  list->push_back(new CCSTEnumerator("GLUE_NR_TYPES"));

  auto *enum_list = new CCSTEnumeratorList(list);
  auto *e = new CCSTEnumSpecifier("glue_type", enum_list);
  std::vector<CCSTDecSpecifier *> tmp;
  tmp.push_back(e);
  std::vector<CCSTInitDeclarator *> empty;
  statements.push_back(new CCSTDeclaration(tmp, empty));
  return statements;
}

CCSTFile *generate_glue_source(Interface *m) {
  std::vector<CCSTExDeclaration *> statements;
  auto gluetype_enum = generate_enum_list(m);
  auto uniq_projs = get_unique_projections(m);

  /// Insert glue type enum
  statements.insert(statements.end(), gluetype_enum.begin(),
                    gluetype_enum.end());

  for (auto proj_type : uniq_projs) {
    std::cout << "-----<<"
              << proj_type->name().substr(0,
                                          proj_type->name().find("_container"))
              << " RT: " << proj_type->real_type() << "\n";
    statements.push_back(function_definition(fn_decl_insert(proj_type, m),
                                             fn_def_insert(proj_type)));
    statements.push_back(function_definition(fn_decl_lookup(proj_type, m),
                                             fn_def_lookup(proj_type)));
  }
  return new CCSTFile(statements);
}

CCSTDeclaration *fn_decl_insert(ProjectionType *pt, Interface *m) {
  std::vector<CCSTInitDeclarator *> func;

  std::string fn_name = "glue_cap_insert_" +
                        pt->name().substr(0, pt->name().find("_container")) +
                        "_type";

  CCSTDirectDecId *name = new CCSTDirectDecId(fn_name);

  std::vector<CCSTParamDeclaration *> p_decs;
  int err;

  p_decs.push_back(new CCSTParamDeclaration(
      type2(m->module_scope()->lookup("glue_cspace", &err)),
      new CCSTDeclarator(new CCSTPointer(), new CCSTDirectDecId("cspace"))));

  p_decs.push_back(new CCSTParamDeclaration(
      type2(pt),
      new CCSTDeclarator(new CCSTPointer(), new CCSTDirectDecId(pt->name()))));

  p_decs.push_back(new CCSTParamDeclaration(
      type2(m->module_scope()->lookup("cptr", &err)),
      new CCSTDeclarator(new CCSTPointer(), new CCSTDirectDecId("c_out"))));

  CCSTParamList *param_list = new CCSTParamList(p_decs);

  CCSTDirectDecParamTypeList *params =
      new CCSTDirectDecParamTypeList(name, param_list);

  func.push_back(new CCSTDeclarator(NULL, params));

  return new CCSTDeclaration(int_type(), func);
}

CCSTDeclaration *fn_decl_lookup(ProjectionType *pt, Interface *m) {
  std::vector<CCSTInitDeclarator *> func;

  std::string fn_name = "glue_cap_lookup_" +
                        pt->name().substr(0, pt->name().find("_container")) +
                        "_type";

  CCSTDirectDecId *name = new CCSTDirectDecId(fn_name);

  std::vector<CCSTParamDeclaration *> p_decs;
  int err;

  p_decs.push_back(new CCSTParamDeclaration(
      type2(m->module_scope()->lookup("glue_cspace", &err)),
      new CCSTDeclarator(new CCSTPointer(), new CCSTDirectDecId("cspace"))));

  p_decs.push_back(new CCSTParamDeclaration(
      type2(m->module_scope()->lookup("cptr", &err)),
      new CCSTDeclarator(NULL, new CCSTDirectDecId("c"))));

  p_decs.push_back(new CCSTParamDeclaration(
      type2(pt), new CCSTDeclarator(new CCSTPointer(new CCSTPointer()),
                                    new CCSTDirectDecId(pt->name()))));

  CCSTParamList *param_list = new CCSTParamList(p_decs);

  CCSTDirectDecParamTypeList *params =
      new CCSTDirectDecParamTypeList(name, param_list);

  func.push_back(new CCSTDeclarator(NULL, params));

  return new CCSTDeclaration(int_type(), func);
}

CCSTCompoundStatement *fn_def_insert(ProjectionType *pt) {
  std::vector<CCSTDeclaration *> declarations;
  std::vector<CCSTStatement *> statements;
  std::vector<CCSTAssignExpr *> cspace_ins_args;
  std::string enum_name = pt->name();
  std_string_toupper(enum_name);

  cspace_ins_args.push_back(new CCSTPrimaryExprId("cspace"));
  cspace_ins_args.push_back(new CCSTPrimaryExprId(pt->name()));
  cspace_ins_args.push_back(new CCSTPostFixExprAccess(
      new CCSTPostFixExprExpr(new CCSTPrimaryExprId("glue_libcap_type_ops"),
                              new CCSTPrimaryExprId("GLUE_TYPE_" + enum_name)),
      object_access_t, "libcap_type"));
  cspace_ins_args.push_back(new CCSTPrimaryExprId("c_out"));
  statements.push_back(
      new CCSTReturn(function_call("glue_cspace_insert", cspace_ins_args)));
  return new CCSTCompoundStatement(declarations, statements);
}

CCSTCompoundStatement *fn_def_lookup(ProjectionType *pt) {
  std::vector<CCSTDeclaration *> declarations;
  std::vector<CCSTStatement *> statements;
  std::vector<CCSTAssignExpr *> cspace_lookup_args;
  std::string enum_name = pt->name();
  std_string_toupper(enum_name);
  std::vector<CCSTSpecifierQual *> spec_quals;

  spec_quals.push_back(
      new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::VoidTypeSpec));

  cspace_lookup_args.push_back(new CCSTPrimaryExprId("cspace"));
  cspace_lookup_args.push_back(new CCSTPrimaryExprId("c"));
  cspace_lookup_args.push_back(new CCSTPostFixExprAccess(
      new CCSTPostFixExprExpr(new CCSTPrimaryExprId("glue_libcap_type_ops"),
                              new CCSTPrimaryExprId("GLUE_TYPE_" + enum_name)),
      object_access_t, "libcap_type"));

  cspace_lookup_args.push_back(new CCSTCastExpr(
      new CCSTTypeName(spec_quals, new CCSTPointer(new CCSTPointer())),
      new CCSTPrimaryExprId(pt->name())));

  statements.push_back(
      new CCSTReturn(function_call("glue_cspace_lookup", cspace_lookup_args)));
  return new CCSTCompoundStatement(declarations, statements);
}
