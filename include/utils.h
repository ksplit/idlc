#ifndef UTILS_H
#define UTILS_H

const std::string new_name(const std::string & name,
  const std::string& suffix);
const std::string struct_name(const std::string& name);
const std::string lookup_name(const std::string& name);
const std::string insert_name(const std::string& name);
const std::string cap_init_name(const std::string& name);
const std::string cap_create_name(const std::string& name);
const std::string cap_exit_name(const std::string& name);
const std::string cap_destroy_name(const std::string& name);
const std::string cspace_name(const std::string& name);
const std::string group_name(const std::string& name);
const std::string exit_name(const std::string& name);
const std::string init_name(const std::string& name);
const std::string glue_name(const std::string& name);
const std::string container_name(const std::string& name);
const std::string hidden_args_name(const std::string& name);
const std::string parameter_name(const std::string& name);
const std::string fp_name(const std::string& name);
const std::string trampoline_func_name(const std::string& name);
const std::string append_strings(const std::string& delimiter,
  const std::vector<std::string> strs);

bool dealloc_caller(Variable *v, const std::string& side);
bool dealloc_callee(Variable *v, const std::string& side);
bool in(Variable *v, const std::string& side);
bool out(Variable *v, const std::string& side);

void std_string_toupper(std::string &input);

#endif /* UTILS_H */
