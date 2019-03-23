// ========================================================================= //
// IDL Compiler Infrastructure for LCDs					     //
// ========================================================================= //

// main.cpp:
// ========
// This is the main file of the IDL compiler, which calls the passes on the
// given input IDL file to generate secure glue code.

#include "Passes/astprint.h"
#include "Passes/require.h"
#include "ccst.h"
#include "code_gen.h"
#include "error.h"
#include "lcd_ast.h"
#include "lcd_idl.h"
#include <fstream>
#include <iostream>
#include <regex>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bits/stdc++.h> 

void print_usage() {
  std::cerr << "Usage:\n  ./compiler <idl file>" << std::endl;
  exit(0);
}

void generate_module_code(Module *m) {
  Project *tree = m->parent_pjt_;
  std::vector<Include *> project_includes = tree->includes();
  std::vector<Include *> caller_includes = project_includes;
  std::vector<Include *> callee_includes = project_includes;
  std::string callee_h = new_name(m->identifier(), std::string("_callee.h"));
  std::string callee_c = new_name(m->identifier(), std::string("_callee.c"));
  std::string callee_disp =
      new_name(m->identifier(), std::string("_callee_dispatch.c"));
  std::string callee_lds =
      new_name(m->identifier(), std::string("_callee.lds.S"));

  std::string glue_helper_c =
      new_name(m->identifier(), std::string("_glue_helper.c"));
  std::string glue_helper_h =
      new_name(m->identifier(), std::string("_glue_helper.h"));

  std::string caller_h = new_name(m->identifier(), std::string("_caller.h"));
  std::string caller_c = new_name(m->identifier(), std::string("_caller.c"));
  std::string caller_disp =
      new_name(m->identifier(), std::string("_caller_dispatch.c"));

  // ah note - this is where the final output files of the idl compiler are
  // created
  std::ofstream ofs_callee_h(callee_h);
  std::ofstream ofs_callee_c(callee_c);
  std::ofstream ofs_callee_disp(callee_disp);
  std::ofstream ofs_callee_lds(callee_lds);

  std::ofstream ofs_glue_helper_c(glue_helper_c);
  std::ofstream ofs_glue_helper_h(glue_helper_h);

  std::ofstream ofs_caller_h(caller_h);
  std::ofstream ofs_caller_c(caller_c);
  std::ofstream ofs_caller_disp(caller_disp);

  callee_includes.push_back(
      new Include(true, "../" + m->identifier() + "_callee.h"));
  caller_includes.push_back(
      new Include(true, "../" + m->identifier() + "_caller.h"));

  tree->resolve_types();
  tree->create_trampoline_structs();
  std::cout << "[main.cpp] invoking function_pointer_to_rpc"
            << std::endl; // for debug
  tree->function_pointer_to_rpc();
  tree->generate_function_tags();
  tree->create_container_variables();
  tree->set_copy_container_accessors();
  tree->copy_types();
  tree->initialize_types();
  tree->set_accessors();
  tree->modify_specs();
  tree->prepare_marshal();

  CCSTFile *ccst_callee_h = generate_server_header(m);
  CCSTFile *ccst_callee_lds = generate_callee_lds(m);
  CCSTFile *ccst_callee_c = generate_server_source(m, callee_includes);
  CCSTFile *ccst_callee_disp = generate_dispatch<false>(m);

  CCSTFile *ccst_caller_h = generate_client_header(m);
  CCSTFile *ccst_caller_c = generate_client_source(m, caller_includes);

  // ah note - this function generates all the source code for the client
  // (caller.c)
  CCSTFile *ccst_caller_disp = generate_dispatch<true>(m);

  CCSTFile *ccst_glue_c = generate_glue_source(m);
  CCSTFile *ccst_glue_h = generate_common_header(m);

  if (!ofs_callee_h.is_open() || !ofs_callee_c.is_open() ||
      !ofs_callee_disp.is_open() || !ofs_callee_lds.is_open() ||
      !ofs_glue_helper_c.is_open() || !ofs_caller_h.is_open() ||
      !ofs_caller_c.is_open() || !ofs_caller_disp.is_open() ||
      !ofs_glue_helper_h.is_open()) {

    std::cerr << "Unable to open one or more files for writing! Exiting ... "
              << std::endl;
    exit(-1);
  }

  // Callee header guard macro
  std::string macro_callee_h("__" + m->identifier() + "_callee_h__");
  std_string_toupper(macro_callee_h);

  ofs_callee_h << "#ifndef " << macro_callee_h << "\n";
  ofs_callee_h << "#define " << macro_callee_h << "\n\n";

  ccst_callee_h->write(ofs_callee_h, 0);

  ofs_callee_h << "\n#endif\t/* " << macro_callee_h << " */";

  ccst_callee_c->write(ofs_callee_c, 0);
  ccst_callee_disp->write(ofs_callee_disp, 0);
  ccst_glue_c->write(ofs_glue_helper_c, 0);

  /// Add common glue cap functions 
  std::ifstream cap_file_c("glue_helper/cap.c", std::ios::in);  
  ofs_glue_helper_c << cap_file_c.rdbuf();

  /// Common header guard macro
  std::string macro_glhlpr_h("__" + m->identifier() + "_glue_helper_h__");
  std_string_toupper(macro_glhlpr_h);

  ofs_glue_helper_h << "#ifndef " << macro_glhlpr_h << "\n";
  ofs_glue_helper_h << "#define " << macro_glhlpr_h << "\n\n";

  ccst_glue_h->write(ofs_glue_helper_h, 0);

  ofs_glue_helper_h << "\n#endif\t/* " << macro_glhlpr_h << " */";

  /// FIXME: Should be generated like this. But how to generate SECTIONS {}
  /// statements.push_back(new CCSTPreprocessor("liblcd/trampoline.h", false));
  // ofs_callee_lds << "#include <liblcd/trampoline_link.h>\n\n";
  ofs_callee_lds << "SECTIONS\n{" << std::endl;
  ccst_callee_lds->write(ofs_callee_lds, 1);
  ofs_callee_lds << "}\nINSERT AFTER .text;\n";

  /// Caller header guard macro
  std::string macro_caller_h("__" + m->identifier() + "_caller_h__");
  std_string_toupper(macro_caller_h);

  ofs_caller_h << "#ifndef " << macro_caller_h << "\n";
  ofs_caller_h << "#define " << macro_caller_h << "\n\n";

  if (ccst_caller_h != NULL)
    ccst_caller_h->write(ofs_caller_h, 0);
  else {
    std::cout << "ccst_caller_h is NULL!" << std::endl;
    // std::cout<<"ccst_caller_h is NULL! Terminating program."<<std::endl;
    // std::exit(0);
  }
  ofs_caller_h << "\n#endif\t/* " << macro_caller_h << " */";
  ccst_caller_c->write(ofs_caller_c, 0);
  ccst_caller_disp->write(ofs_caller_disp, 0);

  // recursively generate code for required modules.
  for (auto require : m->requires())
    generate_module_code(require->module_);
}
int main(int argc, char **argv) {
  if (argc != 2) {
    print_usage();
  }
  try {
    char *file = argv[1];

    // This pass recursively parses the included idls and saves them to a
    // map. This pass also does the first pass of the input idl.
    RequirePass *rp = new RequirePass();

    // This pass prints nodes of the AST.
    ASTPrintPass *printpass = new ASTPrintPass();

    Project *tree = rp->do_pass(std::string(file));
    ErrorReport *er = ErrorReport::instance();
    if (er->errors()) {
      std::cerr << "There were errors during parsing\n";
      // TODO: cleanup?
      exit(0);
    }

    printpass->do_pass(tree);
    //	exit(0);//TODO: this line is temporary, remove it after sorting out the
    // stuff below!
    // The following does code generation. TODO: have this extracted out as a
    // separate pass on the ast.

    for (auto m : *tree) {
      generate_module_code(m);
    }
  } catch (const Parser::ParseException &e) {
    std::cerr << "\n\nALERT!!! - Caught parser exception" << std::endl;
    std::cerr << e.getReason() << std::endl;
    exit(0);
  }
}
