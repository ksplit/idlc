#include "ccst.h"
#include "code_gen.h"
#include "error.h"
#include "lcd_ast.h"
#include "lcd_idl.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fstream>

void print_usage() {
  std::cerr << "Usage:\n  ./compiler [-test] <idl file>" << std::endl;
  exit(0);
}

namespace v2 {
  CCSTFile* generate_common_header(Project* p)
  {
    std::vector<CCSTExDeclaration*> decls {};

    const auto idl_h = new Include(false, "ipc.h");
    decls.push_back(new CCSTPreprocessor(idl_h->get_path(), idl_h->is_relative()));

    decls.push_back(new CCSTDeclaration(
      {new CCSTStructUnionSpecifier(struct_t, "channel")},
      {new CCSTInitDeclarator(
        new CCSTDeclarator(
          nullptr,
          new CCSTDirectDecId(p->main_module()->id() + "_ch")
        )
      )}
    ));

    std::vector<CCSTEnumerator*> rpcs;
    for (const auto m : *p)
      for (const auto rpc : *m)
        rpcs.push_back(new CCSTEnumerator("KLCD_" + rpc->enum_name()));
    decls.push_back(
      new CCSTDeclaration(
        {new CCSTEnumSpecifier("dispatch_t", new CCSTEnumeratorList(new decltype(rpcs)(rpcs)))},
        {}
      )
    );

    return new CCSTFile(decls);
  }
  
  CCSTFile* generate_klcd_header(Project* p)
  {
    std::vector<CCSTExDeclaration*> decls;
    decls.push_back(new CCSTPreprocessor(p->main_module()->id() + "_common.h", true));

    const auto msg_struct_type = new CCSTStructUnionSpecifier(struct_t, "fipc_message");
    const auto msg_p = new CCSTParamDeclaration(
      {msg_struct_type},
      new CCSTDeclarator(new CCSTPointer(), new CCSTDirectDecId("msg"))
    );
    const auto dispatch_f = new CCSTDirectDecParamTypeList(
      new CCSTDirectDecId("dispatch_klcd"),
      new CCSTParamList({msg_p})
    );
    decls.push_back(new CCSTDeclaration(
      {new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::IntegerTypeSpec)},
      {new CCSTInitDeclarator(new CCSTDeclarator(nullptr, dispatch_f))}
    ));

    return new CCSTFile(decls);
  }

  auto get_type_spec(Type* t) {
    switch (t->num()) {
    case INTEGER_TYPE:
      switch (dynamic_cast<IntegerType*>(t)->int_type()) {
      case pt_int_t:
        return CCSTSimpleTypeSpecifier::IntegerTypeSpec;

      case pt_long_t:
        return CCSTSimpleTypeSpecifier::LongTypeSpec;

      case pt_short_t:
        return CCSTSimpleTypeSpecifier::ShortTypeSpec;

      case pt_char_t:
        return CCSTSimpleTypeSpecifier::CharTypeSpec;
      }

    case VOID_TYPE:
      return CCSTSimpleTypeSpecifier::VoidTypeSpec;

    default:
      return CCSTSimpleTypeSpecifier::OtherTypeSpec;
    }
  }

  void generate_callee_protos(Project* p, std::vector<CCSTExDeclaration*>& decls)
  {
    for (const auto m : *p) {
      for (const auto rpc : *m) {
        const auto proto_name = rpc->name();
        const auto type_spec = get_type_spec(rpc->return_variable()->type());

        std::vector<CCSTParamDeclaration*> params;
        for (const auto prm : *rpc) {
          const auto pts = get_type_spec(prm->type());
          params.push_back(
            new CCSTParamDeclaration(
              {new CCSTSimpleTypeSpecifier(pts)},
              new CCSTDeclarator(
                nullptr,
                new CCSTDirectDecId(prm->identifier())
              )
            )
          );
        }

        const auto f_proto = new CCSTDirectDecParamTypeList(
          new CCSTDirectDecId(proto_name),
          new CCSTParamList(params)
        );
        decls.push_back(new CCSTDeclaration(
          {new CCSTSimpleTypeSpecifier(type_spec)},
          {new CCSTInitDeclarator(new CCSTDeclarator(nullptr, f_proto))}
        ));
      }
    }
  }

  CCSTCompoundStatement* generate_rpc_marshal(Rpc* rpc)
  {
    const auto rpc_id = new CCSTPrimaryExprId(rpc->name());
    const auto rpc_call = new CCSTPostFixExprAssnExpr(rpc_id, {});
    auto rpcs = new CCSTExprStatement(rpc_call);

    const auto rt = rpc->return_variable()->type();
    if (rt->num() != VOID_TYPE) {
      const auto regs = new CCSTPostFixExprAccess(new CCSTPrimaryExprId("msg"), pointer_access_t, "reg");
      const auto rreg = new CCSTPostFixExprExpr(regs, new CCSTInteger(0));
      rpcs = new CCSTExprStatement(new CCSTAssignExpr(rreg, new CCSTAssignOp(equal_t), rpc_call));
    }

    return new CCSTCompoundStatement({}, {rpcs});
  }

  void generate_marshal_funcs(Project* p, std::vector<CCSTExDeclaration*>& decls)
  {
    for (const auto m : *p) {
      for (const auto rpc : *m) {
        const auto def_name = rpc->callee_name();
        std::vector<CCSTParamDeclaration*> params {
          new CCSTParamDeclaration(
            {new CCSTStructUnionSpecifier(struct_t, "fipc_message")},
            new CCSTDeclarator(
              new CCSTPointer(),
              new CCSTDirectDecId("msg")
            )
          )
        };
        
        const auto f_proto = new CCSTDirectDecParamTypeList(
          new CCSTDirectDecId(def_name),
          new CCSTParamList(params)
        );
        decls.push_back(new CCSTFuncDef(
          {new CCSTSimpleTypeSpecifier(CCSTSimpleTypeSpecifier::VoidTypeSpec)},
          new CCSTDeclarator(nullptr, f_proto),
          {},
          generate_rpc_marshal(rpc)
        ));
      }
    }
  }

  CCSTFile* generate_klcd_impl(Project* p)
  {
    std::vector<CCSTExDeclaration*> decls;

    decls.push_back(new CCSTPreprocessor(p->main_module()->id() + "_klcd.h", true));
    decls.push_back(new CCSTPreprocessor("stdio.h", false));
    decls.push_back(new CCSTPreprocessor("stdint.h", false));
    generate_callee_protos(p, decls);
    generate_marshal_funcs(p, decls);

    return new CCSTFile(decls);
  }
}

void do_code_generation(Project* tree, bool test_mode)
{
  const auto module = tree->main_module();
  const auto& id = module->id();
  std::ofstream klcd_h {id + "_klcd.h"};
  std::ofstream klcd_c {id + "_klcd.c"};
  std::ofstream lcd_h {id + "_lcd.h"};
  std::ofstream lcd_c {id + "_lcd.c"};
  std::ofstream common_h {id + "_common.h"};
  
  CCSTFile* file;

  file = v2::generate_common_header(tree);
  file->write(common_h, false);

  file = v2::generate_klcd_header(tree);
  file->write(klcd_h, false);

  file = v2::generate_klcd_impl(tree);
  file->write(klcd_c, false);
}

int main(int argc, char **argv) {
  if (argc != 2 && argc != 3) {
    print_usage();
  }

  char *file;
  bool test_mode;
  if (argc == 3) {
    file = argv[2];
    test_mode = true;
  }
  else {
    file = argv[1];
    test_mode = false;
  }

  try {
    // TODO: add support for multiple files, add option to specify
    // which module to compile, put each module in a different file
    Project *tree = (Project *)Parser::parse(std::string(file));

    ErrorReport *er = ErrorReport::instance();
    if (er->errors()) {
      std::cerr << "There were errors during parsing\n";
      // TODO: cleanup?
      exit(0);
    }

    std::vector<Include *> project_includes = tree->includes();
    std::vector<Include *> caller_includes = project_includes;
    std::vector<Include *> callee_includes = project_includes;

    tree->resolve_types();
    tree->create_trampoline_structs();
    tree->function_pointer_to_rpc();
    tree->generate_function_tags();
    tree->create_container_variables();
    tree->set_copy_container_accessors();
    tree->copy_types();
    tree->initialize_types();
    tree->set_accessors();
    tree->modify_specs();
    tree->prepare_marshal();

    // for (auto m : *tree) {
    //   std::string callee_h =
    //       new_name(m->identifier(), std::string("_callee.h"));
    //   std::string callee_c =
    //       new_name(m->identifier(), std::string("_callee.c"));
    //   std::string callee_disp =
    //       new_name(m->identifier(), std::string("_callee_dispatch.c"));
    //   std::string callee_lds =
    //       new_name(m->identifier(), std::string("_callee.lds.S"));

    //   std::string glue_helper_c =
    //       new_name(m->identifier(), std::string("_glue_helper.c"));
    //   std::string glue_helper_h =
    //       new_name(m->identifier(), std::string("_glue_helper.h"));

    //   std::string caller_h =
    //       new_name(m->identifier(), std::string("_caller.h"));
    //   std::string caller_c =
    //       new_name(m->identifier(), std::string("_caller.c"));
    //   std::string caller_disp =
    //       new_name(m->identifier(), std::string("_caller_dispatch.c"));

    //   std::ofstream ofs_callee_h(callee_h);
    //   std::ofstream ofs_callee_c(callee_c);
    //   std::ofstream ofs_callee_disp(callee_disp);
    //   std::ofstream ofs_callee_lds(callee_lds);

    //   std::ofstream ofs_glue_helper_c(glue_helper_c);
    //   std::ofstream ofs_glue_helper_h(glue_helper_h);

    //   std::ofstream ofs_caller_h(caller_h);
    //   std::ofstream ofs_caller_c(caller_c);
    //   std::ofstream ofs_caller_disp(caller_disp);

    //   callee_includes.push_back(
    //       new Include(true, "../" + m->identifier() + "_callee.h"));
    //   caller_includes.push_back(
    //       new Include(true, "../" + m->identifier() + "_caller.h"));

    //   CCSTFile *ccst_callee_h = generate_server_header(m);
    //   CCSTFile *ccst_callee_lds = generate_callee_lds(m);
    //   CCSTFile *ccst_callee_c = generate_server_source(m, callee_includes);
    //   CCSTFile *ccst_callee_disp = generate_dispatch<false>(m);

    //   CCSTFile *ccst_caller_h = generate_client_header(m);
    //   CCSTFile *ccst_caller_c = generate_client_source(m, caller_includes);
    //   CCSTFile *ccst_caller_disp = generate_dispatch<true>(m);

    //   CCSTFile *ccst_glue_c = generate_glue_source(m);
    //   CCSTFile *ccst_glue_h = generate_common_header(m);

    //   if (!ofs_callee_h.is_open() || !ofs_callee_c.is_open() ||
    //       !ofs_callee_disp.is_open() || !ofs_callee_lds.is_open() ||
    //       !ofs_glue_helper_c.is_open() || !ofs_caller_h.is_open() ||
    //       !ofs_caller_c.is_open() || !ofs_caller_disp.is_open() ||
    //       !ofs_glue_helper_h.is_open()) {

    //     std::cerr
    //         << "Unable to open one or more files for writing! Exiting ... "
    //         << std::endl;
    //     exit(-1);
    //   }

    //   /// Callee header guard macro
    //   std::string macro_callee_h("__" + m->identifier() + "_callee_h__");
    //   std_string_toupper(macro_callee_h);

    //   ofs_callee_h << "#pragma once" << std::endl;
    //   ccst_callee_h->write(ofs_callee_h, 0);

    //   ccst_callee_c->write(ofs_callee_c, 0);
    //   ccst_callee_disp->write(ofs_callee_disp, 0);
    //   ccst_glue_c->write(ofs_glue_helper_c, 0);

    //   /// Common header guard macro
    //   std::string macro_glhlpr_h("__" + m->identifier() + "_glue_helper_h__");
    //   std_string_toupper(macro_glhlpr_h);

    //   ofs_glue_helper_h << "#pragma once" << std::endl;
    //   ccst_glue_h->write(ofs_glue_helper_h, 0);

    //   /// FIXME: Should be generated like this. But how to generate SECTIONS {}
    //   /// statements.push_back(new CCSTPreprocessor("liblcd/trampoline.h",
    //   /// false));
    //   // ofs_callee_lds << "#include <liblcd/trampoline_link.h>\n\n";
    //   ofs_callee_lds << "SECTIONS\n{" << std::endl;
    //   ccst_callee_lds->write(ofs_callee_lds, 1);
    //   ofs_callee_lds << "}\nINSERT AFTER .text;\n";

    //   /// Caller header guard macro
    //   std::string macro_caller_h("__" + m->identifier() + "_caller_h__");
    //   std_string_toupper(macro_caller_h);

    //   ofs_caller_h << "#pragma once" << std::endl;
    //   ccst_caller_h->write(ofs_caller_h, 0);

    //   ccst_caller_c->write(ofs_caller_c, 0);
    //   ccst_caller_disp->write(ofs_caller_disp, 0);
    // }

    do_code_generation(tree, test_mode);

    return 0;
  } catch (const Parser::ParseException &e) {
    std::cerr << "\n\nALERT!!! - Caught parser exception" << std::endl;
    std::cerr << e.getReason() << std::endl;
    exit(0);
  }
}
