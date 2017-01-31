#include "ccst.h"
#include "code_gen.h"
#include <list>
#include <string>
#include <algorithm>

std::list<ProjectionType*> get_unique_projections(Module *m)
{
  auto all_types = m->module_scope()->all_type_definitions();
  std::list<ProjectionType*> uniq_projs;

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
    if (t->num() == PROJECTION_TYPE
      || t->num() == PROJECTION_CONSTRUCTOR_TYPE) {
      proj_type = dynamic_cast<ProjectionType*>(t);
      rtype = proj_type->real_type();
    }

    if (t->num() == PROJECTION_TYPE || t->num() == PROJECTION_CONSTRUCTOR_TYPE
      || t->num() == FUNCTION_TYPE) {
      /// find its container
      if (all_types.find(container_name(t->name())) != all_types.end()) {
        auto *type = all_types[container_name(t->name())];
        auto *pt_container = dynamic_cast<ProjectionType*>(type);
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

CCSTFile *generate_common_header(Module *m)
{
  std::vector<CCSTExDeclaration*> definitions;
  auto proj_list = get_unique_projections(m);

  /// generate container structures for unique projections in our module
  for (auto container : proj_list) {
    std::vector<CCSTDecSpecifier*> specifier;
    specifier.push_back(struct_declaration(container));
    std::vector<CCSTInitDeclarator*> empty;
    definitions.push_back(new CCSTDeclaration(specifier, empty));
  }

  /// print trampoline structs
  for (auto rpc : *m) {
    if (rpc->function_pointer_defined()) {
      int err;
      auto *t = rpc->current_scope()->lookup("trampoline_hidden_args", &err);
      Assert(t != NULL, "Error: failure looking up type\n");
      auto *pt = dynamic_cast<ProjectionType*>(t);
      Assert(pt != NULL, "Error: dynamic cast to projection type failed!\n");

      std::vector<CCSTDecSpecifier*> specifier;
      specifier.push_back(struct_declaration(pt));
      std::vector<CCSTInitDeclarator*> empty;
      definitions.push_back(new CCSTDeclaration(specifier, empty));

      /// XXX: struct trampoline_hidden_args need to be defined only once
      /// despite the number of function pointers.
      /// So, break after writing once.
      break;
    }
  }
  return new CCSTFile(definitions);
}

std::vector<CCSTExDeclaration*> generate_enum_list(Module *m)
{
  std::vector<CCSTExDeclaration*> statements;
  std::vector<CCSTEnumerator*>* list = new std::vector<CCSTEnumerator*>();

  auto uniq_projs = get_unique_projections(m);

  for (auto type : uniq_projs) {
    std::string enum_name = type->name();
    std_string_toupper(enum_name);
    list->push_back(new CCSTEnumerator(enum_name));
  }

  auto *enum_list = new CCSTEnumeratorList(list);
  auto *e = new CCSTEnumSpecifier("glue_type", enum_list);
  std::vector<CCSTDecSpecifier*> tmp;
  tmp.push_back(e);
  std::vector<CCSTInitDeclarator*> empty;
  statements.push_back(new CCSTDeclaration(tmp, empty));
  return statements;
}

CCSTFile* generate_glue_source(Module *m)
{
  std::vector<CCSTExDeclaration*> statements;
  auto gluetype_enum = generate_enum_list(m);

  /// Insert glue type enum
  statements.insert(statements.end(), gluetype_enum.begin(),
    gluetype_enum.end());

  return new CCSTFile(statements);
}
