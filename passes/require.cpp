// This pass expands requires in a given file.

#include <lcd_idl.h>
#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Passes/require.h"
#include <map>

//namespace Parser {};
//using namespace Parser;

void RequirePass::test_func(){
  std::cout<<"test this function"<<std::endl;
}

std::map<std::string,Project*> idlMap; 
std::map<std::string, Module*> moduleMap;


/*
 Structure of IDL Files:
 
 IDL |--> Modules --> Requires --> Modules
     |
     |	     	   |--> IDL1		
     |--> Includes |--> IDL2 
		   |--> IDL3

 Pass Approach:

 1. Parse all the included idl files in an IDL recursively storing their asts
 in a map. - TODO: handle cyclic dependency case, else we may be looping between
 two idl files that include each other, or multiple idl files that form a cycle.  

 2. Go through the map, and generate a new map containing module names and asts
 of those modules only. - CAVEAT: this technique assumes module names are unique
 across all the idl files.  TODO: need to handle the case where the module names
 may not be unique across idl files.  

 3. Save refs to these modules in all the require nodes recursively - i.e.
 expanding the require nodes of each module before saving them.
*/  
Project * RequirePass::do_pass(std::string input){
	std::cout<<__FILE__<<"- Performing RequirePass"<<std::endl;
	Project * tree = process_includes(input);
	std::cout<<"Parsed input:  "<<input<<std::endl;
	create_module_map();
	print_maps();
	tree = resolve_requires(tree);
	return tree;
}

Project * RequirePass::resolve_requires(Project * tree){
	for (auto m : *tree) {
	  expand_module_requires(m);
	}
	return tree;
}

Module * RequirePass::expand_module_requires(Module * m){
	std::vector<Require*> module_requires = m->requires();
	std::string required_module;
	for (auto require : module_requires) {
          required_module = require->get_required_module_name();
	  require->save_ast(expand_module_requires(moduleMap.find(required_module)->second));
	}
	return m;
}

void RequirePass::print_maps(){
	std::map<std::string,Project*>::iterator itr1;
	for (itr1 = idlMap.begin(); itr1 != idlMap.end(); itr1++) {
	  std::cout<<__FILE__<<" idlMap: "<<itr1->first<<std::endl;
	}
	std::map<std::string,Module*>::iterator itr2;
	for (itr2 = moduleMap.begin(); itr2 != moduleMap.end(); itr2++) {
	  std::cout<<__FILE__<<" moduleMap: "<<itr2->first<<std::endl;
	}
}

void RequirePass::create_module_map(){
	Project * tree;	
	std::map<std::string,Project*>::iterator itr;
	for (itr = idlMap.begin(); itr != idlMap.end(); itr++) {
	  tree = itr->second;
	  for (auto m: *tree) {
	    moduleMap.insert(std::pair<std::string, Module*>(m->identifier(), m));
	  }
	}
}

Project * RequirePass::process_includes(std::string input){
	std::cout<<__FILE__<<"- Performing RequirePass"<<std::endl;
	Project * tree = (Project *) Parser::parse(input);
	std::cout<<"Parsed input:  "<<input<<std::endl;
	ErrorReport* er = ErrorReport::instance();
	if (er->errors()) {
	  std::cerr << "There were errors during parsing\n";
	  // TODO: cleanup?
	  exit(0);
	}
	std::vector<Include*> project_includes = tree->includes();
	std::string included_idl;
	// Parsing all IDL files recursively
	for (auto inc : project_includes){
	  included_idl = inc->get_path();
          std::cout<<__FILE__<<"- included header idl: "<<inc->get_path()<<std::endl;
	  idlMap.insert(std::pair<std::string, Project*>(std::string(included_idl), process_includes(included_idl)));
		}		
	return tree;
}

