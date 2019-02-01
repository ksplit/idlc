#include "lcd_ast.h"
#include "utils.h"
#include <stdio.h>
#define FILENAME "[lcd_ast.cpp]"
Rpc::Rpc(ReturnVariable *return_value, const std::string& name,
  std::vector<Parameter*> parameters, LexicalScope *current_scope) :
  tag_(0),
  explicit_return_(return_value),
  current_scope_(current_scope),
  name_(name),
  enum_str(name),
  parameters_(parameters),
  function_pointer_defined_(false)
{
  this->symbol_table_ = new SymbolTable();

  // Convert name to upper case for writing enums
  std_string_toupper(this->enum_str);

  for (auto p : *this) {
    this->symbol_table_->insert(p->identifier());
  }
}

unsigned int Rpc::tag()
{
  return this->tag_;
}

void Rpc::set_tag(unsigned int t)
{
  this->tag_ = t;
}

std::vector<Variable*> Rpc::marshal_projection_parameters(ProjectionType *pt,
  const std::string& direction)
{
  std::vector<Variable*> marshal_parameters;

  for (auto pf : *pt) {
    if ((direction == "in" && pf->in()) || (direction == "out" && pf->out())
      || (direction == "inout" && pf->in() && pf->out()) || direction == "") {

      if (pf->type()->num() == PROJECTION_TYPE
        || pf->type()->num() == PROJECTION_CONSTRUCTOR_TYPE) {
        auto *pt_tmp = dynamic_cast<ProjectionType*>(pf->type());
        Assert(pt_tmp != 0x0,
          "Error: dynamic cast to Projection type failed.\n");
        auto tmp_params = marshal_projection_parameters(pt_tmp, direction);
        marshal_parameters.insert(marshal_parameters.end(), tmp_params.begin(),
          tmp_params.end());
      } else {
        marshal_parameters.push_back(pf);
      }

      if (pf->container() != 0x0) {
        auto *con = pf->container();
        if ((direction == "in" && con->in())
          || (direction == "out" && con->out())
          || (direction == "inout" && con->in() && pf->out())
          || direction == "") {

          if (con->type()->num() == PROJECTION_TYPE
            || con->type()->num() == PROJECTION_CONSTRUCTOR_TYPE) {
            auto *pt_tmp = dynamic_cast<ProjectionType*>(con->type());
            Assert(pt_tmp != 0x0,
              "Error: dynamic cast to Projection type failed.\n");
            auto tmp_params = marshal_projection_parameters(pt_tmp, direction);
            marshal_parameters.insert(marshal_parameters.end(),
              tmp_params.begin(), tmp_params.end());
          } else {
            marshal_parameters.push_back(con);
          }
        }
      }
    }
  }
  return marshal_parameters;
}

void Rpc::create_container_variables()
{
  // for each parameter that is a pointer, need to create a container variable
  std::cout << "in create container variables for " <<  this->name_ << std::endl;
  for (auto p: *this) {
    p->create_container_variable(this->current_scope());
  }
  this->return_variable()->create_container_variable(this->current_scope());
}

void Rpc::set_function_pointer_defined(bool b)
{
  this->function_pointer_defined_ = b;
}

void Rpc::set_hidden_args(std::vector<Parameter*> hidden_args)
{
  this->hidden_args_ = hidden_args;
}

bool Rpc::function_pointer_defined()
{
  return this->function_pointer_defined_;
}

ReturnVariable* Rpc::return_variable()
{
  return this->explicit_return_;
}

const std::string Rpc::name() const
{
  return name_;
}

/*
Marshal_type* Rpc::accept(MarshalVisitor* worker, Registers *data)
{
  return worker->visit(this, data);
}
*/

LexicalScope* Rpc::current_scope()
{
  return this->current_scope_;
}

const std::string Rpc::callee_name() const
{
  return new_name(this->name_, "_callee");
}

const std::string& Rpc::enum_name() const
{
  return this->enum_str;
}

std::vector<Parameter*> Rpc::parameters()
{
  return parameters_;
}

void Rpc::prepare_marshal()
{
  // TODO: account for hidden args
  std::vector<Variable*> in_params;
  std::vector<Variable*> out_params;
  std::vector<Variable*> in_out_params;

  auto all_params = this->parameters_;
  if (this->function_pointer_defined()) {
    all_params.insert(all_params.end(), this->hidden_args_.begin(),
      this->hidden_args_.end());
  }

  // sort our parameters
  for (auto p : all_params) {
    std::cout << "parameter we are going to marshal is " << p->identifier()
      << std::endl;
    /// Nothing needs to be marshalled for a function pointer
    /// skip to marshalling its container
    if (p->type()->num() == FUNCTION_TYPE) {
      goto marshal_container;
    }

    if (p->in() && !p->out()) {
      /// We just need the function pointer's container. So skip the func_ptr
      in_params.push_back(p);
      if (p->type()->num() == PROJECTION_TYPE
        || p->type()->num() == PROJECTION_CONSTRUCTOR_TYPE) {
        auto *pt = dynamic_cast<ProjectionType*>(p->type());
        Assert(pt != 0x0, "Error: dynamic cast to projection failed\n");
        auto tmp = this->marshal_projection_parameters(pt, "in");
        in_params.insert(in_params.end(), tmp.begin(), tmp.end());
      }
    } else if (!p->in() && p->out()) {
      out_params.push_back(p);
      if (p->type()->num() == PROJECTION_TYPE
        || p->type()->num() == PROJECTION_CONSTRUCTOR_TYPE) {
        auto *pt = dynamic_cast<ProjectionType*>(p->type());
        Assert(pt != 0x0, "Error: dynamic cast to projection failed\n");
        auto tmp = this->marshal_projection_parameters(pt, "out");
        out_params.insert(out_params.end(), tmp.begin(), tmp.end());
      }
    } else if (p->in() && p->out()) {
      in_out_params.push_back(p);
      if (p->type()->num() == PROJECTION_TYPE
        || p->type()->num() == PROJECTION_CONSTRUCTOR_TYPE) {
        auto *pt = dynamic_cast<ProjectionType*>(p->type());
        Assert(pt != 0x0, "Error: dynamic cast to projection failed\n");
        // in
        auto tmp = this->marshal_projection_parameters(pt, "in");
        in_params.insert(in_params.end(), tmp.begin(), tmp.end());

        // out
        auto tmp2 = this->marshal_projection_parameters(pt, "out");
        out_params.insert(out_params.end(), tmp2.begin(), tmp2.end());

        // in out
        auto tmp3 = this->marshal_projection_parameters(pt, "inout");
        in_out_params.insert(in_out_params.end(), tmp3.begin(), tmp3.end());
      }

    }

    marshal_container:
    // have to do it for container too!!!
    if (p->container()) {
      auto *container = p->container();
      std::cout << "parameter container we are going to marshal is "
        << p->container()->identifier() << std::endl;

      if (container->in() && !container->out()) {
        //in_params.push_back(container);
        if (container->type()->num() == PROJECTION_TYPE
          || container->type()->num() == PROJECTION_CONSTRUCTOR_TYPE) {
          auto *pt = dynamic_cast<ProjectionType*>(container->type());
          Assert(pt != 0x0, "Error: dynamic cast to projection failed\n");
          auto tmp = this->marshal_projection_parameters(pt, "in");
          in_params.insert(in_params.end(), tmp.begin(), tmp.end());
        }
      } else if (!container->in() && container->out()) {
        //out_params.push_back(container);
        if (container->type()->num() == PROJECTION_TYPE
          || container->type()->num() == PROJECTION_CONSTRUCTOR_TYPE) {
          auto *pt = dynamic_cast<ProjectionType*>(container->type());
          Assert(pt != 0x0, "Error: dynamic cast to projection failed\n");
          auto tmp = this->marshal_projection_parameters(pt, "out");
          out_params.insert(out_params.end(), tmp.begin(), tmp.end());
        }
      } else if (container->in() && container->out()) {
        //in_out_params.push_back(container);
        if (container->type()->num() == PROJECTION_TYPE
          || container->type()->num() == PROJECTION_CONSTRUCTOR_TYPE) {
          auto *pt = dynamic_cast<ProjectionType*>(container->type());
          Assert(pt != 0x0, "Error: dynamic cast to projection failed\n");
          // in
          auto tmp = this->marshal_projection_parameters(pt, "in");
          in_params.insert(in_params.end(), tmp.begin(), tmp.end());

          // out
          auto tmp2 = this->marshal_projection_parameters(pt, "out");
          out_params.insert(out_params.end(), tmp2.begin(), tmp2.end());

          // in out
          auto tmp3 = this->marshal_projection_parameters(pt, "inout");
          in_out_params.insert(in_out_params.end(), tmp3.begin(), tmp3.end());
        }

      }
    }
  }

  // assign register(s) to return value
  if (this->explicit_return_->type()->num() != VOID_TYPE)  {
    out_params.push_back(this->explicit_return_);
  }
  if (this->explicit_return_->type()->num() == PROJECTION_TYPE
    || this->explicit_return_->type()->num() == PROJECTION_CONSTRUCTOR_TYPE) {
    auto *pt = dynamic_cast<ProjectionType*>(this->explicit_return_->type());
    Assert(pt != 0x0, "Error: dynamic cast to projection failed\n");
    if (this->explicit_return_->container()) {
      auto *pt1 = dynamic_cast<ProjectionType*>(this->explicit_return_->container()->type());
      Assert(pt1 != 0x0, "Error: dynamic cast to projection failed\n");
      /// Since we need to pass our reference when calling and get back the
      /// callee's reference while returning, the param needs to be both
      /// in and out
      auto tmp1 = this->marshal_projection_parameters(pt1, "in");
      auto tmp = this->marshal_projection_parameters(pt1, "out");
      out_params.insert(out_params.end(), tmp.begin(), tmp.end());
      in_params.insert(in_params.end(), tmp1.begin(), tmp1.end());
    }
  }

  // marshal prepare the in parameters
  auto *in_reg = new Registers();
  int arr[1];
  /// make sure register 0 is free for function tag (only for sync)
  /// This is needed only for sync registers
  /// TODO: Remove this for async after introducing a sync marker in
  /// the grammar
  arr[0] = 1;
  in_reg->init(arr, 1, 0x0, 0);

  auto *in_marshal_worker = new MarshalPrepareVisitor(in_reg);

  for (auto v : in_params) {
    v->prepare_marshal(in_marshal_worker);
  }

  // marshal prepare the out parameters
  auto *out_reg = new Registers();
  out_reg->init(arr, 1, 0x0, 0);

  auto *out_marshal_worker = new MarshalPrepareVisitor(out_reg);

  for (auto v : out_params) {
    v->prepare_marshal(out_marshal_worker);
  }

  // marshal prepare for the in/out params.  meaning they need only 1 register for both ways
  // need to get the set union of in_marshal_worker's registers and out_marshal_worker's registers
  auto *in_out_regs = new Registers();
  in_out_regs->init(in_reg, out_reg);

  auto *in_out_marshal_worker = new MarshalPrepareVisitor(in_out_regs);

  for (auto v : in_out_params) {
    v->prepare_marshal(in_out_marshal_worker);
  }
}

void Rpc::resolve_types()
{  
  // marshal prepare for parameters as long as they are in or out
  for (auto p : *this) {
    p->resolve_types(this->current_scope_);
  }
  
  // marshal prepare for return value
  this->explicit_return_->resolve_types(this->current_scope_);
}

void Rpc::copy_types()
{
  // copy parameters.
  for (auto p : *this) {
    p->type_ = p->type_->clone();
    if (p->container_ != 0x0) {
      p->container_ = p->container_->clone();
    }
  }
  
  // copy return type
  this->explicit_return_->type_ = this->explicit_return_->type_->clone();
  if (this->explicit_return_->container_) {
    this->explicit_return_->container_ = this->explicit_return_->container_->clone();
  }
}

void Rpc::set_accessors()
{
  // return variable
  this->explicit_return_->set_accessor(0x0);

  // parameters
  for (auto p : *this) {
    p->set_accessor(0x0);
  }
}

void Rpc::modify_specs()
{
  /// XXX: is it needed for return variable?
  /// for parameters
  for (auto p : *this) {
    p->modify_specs();
  }
}

void Rpc::set_copy_container_accessors()
{
  // return variable
  this->explicit_return_->set_accessor(0x0);

  // parameters
  for (auto p : *this) {
    p->set_accessor(0x0);
  }
}

void Rpc::initialize_types()
{
  // parameters
  for (auto p : *this) {
    p->initialize_type();
  }

  // return variable
  this->explicit_return_->initialize_type();
}

void Rpc::create_trampoline_structs()
{
  for (auto p : *this) {
    if (p->type()->num() == FUNCTION_TYPE) {
      Function *f = dynamic_cast<Function*>(p->type());
      Assert(f != 0x0, "Error: dynamic cast to function type failed!\n");

      std::vector<ProjectionField*> trampoline_fields;
      int err;
      trampoline_fields.push_back(new ProjectionField(this->current_scope_->lookup("cspace", &err), "cspace", 1)); // dstore field
      trampoline_fields.push_back(new ProjectionField(this->current_scope_->lookup("lcd_trampoline_handle", &err), "t_handle", 1)); // lcd_trampoline handle field
      
      const std::string& trampoline_struct_name = hidden_args_name(f->name());
      this->current_scope_->insert(trampoline_struct_name, new ProjectionType(trampoline_struct_name, trampoline_struct_name, trampoline_fields));
    }
  }
}

void Require::save_ast(Module * module){
  this->module_ = module;
}

Module::Module(const std::string& id, std::vector<Rpc*> rpc_definitions,
  std::vector<GlobalVariable*> channels, LexicalScope *ls, std::vector<Require*> requires) :
  module_name_(id),
  module_scope_(ls),
  channels_(channels),
  rpc_definitions_(rpc_definitions),
  requires_(requires)
{
  std::cout<<FILENAME<<" Creating new module"<<std::endl;
  std::cout<<FILENAME<<" -module name: "<<id<<std::endl;
  std::cout<<FILENAME<<" -rpcs size: "<<rpc_definitions.size()<<std::endl;
  std::cout<<FILENAME<<" -requires size: "<<requires.size()<<std::endl;

  this->module_scope_->setactiveChannel(ls->activeChannel);
  if (ls->activeChannel) {
    std::cout << "Active channel " << ls->activeChannel->name() << std::endl;
  }

  int err;
  Type *cspace = this->module_scope_->lookup("glue_cspace", &err);
  if(!cspace) {
    cspace = new UnresolvedType("glue_cspace");
  }
  // create cspaces.
  for (auto gv : channels_) {
    this->cspaces_.push_back(new GlobalVariable(cspace, cspace_name(gv->identifier()), 1));
  }
  
  // create channel group
  Type *group = this->module_scope_->lookup("lcd_sync_channel_group", &err);
  if(!group) {
    group = new UnresolvedType("lcd_sync_channel_group");
  }

  this->channel_group = new GlobalVariable(group, group_name(this->identifier()), 1);
}

std::vector<Rpc*> Module::rpc_definitions()
{
  return this->rpc_definitions_;
}

std::vector<Require*> Module::requires()
{
  return this->requires_;
}

std::vector<GlobalVariable*> Module::channels()
{
  return this->channels_;
}

LexicalScope* Module::module_scope()
{
  return this->module_scope_;
}

void Module::prepare_marshal()
{
  for (auto rpc: *this) {
    rpc->prepare_marshal();
  }
}

void Module::resolve_types()
{
  // need to resolve types in projections.
  this->module_scope_->resolve_types();
  
  for (auto rpc: *this) {
    rpc->resolve_types();
  }
}

void Module::copy_types()
{
  for (auto rpc: *this) {
    rpc->copy_types();
  }
}

void Module::set_accessors()
{
  for (auto rpc: *this) {
    rpc->set_accessors();
  }
}

void Module::modify_specs()
{
  for (auto rpc: *this) {
    rpc->modify_specs();
  }
}

void Module::initialize_types()
{
  for (auto rpc: *this) {
    rpc->initialize_types();
  }
}

void Module::function_pointer_to_rpc()
{
  std::vector<Rpc*> rpcs = this->module_scope()->function_pointer_to_rpc();
  if (rpcs.size()!=0)
   std::cout<<"lcd_ast.cpp-Module::function_pointer_to_rpc()-Inserted an rpc for a function pointer."<<std::endl;
  this->rpc_definitions_.insert(this->rpc_definitions_.end(), rpcs.begin(), rpcs.end());
}

void Module::create_trampoline_structs()
{
  this->module_scope_->create_trampoline_structs();
  // loop through rpc definitions
  // todo

  for (auto rpc: *this) {
    rpc->create_trampoline_structs();
  }
}

void Module::generate_function_tags(Project *p)
{
  for (auto rpc: *this) {
    rpc->set_tag(p->get_next_tag());
  }
}

void Module::create_container_variables()
{
  for (auto rpc: *this) {
    rpc->create_container_variables();
  }
}

void Module::set_copy_container_accessors()
{
  for (auto rpc: *this) {
    rpc->set_copy_container_accessors();
  }
}

const std::string Module::identifier()
{
  return this->module_name_;
}

Project::Project(LexicalScope *scope, std::vector<Module*> modules,
  std::vector<Include*> includes) :
  project_scope_(scope),
  project_modules_(modules),
  project_includes_(includes),
  last_tag_(0)
{
}

void Project::prepare_marshal()
{
  for (auto module: *this) {
    module->prepare_marshal();
  }
}

void Project::resolve_types()
{
  this->project_scope_->resolve_types();
  
  for (auto module: *this) {
    module->resolve_types();
  }
}

void Project::copy_types()
{
  for (auto module: *this) {
    module->copy_types();
  }
}

void Project::set_accessors()
{
  for (auto module: *this) {
    module->set_accessors();
  }
}

void Project::modify_specs()
{
  for (auto module: *this) {
    module->modify_specs();
  }
}

void Project::function_pointer_to_rpc()
{
  // right now project doesnt have free rpcs.
    std::cout<<FILENAME<<" invoking Project::function_pointer_to_rpc"<<std::endl;//for debug
  for (auto module: *this) {
    module->function_pointer_to_rpc();
  }
}

void Project::create_trampoline_structs()
{
  for (auto module: *this) {
    module->create_trampoline_structs();
  }
}

void Project::generate_function_tags()
{
  for (auto module: *this) {
    module->generate_function_tags(this);
  }
}

void Project::create_container_variables()
{
  for (auto module: *this) {
    module->create_container_variables();
  }
}

std::vector<Module*> Project::modules()
{
  return this->project_modules_;
}

std::vector<Include*> Project::includes()
{
  return this->project_includes_;
}

unsigned int Project::get_next_tag()
{
  this->last_tag_ += 1;
  return this->last_tag_;
}

void Project::initialize_types()
{
  for (auto module: *this) {
    module->initialize_types();
  }
}

void Project::set_copy_container_accessors()
{
  for (auto module: *this) {
    module->set_copy_container_accessors();
  }
}

Require::Require(const std::string& required_module_name) :
  required_module_name_(required_module_name)
{
}

Include::Include(bool relative, const std::string& path) :
  relative_(relative),
  path_(path)
{
}
