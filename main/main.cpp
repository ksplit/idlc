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
    char * file = argv[2];

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
       tree->accept(mv, 0x0);

       CCSTFile* ccst_tree = generate_server_header(tree);
       ccst_tree->write(of);
       */
      std::cout << "TODO: callee header\n";
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

      std::vector<Module*> project_modules = tree->modules();
      std::vector<Include*> project_includes = tree->includes();

      for (std::vector<Module*>::iterator it = project_modules.begin();
          it != project_modules.end(); it++) {
        Module *m = *it;
        if (!m) {
          std::cerr << "Module is null\n";
          exit(0);
        }

        char* of_name = (char*) malloc(
            sizeof(char) * (strlen(m->identifier()) + 10));
        std::ostringstream total;
        total << m->identifier() << "_callee.c";
        strncpy(of_name, total.str().c_str(), strlen(m->identifier()) + 10);

        FILE *of = fopen(of_name, "w");
        if (!of) {
          std::cerr << "Error: unable to open " << of_name << "for writing\n";
          // cleanup
          exit(0);
        }
        CCSTFile* ccst_tree = generate_server_source(m, project_includes);
        ccst_tree->write(of, 0);
        fclose(of);
      }

      std::cout << "Completed callee source writing\n";
    } else if (!strcmp(argv[1], "-clientheader")) {
      /*
       CCSTFile* ccst_tree = generate_client_header(tree);
       ccst_tree->write(of);
       */
      std::cout << "TODO: caller header\n";
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

      std::vector<Module*> project_modules = tree->modules();
      std::vector<Include*> project_includes = tree->includes();

      for (std::vector<Module*>::iterator it = project_modules.begin();
          it != project_modules.end(); it++) {
        Module *m = *it;
        if (!m) {
          std::cerr << "Module is null\n";
          exit(0);
        }

        char* of_name = (char*) malloc(
            sizeof(char) * (strlen(m->identifier()) + 10));
        std::ostringstream total;
        total << m->identifier() << "_caller.c";
        strncpy(of_name, total.str().c_str(), strlen(m->identifier()) + 10);

        FILE *of = fopen(of_name, "w");
        if (!of) {
          std::cerr << "Error: unable to open " << of_name << "for writing\n";
          // TODO: cleanup
          exit(0);
        }
        CCSTFile* ccst_tree = generate_client_source(m, project_includes);
        ccst_tree->write(of, 0);
        fclose(of);
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
