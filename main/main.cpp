#include "lcd_ast.h"
#include "lcd_idl.h"
#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include "ccst.h"
#include "marshal_visitor.h"

int main(int argc, char ** argv)
{
  if(argc != 4)
    {
      printf("error in number of args\n");
      exit(0);
    }
  try
    {
      char * file = argv[2];
      File * tree = (File *) Parser::parse(std::string(file));
      ErrorReport* er = ErrorReport::instance();
      if(er->errors())
	{
	  printf("There were errors during parsing\n");
	  // cleanup?
	  exit(0);
	}
      //   char* out_option = argv[3];
      char* out_file = argv[3];
      printf("out file: %s\n", out_file);
      FILE* of = fopen(out_file, "w");
      
      if(!of)
	{
	  printf("Error: unable to open %s for writing\n", out_file);
	  perror("error");
	  // cleanup
	  exit(0);
	}
      if(!strcmp(argv[1],"-serverheader"))
	{
	  printf("todo\n");

	  MarshalVisitor* mv = new MarshalVisitor();
	  tree->accept(mv);

	  CCSTFile* ccst_tree = generate_server_header(tree);
	  ccst_tree->write(of);
	  printf("completed header source writing\n");
	}
      else if(!strcmp(argv[1],"-serversource"))
	{
	  printf("todo\n");

	  CCSTFile* ccst_tree = generate_server_source(tree);
	  ccst_tree->write(of);
	  printf("completed server source writing\n");
	}
      else if(!strcmp(argv[1], "-clientheader"))
	{
	  printf("todo\n");
	  
	  CCSTFile* ccst_tree = generate_client_header(tree);
	  ccst_tree->write(of);
	  printf("completed client header writing\n");
	}
      else if(!strcmp(argv[1], "-clientsource"))
	{
	  printf("todo\n");
	  
	  CCSTFile* ccst_tree = generate_client_source(tree);
	  ccst_tree->write(of);
	  printf("completed client source writing\n");
	}
      else
	{
	  printf("error unrecognized option: %s\n", argv[1]);
	}
      return 0;
    }
  catch (const Parser::ParseException e)
    {
      printf("caught a parser exception\n");
      //  printf("e is: %s\n", e.getReason().c_str());
      
      exit(0);
    }
}
