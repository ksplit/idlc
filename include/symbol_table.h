#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

class SymbolTable {
  unsigned int last_tmp_;
  std::vector<std::string> symbols_;

public:
  SymbolTable();
  SymbolTable(std::vector<std::string> &symbols);
  const std::string &unique_tmp();
  bool contains(const std::string &symbol);
  int insert(const std::string &symbol);
  int insert(const std::vector<std::string> &symbols);
  std::string to_string(int value);
};

#endif
