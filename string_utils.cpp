#include "code_gen.h"
#include <algorithm>
#include <iostream>
#include <string>

const std::string new_name(const std::string &name, const std::string &suffix) {
  return std::string(name).append(suffix);
}

const std::string struct_name(const std::string &name) {
  return new_name(std::string("struct "), name);
}

const std::string lookup_name(const std::string &name) {
  return new_name(std::string("glue_cap_lookup_"), name);
}

const std::string insert_name(const std::string &name) {
  return new_name(std::string("glue_cap_insert_"), name);
}

const std::string cap_init_name(const std::string &name) {
  return new_name(name, std::string("_cap_init"));
}

const std::string cap_create_name(const std::string &name) {
  return new_name(name, std::string("_cap_create"));
}

const std::string cap_exit_name(const std::string &name) {
  return new_name(name, std::string("_cap_exit"));
}

const std::string cap_destroy_name(const std::string &name) {
  return new_name(name, std::string("_cap_destroy"));
}

const std::string cspace_name(const std::string &name) {
  return new_name(name, std::string("_cspace"));
}

const std::string group_name(const std::string &name) {
  return new_name(name, std::string("_group"));
}

/*
 * returns a new string with _exit on the end.
 */
const std::string exit_name(const std::string &name) {
  return new_name(name, std::string("_exit"));
}

/*
 * returns a new string with _init on the end.
 */
const std::string init_name(const std::string &name) {
  return new_name(name, std::string("_init"));
}

/*
 * returns a new stirng with glue_ on the front.
 */
const std::string glue_name(const std::string &name) {
  return new_name(std::string("glue_"), name);
}

/*
 * returns a new string with _container on the end.
 */
const std::string container_name(const std::string &name) {
  return new_name(name, std::string("_container"));
}

/*
 * returns a new string with _hidden_args on the end.
 */
const std::string hidden_args_name(const std::string &name) {
  return new_name(name, std::string("_hidden_args"));
}

/*
 * returns a new string with _p on the end.
 */
const std::string parameter_name(const std::string &name) {
  return new_name(name, std::string("_p"));
}

/*
 * returns a new string with _p on the end.
 */
const std::string fp_name(const std::string &name) {
  return new_name(name, std::string("_fp"));
}

/*
 * returns a new string with _container on the end.
 */
const std::string trampoline_func_name(const std::string &name) {
  return new_name(name, std::string("_trampoline"));
}

/*
 * converts input string to uppercase
 */
void std_string_toupper(std::string &input) {
  std::transform(input.begin(), input.end(), input.begin(),
                 std::ptr_fun<int, int>(std::toupper));
}

/*
 * appends strings in list together with the delimiter.
 */
const std::string append_strings(const std::string &delimiter,
                                 const std::vector<std::string> strs) {
  std::string total;

  for (auto str : strs) {
    total.append(str);
    total.append(delimiter);
  }
  // Trim the last underscore to not have __ at the end
  total.resize(total.length() - 1);
  return total;
}
