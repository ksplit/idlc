#include "lcd_ast.h"
#include "lcd_idl.h"
#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include "ccst.h"
#include "code_gen.h"
//#include "marshal_visitor.h"
#include <fstream>

void print_usage()
{
  std::cerr << "Usage:\n  ./compiler [options] <idl file>\n"
      "Options:\n  -serverheader\t generate callee header\n"
      "  -serversource\t generate callee code\n"
      "  -clientheader\t generate caller header\n"
      "  -clientsource\t generate caller code\n";
  exit(0);
}

int main(int argc, char ** argv)
{
  bool pDebug = false;

  if (argc != 3) {
    print_usage();
  }

  try {
    char* file = argv[2];

    // TODO: add support for multiple files, add option to specify
    // which module to compile, put each module in a different file
    Project * tree = (Project *) Parser::parse(std::string(file));

    ErrorReport* er = ErrorReport::instance();
    if (er->errors()) {
      std::cerr << "There were errors during parsing\n";
      // TODO: cleanup?
      exit(0);
    }

    if (pDebug) {
      std::cout << "successfully parsed project.\n";
    }

    if (!strcmp(argv[1], "-serverheader")) {
      /*
       MarshalVisitor* mv = new MarshalVisitor();
       tree->accept(mv, 0x0);*/
      // convert func pointer to rpc so that we can generate enums
      tree->function_pointer_to_rpc();

      for (auto m : *tree) {
        //FIXME: Looks cluttered. Refactor appropriately
        std::string fname(m->identifier());
        std::string lds_fname(m->identifier());
        fname.append("_callee.h");
        lds_fname.append("_callee.lds.S");

        std::ofstream of_hdr(fname);
        std::ofstream of_lds(lds_fname);

        if (!of_hdr.is_open() || !of_lds.is_open()) {
          if (!of_lds.is_open())
            std::cerr << "Error: unable to open " << lds_fname << "for writing\n";
          if (!of_hdr.is_open())
            std::cerr << "Error: unable to open " << fname << "for writing\n";
          exit(-1);
        }

        CCSTFile* ccst_hdr = generate_server_header(m);
        CCSTFile* ccst_lds = generate_callee_lds(m);

        std::string hdr_macro("__");
        hdr_macro.append(m->identifier());
        hdr_macro.append("_callee_h__");
        std_string_toupper(hdr_macro);

        of_hdr << "#ifndef " << hdr_macro << std::endl;
        of_hdr << "#define " << hdr_macro << "\n\n";
        ccst_hdr->write(of_hdr, 0);
        of_hdr << "\n#endif /* " << hdr_macro << " */" << std::endl;

        /// FIXME: Should be generated like this. But how to generate SECTIONS {}
        /// statements.push_back(new CCSTPreprocessor("liblcd/trampoline.h", false));
        of_lds << "#include <liblcd/trampoline_link.h>\n\n";
        of_lds << "SECTIONS\n{" << std::endl;
        ccst_lds->write(of_lds, 1);
        of_lds << "}\nINSERT AFTER .text;\n";
      }
    } else if (!strcmp(argv[1], "-serversource")) {
      // Callee code
      tree->resolve_types();
      tree->create_trampoline_structs();
      tree->function_pointer_to_rpc();
      tree->generate_function_tags();
      // resolve types that weren't resolved during parsing
      tree->create_container_variables();

      // TODO
      //tree->set_copy_container_accessors();

      tree->copy_types();
      tree->initialize_types(); // for calling initialize o
      tree->set_accessors();
      tree->prepare_marshal();
      std::cout << "Done doing all setup\n";

      std::vector<Include*> project_includes = tree->includes();
      for (auto m : *tree) {
        if (!m) {
          std::cerr << "Module is null\n";
          exit(0);
        }

        std::string fname = new_name(m->identifier(), std::string("_callee.c"));
        std::ofstream of_callee(fname);
        std::ofstream of_glue(m->identifier() + "_cap.c");

        project_includes.push_back(new Include(true, "../" + m->identifier() + "_callee.h"));

        if (!of_callee.is_open()) {
          std::cerr << "Error: unable to open " << fname << "for writing\n";
          // cleanup
          exit(0);
        }
        CCSTFile* ccst_tree = generate_server_source(m, project_includes);
        CCSTFile *glue_ccst_tree = generate_glue_source(m);
        glue_ccst_tree->write(of_glue, 0);
        ccst_tree->write(of_callee, 0);
      }

      std::cout << "Completed callee source writing\n";
    } else if (!strcmp(argv[1], "-clientheader")) {
       tree->function_pointer_to_rpc();

       for (auto m : *tree) {
         std::string fname (m->identifier());
         fname.append("_caller.h");

         std::ofstream of_hdr(fname);

         if (!of_hdr.is_open()) {
           std::cerr << "Error: unable to open " << fname << "for writing\n";
           exit(0);
         }

         CCSTFile* ccst_tree = generate_client_header(m);

         std::string hdr_macro("__");
         hdr_macro.append(m->identifier());
         hdr_macro.append("_caller_h__");
         std_string_toupper(hdr_macro);

         of_hdr << "#ifndef " << hdr_macro << std::endl;
         of_hdr << "#define " << hdr_macro << "\n\n";
         ccst_tree->write(of_hdr, 0);
         of_hdr << "\n#endif /* " << hdr_macro << " */" << std::endl;
       }
    } else if (!strcmp(argv[1], "-clientsource")) {
      // Caller code
      tree->create_trampoline_structs();
      tree->function_pointer_to_rpc();
      tree->generate_function_tags();
      // resolve types that weren't resolved during parsing
      tree->resolve_types();
      tree->create_container_variables();
      // TODO
      tree->set_copy_container_accessors();

      tree->copy_types();
      tree->initialize_types(); // for calling initialize o
      tree->set_accessors();
      tree->prepare_marshal();
      std::cout << "Done doing all setup\n";

      std::vector<Include*> project_includes = tree->includes();

      for (auto m : *tree) {
        if (!m) {
          std::cerr << "Module is null\n";
          exit(0);
        }

        std::string fname = new_name(m->identifier(), std::string("_caller.c"));
        std::ofstream of_caller(fname);

        project_includes.push_back(new Include(true, "../" + m->identifier() + "_caller.h"));

        if (!of_caller.is_open()) {
          std::cerr << "Error: unable to open " << fname << "for writing\n";
          // TODO: cleanup
          exit(0);
        }

        CCSTFile* ccst_tree = generate_client_source(m, project_includes);
        ccst_tree->write(of_caller, 0);
      }
      std::cout << "Completed caller source writing\n";
    } else {
      std::cerr << "Error unrecognized option: " << argv[1] << std::endl;
    }
    return 0;
  } catch (const Parser::ParseException & e) {
    std::cerr << "\n\nALERT!!! - Caught parser exception" << std::endl;
    std::cerr << e.getReason() << std::endl;
    exit(0);
  }
}
