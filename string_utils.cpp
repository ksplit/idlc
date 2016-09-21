#include "code_gen.h"
#include <iostream>
#include <algorithm>
#include <string>

const char* new_name(const char* name, const char* suffix)
{
  std::string *str = new std::string(name);
  str->append(suffix);
  return str->c_str();
}

const char* struct_name(const char* name)
{
  return new_name("struct ", name);
}

const char* lookup_name(const char* name)
{
  return new_name("lookup_", name);
}

const char* insert_name(const char* name)
{
  return new_name(name, "_insert");
}

const char* cap_init_name(const char* name)
{
  return new_name(name, "_cap_init");
}

const char* cap_create_name(const char* name)
{
  return new_name(name, "_cap_create");
}

const char* cap_exit_name(const char* name)
{
  return new_name(name, "_cap_exit");
}

const char* cap_destroy_name(const char* name)
{
  return new_name(name, "_cap_destroy");
}

const char* cspace_name(const char* name)
{
  return new_name(name, "_cspace");
}

const char* group_name(const char* name)
{
  return new_name(name, "_group");
}

/*
 * returns a new string with _exit on the end.
 */
const char* exit_name(const char* name)
{
  return new_name(name, "_exit");
}

/*
 * returns a new string with _init on the end.
 */
const char* init_name(const char* name)
{
  return new_name(name, "_init");
}

/*
 * returns a new stirng with glue_ on the front.
 */
const char* glue_name(const char* name)
{
  return new_name("glue_", name);
}

/*
 * returns a new string with _container on the end.
 */
const char* container_name(const char* name)
{
  return new_name(name, "_container");
}

/*
 * returns a new string with _hidden_args on the end.
 */
const char* hidden_args_name(const char* name)
{
  return new_name(name, "_hidden_args");
}

/* 
 * returns a new string with _p on the end.
 */ 
const char* parameter_name(const char* name)
{
  return new_name(name, "_p");
}

/*
 * returns a new string with _p on the end.
 */
const char* fp_name(const char* name)
{
  return new_name(name, "_fp");
}


/*
 * returns a new string with _container on the end.
 */
const char* trampoline_func_name(const char* name)
{
  return new_name(name, "_trampoline");
}

/*
 * converts input string to uppercase
 */
void std_string_toupper(std::string &input)
{
  std::transform(input.begin(), input.end(), input.begin(),
      std::ptr_fun<int, int>(std::toupper));
}

/*
 * appends strings in list together with the delimiter.
 */
const char* append_strings(const char* delimiter, std::vector<const char*> strs)
{
  int delim_length = strlen(delimiter);
  int length = 0;
  for(std::vector<const char*>::iterator it = strs.begin(); it != strs.end(); it ++) {
    const char* s = *it;
    length += strlen(s);
  }

  char* new_str = (char*) malloc(sizeof(char)*(length+1+( strs.size()-1)*delim_length));

  std::ostringstream total;
  for(std::vector<const char*>::iterator it = strs.begin(); it != strs.end()-1; it ++) {
    const char* s = *it;
    total << s << delimiter;
  }

  const char* s = *(strs.end()-1);
  total << s;
    
  strncpy(new_str, total.str().c_str(), length+1+( strs.size()-1)*delim_length);
  return new_str;
}
