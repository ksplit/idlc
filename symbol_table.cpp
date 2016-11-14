#include "symbol_table.h"

SymbolTable::SymbolTable() :
  last_tmp_(0)
{
  // init symbols
}

SymbolTable::SymbolTable(std::vector<std::string>& symbols) :
  last_tmp_(0)
{
  this->symbols_ = symbols;
}

/*
 * returns a unique const std::string& that begins with tmp 
 * that is not already in symbol table
 * adds to symbol table
 */
const std::string& SymbolTable::unique_tmp()
{
  std::string str_tmp = "tmp" + to_string(this->last_tmp_ + 1);
  char *tmp = (char*) malloc(sizeof(char)*(str_tmp.length()+1));
  strcpy(tmp, str_tmp.c_str());
  if(!contains(tmp)) {
    this->last_tmp_ += 1;
    this->insert(tmp);
    return tmp;
  } else {
    this->last_tmp_ += 1;
    return unique_tmp();
  }
}

/*
 * returns true if symbol table contains symbol
 */
bool SymbolTable::contains(const std::string& symbol)
{
  for(std::vector<std::string>::const_iterator it = this->symbols_.begin(); it != this->symbols_.end(); it ++) {
    std::string str = *it;
    if(str == symbol) {
      return true;
    }
  }
  return false;
}

/*
 * inserts symbol into symbol table
 * if symbol is already in table returns -1
 * returns 0 on success
 */
int SymbolTable::insert(const std::string& symbol)
{
  if(contains(symbol))
    return -1;
  
  this->symbols_.push_back(symbol);
  return 0;
}

/*
 * inserts a list of symbols into the symbol table
 * if any of these symbols are already in the table it fails
 * and returns -1
 * returns 0 on success
 */
int SymbolTable::insert(const std::vector<std::string>& symbols)
{
  for(std::vector<std::string>::const_iterator it = symbols.begin(); it != symbols.end(); it ++) {
    std::string str = *it;
    if (contains(str))
      return -1;
  }
  
  this->symbols_.insert(this->symbols_.end(), symbols.begin(), symbols.end());
  return 0;
}

std::string SymbolTable::to_string(int val)
{
  std::ostringstream os;
  os << val;
  return os.str();
}
