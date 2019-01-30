// This passes expands requires in a given file.

#include "lcd_idl.h"
#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <regex>
#include <string.h>
#include "ccst.h"
#include "code_gen.h"
#include "require.h"
#include <fstream>

void RequirePass::test_func(){
  std::cout<<"test this function"<<std::endl;
}

//Project * RequirePass::do_pass(Project * tree){
Project * RequirePass::do_pass(std::string input){
    std::cout<<"[RequirePass::do_pass()] Performing RequirePass"<<std::endl;
    // TODO: add support for multiple files, add option to specify
    // which module to compile, put each module in a different file
    // ah note - this is where the input idl is parsed. The output of
    // the parse is a tree. This is the AST, on which we generate the CCST
    // (the concrete syntax tree) - from which the code is eventually 
    // generated.
    std::cout<<"Parsing: "<<input<<std::endl;
    Project * tree = (Project *) Parser::parse(input);

    ErrorReport* er = ErrorReport::instance();
    if (er->errors()) {
      std::cerr << "There were errors during parsing\n";
      // TODO: cleanup?
      exit(0);
    }

    std::vector<Include*> project_includes = tree->includes();
    std::vector<Include*> caller_includes = project_includes;
    std::vector<Include*> callee_includes = project_includes;

    int check_num=0;
    for (auto m : *tree) {
      std::cout<<"Module number "<<check_num<<std::endl;
      check_num++;
      std::vector<Require*> module_requires = m->requires();

      // search for the include that matches the required module.
      std::string required_module;
      std::string included_idl;
      std::string included_idl_name;

      // start of processing required modules
      for (auto require : module_requires) {
        required_module = require->get_required_module_name();
        std::cout<<"[main.cpp] requiring: "<<required_module<<std::endl;
        for (auto inc : project_includes) {
	  included_idl = inc->get_path();
          std::cout<<"[main.cpp] included header idl: "<<included_idl<<std::endl;

	  // Preprocessing IDL file - Checking whether the IDL file contains
	  // the required module:  (1) scan the file included for the required
	  // module name.  (2) if it contains it, parse the file and save the
	  // parse tree.

	  std::ifstream idl_file;
	  std::string line;
  	  idl_file.open(included_idl);
	  std::string *prepattern = new std::string("(\\s*)(\\t*)(module)(\\s*)(\\t*)(");
	  std::string *postpattern = new std::string(")(.*)");
	  std::string *pattern = new std::string(*prepattern + required_module.c_str()+ *postpattern);
	  std::cout << "pattern: "<< *pattern<<std::endl;

	  while(std::getline(idl_file, line)) {
	  	std::regex rgx(*pattern);
	  	std::cout<<"[main.cpp] scanning line: "<<line<<std::endl;	
    		if (std::regex_match(line, rgx)) {
	  	  std::cout<<"[main.cpp] matched line: "<<line<<std::endl;	
	    	  std::cout<<"[main.cpp] this included idl contains the required module. Parsing this idl "<<std::endl;
            	  require->save_ast(do_pass(included_idl));
		  break;
	  	}
	  }
  	  idl_file.close();


	  // The following logic assumes that the module name is the same as
	  // the name of the included idl file. (not using this because a file
	  // may have multiple modules.)
          /*
	  included_idl_name = included_idl.substr(0, included_idl.size()-4);
          std::cout<<"[main.cpp] included header name: "<< included_idl_name <<std::endl;
          if (included_idl_name.compare(required_module) == 0) { 
	    std::cout<<"[main.cpp] found required module. Invoking parse on the required idl"<<std::endl;
            require->save_ast_of_idl_of_required_module(process_idl(included_idl));
	  }
	  */

	}
      }
      // end of processing required modules

      // The following does code generation. TODO: have this extracted out as a
      // separate pass on the ast.	
      std::string callee_h = new_name(m->identifier(), std::string("_callee.h"));
      std::string callee_c = new_name(m->identifier(), std::string("_callee.c"));
      std::string callee_disp = new_name(m->identifier(), std::string("_callee_dispatch.c"));
      std::string callee_lds = new_name(m->identifier(), std::string("_callee.lds.S"));

      std::string glue_helper_c = new_name(m->identifier(), std::string("_glue_helper.c"));
      std::string glue_helper_h = new_name(m->identifier(), std::string("_glue_helper.h"));

      std::string caller_h = new_name(m->identifier(), std::string("_caller.h"));
      std::string caller_c = new_name(m->identifier(), std::string("_caller.c"));
      std::string caller_disp = new_name(m->identifier(), std::string("_caller_dispatch.c"));

      // ah note - this is where the final output files of the idl compiler are created
      std::ofstream ofs_callee_h(callee_h);
      std::ofstream ofs_callee_c(callee_c);
      std::ofstream ofs_callee_disp(callee_disp);
      std::ofstream ofs_callee_lds(callee_lds);

      std::ofstream ofs_glue_helper_c(glue_helper_c);
      std::ofstream ofs_glue_helper_h(glue_helper_h);

      std::ofstream ofs_caller_h(caller_h);
      std::ofstream ofs_caller_c(caller_c);
      std::ofstream ofs_caller_disp(caller_disp);

      callee_includes.push_back(new Include(true, "../" + m->identifier() + "_callee.h"));
      caller_includes.push_back(new Include(true, "../" + m->identifier() + "_caller.h"));

      tree->resolve_types();
      tree->create_trampoline_structs();
      std::cout<<"[main.cpp] invoking function_pointer_to_rpc"<<std::endl;//for debug
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

      if (!ofs_callee_h.is_open() || !ofs_callee_c.is_open()
        || !ofs_callee_disp.is_open() || !ofs_callee_lds.is_open()
        || !ofs_glue_helper_c.is_open() || !ofs_caller_h.is_open()
        || !ofs_caller_c.is_open() || !ofs_caller_disp.is_open()
        || !ofs_glue_helper_h.is_open()) {

        std::cerr
          << "Unable to open one or more files for writing! Exiting ... "
          << std::endl;
        exit(-1);
      }

      // Callee header guard macro
      std::string macro_callee_h("__" + m->identifier() + "_callee_h__");
      std_string_toupper(macro_callee_h);

      ofs_callee_h << "#pragma once"<< std::endl;
      ccst_callee_h->write(ofs_callee_h, 0);

      ccst_callee_c->write(ofs_callee_c, 0);
      ccst_callee_disp->write(ofs_callee_disp, 0);
      ccst_glue_c->write(ofs_glue_helper_c, 0);

      /// Common header guard macro
      std::string macro_glhlpr_h("__" + m->identifier() + "_glue_helper_h__");
      std_string_toupper(macro_glhlpr_h);

      ofs_glue_helper_h <<  "#pragma once"<< std::endl;
      ccst_glue_h->write(ofs_glue_helper_h, 0);


      /// FIXME: Should be generated like this. But how to generate SECTIONS {}
      /// statements.push_back(new CCSTPreprocessor("liblcd/trampoline.h", false));
      //ofs_callee_lds << "#include <liblcd/trampoline_link.h>\n\n";
      ofs_callee_lds << "SECTIONS\n{" << std::endl;
      ccst_callee_lds->write(ofs_callee_lds, 1);
      ofs_callee_lds << "}\nINSERT AFTER .text;\n";

      /// Caller header guard macro
      std::string macro_caller_h("__" + m->identifier() + "_caller_h__");
      std_string_toupper(macro_caller_h);

      ofs_caller_h << "#pragma once"<< std::endl;
      
      if (ccst_caller_h!=NULL)
      ccst_caller_h->write(ofs_caller_h, 0);
      else {
       std::cout<<"ccst_caller_h is NULL! Terminating program."<<std::endl;
       std::exit(0);
      }     
 
      ccst_caller_c->write(ofs_caller_c, 0);
      ccst_caller_disp->write(ofs_caller_disp, 0);
    }

    return tree;

}
