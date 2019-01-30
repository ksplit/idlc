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

void print_usage()
{
  std::cerr << "Usage:\n  ./compiler <idl file>" << std::endl;
  exit(0);
}


int main(int argc, char ** argv) {
  try {
	char* file = argv[1];
	RequirePass *rp = new RequirePass();
	// This pass recursively parses the included requires and saves the
	// info in the same ast at once.  TODO: non recursive approach - parse
	// all the included idls first and then include the ones in the base
	// ast whenever called. However, those included idls may also have
	// requires, so we would still need to use the recursive logic.
	// TODO: need to decide on this. TODO: do the passes on the tree
	// rather than the idl file. 
	//rp->do_pass(tree);
	rp->do_pass(std::string(file));
	// following passes need to come in here
	ErrorReport* er = ErrorReport::instance();
    	if (er->errors()) {
      		std::cerr << "There were errors during parsing\n";
      		// TODO: cleanup?
      		exit(0);
    	}
  } catch(const Parser::ParseException & e) {
         std::cerr << "\n\nALERT!!! - Caught parser exception" << std::endl;
         std::cerr << e.getReason() << std::endl;
         exit(0);
  }
  
  if (argc != 2) {
    print_usage();
  }
  std::cout<<"[main/main.cpp] Enter main\n";

}
