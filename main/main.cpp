
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <fstream>

#include "../parser/parser.h"

int main(int argc, char **argv) {
  char *file;
  bool test_mode;
  file = argv[1];
  test_mode = false;

  try {
    Parser::parse(std::string(file));
    return 0;
  } catch (const Parser::ParseException &e) {
    std::cerr << "\n\nALERT!!! - Caught parser exception" << std::endl;
    std::cerr << e.getReason() << std::endl;
    exit(0);
  }
}
