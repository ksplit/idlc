#include "lcd_ast.h"
#include "utils.h"
#include <iostream>

using namespace std;

FloatType::FloatType()
{
}

Marshal_type* FloatType::accept(MarshalPrepareVisitor *worker)
{
  return worker->visit(this);
}

CCSTTypeName* FloatType::accept(TypeNameVisitor *worker)
{
  return worker->visit(this);
}

CCSTStatement* FloatType::accept(TypeVisitor *worker, Variable *v)
{
  return worker->visit(this, v);
}

int FloatType::num()
{
  return FLOAT_TYPE;
}

const char* FloatType::name()
{
  return "float";
}

void FloatType::resolve_types(LexicalScope *ls)
{
  return;
}

void FloatType::create_trampoline_structs(LexicalScope *ls)
{
  return;
}

DoubleType::DoubleType()
{
}

Marshal_type* DoubleType::accept(MarshalPrepareVisitor *worker)
{
  return worker->visit(this);
}

CCSTTypeName* DoubleType::accept(TypeNameVisitor *worker)
{
  return worker->visit(this);
}

CCSTStatement* DoubleType::accept(TypeVisitor *worker, Variable *v)
{
  return worker->visit(this, v);
}

int DoubleType::num()
{
  return DOUBLE_TYPE;
}

const char* DoubleType::name()
{
  return "double";
}

void DoubleType::resolve_types(LexicalScope *ls)
{
  return;
}

void DoubleType::create_trampoline_structs(LexicalScope *ls)
{
  return;
}

BoolType::BoolType()
{ 
}

Marshal_type* BoolType::accept(MarshalPrepareVisitor *worker)
{
  return worker->visit(this);
}

CCSTTypeName* BoolType::accept(TypeNameVisitor *worker)
{
  return worker->visit(this);
}

CCSTStatement* BoolType::accept(TypeVisitor *worker, Variable *v)
{
  return worker->visit(this, v);
}

int BoolType::num()
{
  return BOOL_TYPE;
}

const char* BoolType::name()
{
  return "bool";
}

void BoolType::resolve_types(LexicalScope *ls)
{
  return;
}

void BoolType::create_trampoline_structs(LexicalScope *ls)
{
  return;
}

Function::Function(const char *id, ReturnVariable *return_var, std::vector<Parameter*> parameters, LexicalScope *ls)
{
  this->identifier_  = id;
  this->return_var_ = return_var;
  this->parameters_  = parameters;
  this->current_scope_ = ls;
}

Function::Function(const Function& other)
{
  // copy id
  this->identifier_ = new_name("", other.identifier_);

  // copy return var
  this->return_var_ = new ReturnVariable(*other.return_var_);
  // copy parameters
  std::vector<Parameter*> parameters_copy;
  for(std::vector<Parameter*>::const_iterator it = other.parameters_.begin(); it != other.parameters_.end(); it ++) {
    Parameter *original = *it;
    Parameter *copy = new Parameter(*original);
  }
  this->parameters_ = parameters_copy;

  // don't need to copy scope
  this->current_scope_ = other.current_scope_;
}

Marshal_type* Function::accept(MarshalPrepareVisitor *worker)
{
  return worker->visit(this);
}

CCSTTypeName* Function::accept(TypeNameVisitor *worker)
{
  return worker->visit(this);
}

CCSTStatement* Function::accept(TypeVisitor *worker, Variable *v)
{
  return worker->visit(this, v);
}

int Function::num()
{
  return FUNCTION_TYPE;
}

const char* Function::name()
{
  return this->identifier_;
}

void Function::resolve_types(LexicalScope *ls)
{
  return;
}

void Function::create_trampoline_structs(LexicalScope *ls)
{
  return;
}

Rpc* Function::to_rpc(ProjectionType *pt)
{
  // adding extra parameters here. but maybe depending on needs this could be done at parse time
  // and these extra parameters can be added to the Function.
  std::vector<Parameter*> new_parameters;
  // new_parameters.insert(new_parameters.end(), this->parameters_.begin(), this->parameters_.end());
  int err;

  Type *tramp_hidden_args = this->current_scope_->lookup("trampoline_hidden_args", &err);

  if (!tramp_hidden_args) {
    cerr << "Error: trampoline_hidden_args is not in scope\n";
  }

  new_parameters.push_back(new Parameter(tramp_hidden_args, "hidden_args", 1));


  Rpc* tmp = new Rpc(this->return_var_, this->identifier_, this->parameters_, this->current_scope_);
  tmp->set_function_pointer_defined(true);
  tmp->set_hidden_args(new_parameters);
  return tmp;
}

/* end */

UnresolvedType::UnresolvedType(const char *name)
{
  this->type_name_ = name;
}

UnresolvedType::UnresolvedType(const UnresolvedType& other)
{
  this->type_name_ = new_name("", other.type_name_);
}

CCSTTypeName* UnresolvedType::accept(TypeNameVisitor *worker)
{
  return worker->visit(this);
}

CCSTStatement* UnresolvedType::accept(TypeVisitor *worker, Variable *v)
{
  return worker->visit(this, v);
}

Marshal_type* UnresolvedType::accept(MarshalPrepareVisitor *worker)
{
  return worker->visit(this);
}

int UnresolvedType::num()
{
  return UNRESOLVED_TYPE;
}

const char* UnresolvedType::name()
{
  return this->type_name_;
}

void UnresolvedType::resolve_types(LexicalScope *ls)
{
  return;
}

void UnresolvedType::create_trampoline_structs(LexicalScope *ls)
{
  return;
}

Channel::Channel(const char *name, ChannelType type, Channel *host) :
  chName(name),
  chType(type),
  hostChannel(host)
{
  std::cout << "Channel Type : " << type << std::endl;
}

Channel::Channel(const Channel& other)
{
  
}

Marshal_type* Channel::accept(MarshalPrepareVisitor *worker)
{
  return worker->visit(this);
}
CCSTTypeName* Channel::accept(TypeNameVisitor *worker)
{
  return worker->visit(this);
}
CCSTStatement* Channel::accept(TypeVisitor *worker, Variable *v)
{
  return worker->visit(this, v);
}
const char* Channel::name()
{
  return this->chName.c_str();
}
int Channel::num()
{
  return CHANNEL_TYPE;
}

void Channel::resolve_types(LexicalScope *ls)
{
  return;
}

void Channel::create_trampoline_structs(LexicalScope *ls)
{
  return;
}

/* typedef type */


Typedef::Typedef(const char* id, const char* alias, Type* type)
{
  this->identifier_ = id;
  this->alias_ = alias;
  this->type_ = type; // need to allocate?
}

Typedef::Typedef(const Typedef& other)
{
  // copy id
  this->identifier_ = new_name("", other.identifier_);
  // copy alias
  this->alias_ = new_name("", other.alias_);;
  // copy Type
  this->type_ = other.type_->clone();
  // copy marshal info
  this->marshal_info_ = new_name("", other.marshal_info_);
}

Marshal_type* Typedef::accept(MarshalPrepareVisitor *worker)
{
  return worker->visit(this);
}

CCSTTypeName* Typedef::accept(TypeNameVisitor *worker)
{
  return worker->visit(this);
}

CCSTStatement* Typedef::accept(TypeVisitor *worker, Variable *v)
{
  return worker->visit(this, v);
}

const char* Typedef::alias()
{
  return this->alias_;
}

Type* Typedef::type() 
{
  return this->type_;
}

int Typedef::num()
{
  return TYPEDEF_TYPE;
}

const char* Typedef::name()
{
  return this->identifier_;
}

void Typedef::resolve_types(LexicalScope *ls)
{
  // TODO
  return;
}

void Typedef::create_trampoline_structs(LexicalScope *ls)
{
  return; //todo?
}

/* end */

/* void type */

VoidType::VoidType()
{
}

VoidType::VoidType(const VoidType& other)
{
  
}

Marshal_type* VoidType::accept(MarshalPrepareVisitor *worker)
{
  return worker->visit(this);
}

CCSTTypeName* VoidType::accept(TypeNameVisitor *worker)
{
  return worker->visit(this);
}

CCSTStatement* VoidType::accept(TypeVisitor *worker, Variable *v)
{
  return worker->visit(this, v);
}

int VoidType::num()
{
  return VOID_TYPE;
}

const char* VoidType::name()
{
  return "void";
}

void VoidType::resolve_types(LexicalScope *ls)
{
  return;
}

void VoidType::create_trampoline_structs(LexicalScope *ls)
{
  return;
}

/* end */

/* integer type */

IntegerType::IntegerType(PrimType type, bool un, int size)
{
  this->type_ = type;
  this->unsigned_ = un;
  this->size_ = size;
}

IntegerType::IntegerType(const IntegerType& other)
{
  this->unsigned_ = other.unsigned_;
  this->type_ = other.type_;
  this->size_ = other.size_;
}

Marshal_type* IntegerType::accept(MarshalPrepareVisitor *worker)
{
  return worker->visit(this);
}

CCSTTypeName* IntegerType::accept(TypeNameVisitor *worker)
{
  return worker->visit(this);
}

CCSTStatement* IntegerType::accept(TypeVisitor *worker, Variable *v)
{
  return worker->visit(this, v);
}

PrimType IntegerType::int_type()
{
  return this->type_;
}

bool IntegerType::is_unsigned()
{
  return unsigned_;
}

int IntegerType::num()
{
  return INTEGER_TYPE;
}

const char* IntegerType::name()
{
  std::cout << "todo integer type name function.\n";
  return "";
}

void IntegerType::resolve_types(LexicalScope *ls)
{
  return;
}

void IntegerType::create_trampoline_structs(LexicalScope *ls)
{
  return;
}

/* end */

/* projection type */
ProjectionType::ProjectionType()
{
}

ProjectionType::ProjectionType(const char* id, const char* real_type, std::vector<ProjectionField*> fields)
{
  this->id_ = id; 
  this->real_type_ = real_type; 
  this->fields_ = fields;
}

ProjectionType::ProjectionType(const char* id, const char* real_type, std::vector<ProjectionField*> fields, std::vector<ProjectionField*> channels)
{
  this->id_ = id; 
  this->real_type_ = real_type; 
  this->fields_ = fields;
  this->channels_ = channels;
}

ProjectionType::ProjectionType(const ProjectionType& other)
{
  // copy id
  this->id_ = new_name("", other.id_);
  // copy real_type_
  this->real_type_ = new_name("", other.real_type_);;
  // copy fields_
  std::vector<ProjectionField*> fields_copy;
  for(std::vector<ProjectionField*>::const_iterator it = other.fields_.begin(); it != other.fields_.end(); it ++) {
    ProjectionField *original = *it;
    ProjectionField *copy = new ProjectionField(*original);
    fields_copy.push_back(copy);
  }
  this->fields_ = fields_copy;
  // copy init_variables;
  std::vector<ProjectionField*> channels;
  for(std::vector<ProjectionField*>::const_iterator it = other.channels_.begin(); it != other.channels_.end(); it ++) {
    ProjectionField *original = *it;
    ProjectionField *copy = new ProjectionField(*original);
    channels.push_back(copy);
  }
  this->channels_ = channels;
}

Marshal_type* ProjectionType::accept(MarshalPrepareVisitor *worker)
{
  return worker->visit(this);
}

CCSTTypeName* ProjectionType::accept(TypeNameVisitor *worker)
{
  return worker->visit(this);
}

CCSTStatement* ProjectionType::accept(TypeVisitor *worker, Variable *v)
{
  return worker->visit(this, v);
}

// may never be used
const char* ProjectionType::real_type()
{
  return this->real_type_;
}

std::vector<ProjectionField*> ProjectionType::fields()
{
  return this->fields_;
}

int ProjectionType::num()
{
  return PROJECTION_TYPE;
}

const char* ProjectionType::name()
{
  return this->id_;
}

void ProjectionType::resolve_types(LexicalScope *ls)
{
  for(std::vector<ProjectionField*>::iterator it = this->fields_.begin(); it != this->fields_.end(); it ++) {
    ProjectionField *pf = (ProjectionField*) *it;
    pf->resolve_types(ls);
  }

  // channels
  for(std::vector<ProjectionField*>::iterator it = this->channels_.begin(); it != this->channels_.end(); it ++) {
    ProjectionField *pf = *it;
    pf->resolve_types(ls);
  }
}

void ProjectionType::create_trampoline_structs(LexicalScope *ls)
{
  for(std::vector<ProjectionField*>::iterator it = this->fields_.begin(); it != this->fields_.end(); it ++) {
    ProjectionField *pf = (ProjectionField*) *it;
    if (pf->type()->num() == FUNCTION_TYPE) { // function pointer
      Function *f = dynamic_cast<Function*>(pf->type());
      Assert(f != 0x0, "Error: dynamic cast to function type failed!\n");

      std::vector<ProjectionField*> trampoline_fields;
      int err;
      trampoline_fields.push_back(new ProjectionField(ls->lookup("void", &err), "struct_container", 1));
      trampoline_fields.push_back(new ProjectionField(ls->lookup("cspace", &err), "cspace", 1)); // dstore field
      trampoline_fields.push_back(new ProjectionField(ls->lookup("lcd_trampoline_handle", &err), "t_handle", 1)); // lcd_trampoline handle field
      trampoline_fields.push_back(new ProjectionField(ls->lookup("thc_channel", &err), "async_chnl", 1));
      trampoline_fields.push_back(new ProjectionField(ls->lookup("cptr_t", &err), "sync_ep", 0));

      const char* trampoline_struct_name = hidden_args_name(f->name());
      ls->insert(trampoline_struct_name, new ProjectionType(trampoline_struct_name, trampoline_struct_name, trampoline_fields));
    }
  }
}

ProjectionField* ProjectionType::get_field(const char *field_name)
{
  for(std::vector<ProjectionField*>::iterator it = this->fields_.begin(); it != this->fields_.end(); it ++) {
    ProjectionField *pf = *it;
    if (strcmp(field_name, pf->identifier()) == 0) {
      return pf;
    }
  }
  return 0x0;
}

void ProjectionType::initialize_type()
{
  for(std::vector<ProjectionField*>::iterator it = this->fields_.begin(); it != this->fields_.end(); it ++) {
    ProjectionField *pf = *it;
    pf->initialize_type();
  }
}

/* projection constructor type*/
ProjectionConstructorType::ProjectionConstructorType(const char* id, const char* real_type, std::vector<ProjectionField*> fields, std::vector<ProjectionField*> channel_fields, std::vector<ProjectionField*> channel_params)
{
  this->id_ = id;
  this->real_type_ = real_type;
  this->fields_ = fields;
  this->channels_ = channel_fields;
  
  for(std::vector<ProjectionField*>::iterator it = channel_params.begin(); it != channel_params.end(); it ++) {
    ProjectionField *tmp = *it;
    Variable *v = 0x0;
    this->channel_params_.push_back(std::make_pair(tmp, v));
  }
}

ProjectionConstructorType::ProjectionConstructorType(const ProjectionConstructorType& other)
{
  // copy id
  this->id_ = new_name("", other.id_);
  // copy real_type_
  this->real_type_ = new_name("", other.real_type_);
  // copy fields_
  std::vector<ProjectionField*> fields_copy;
  for(std::vector<ProjectionField*>::const_iterator it = other.fields_.begin(); it != other.fields_.end(); it ++) {
    ProjectionField *original = *it;
    ProjectionField *copy = new ProjectionField(*original);
    fields_copy.push_back(copy);
  }
  this->fields_ = fields_copy;
  // copy CHANNELS;
  std::vector<ProjectionField*> channels;
  for(std::vector<ProjectionField*>::const_iterator it = other.channels_.begin(); it != other.channels_.end(); it ++) {
    ProjectionField *original = *it;
    ProjectionField *copy = new ProjectionField(*original);
    channels.push_back(copy);
  }
  this->channels_ = channels;

  // copy channel params
  std::vector<std::pair<Variable*, Variable*> > channel_params;
  for(std::vector<std::pair<Variable*,Variable*> >::const_iterator it = other.channel_params_.begin(); it != other.channel_params_.end(); it ++) {
    std::pair<Variable*,Variable*> tmp = *it;
    std::pair<Variable*,Variable*> copy (tmp);
    channel_params.push_back(copy);
  }

  this->channel_params_ = channel_params;
}

int ProjectionConstructorType::num()
{
  return PROJECTION_CONSTRUCTOR_TYPE;
}

void ProjectionConstructorType::initialize(std::vector<Variable*> chans)
{
  std::cout << "calling initialize on a pct\n";
  // check that chans is correct length;
  if(chans.size() != this->channel_params_.size()) {
    Assert(1 == 0, "Error: required number of params and provided number do not match\n");
  }
  
  // set them
  for(int i = 0; i < chans.size(); i ++) {
    this->channel_params_.at(i).second = chans.at(i);
  }
 
}

void ProjectionConstructorType::resolve_types(LexicalScope *ls)
{
  for(std::vector<ProjectionField*>::iterator it = this->fields_.begin(); it != this->fields_.end(); it ++) {
    ProjectionField *pf = (ProjectionField*) *it;
    pf->resolve_types(ls);
  }

  // channels
  for(std::vector<ProjectionField*>::iterator it = this->channels_.begin(); it != this->channels_.end(); it ++) {
    ProjectionField *pf = *it;
    pf->resolve_types(ls);
  }

  // chan params
  for(std::vector<std::pair<Variable*,Variable*> >::iterator it = this->channel_params_.begin(); it != this->channel_params_.end(); it ++) {
    std::pair<Variable*,Variable*> tmp = *it;
    tmp.first->resolve_types(ls);
  }
}

/* initialize type */
/* this is meant to be used with projection constructor type to initialize them after 
 * type resolution and copying
 * this is another place holder type like UnresolvedType
 */

InitializeType::InitializeType(Type *type, std::vector<Variable*> init_vals)
{
  this->type_ = type;
  this->values_ = init_vals;

  // concerned initalize type vars wont see updates in constructor proj. from set accessors.
}

InitializeType::InitializeType(const InitializeType& other)
{
  this->values_ = other.values_;
  this->type_ = other.type_->clone();
}

Marshal_type* InitializeType::accept(MarshalPrepareVisitor *worker)
{
  return worker->visit(this);
}

CCSTTypeName* InitializeType::accept(TypeNameVisitor *worker)
{
  return worker->visit(this);
}

CCSTStatement* InitializeType::accept(TypeVisitor *worker, Variable *v)
{
  return worker->visit(this, v);
}

int InitializeType::num()
{
  return INITIALIZE_TYPE;
}

const char* InitializeType::name()
{
  return this->type_->name();
}

void InitializeType::resolve_types(LexicalScope *ls)
{
  for(std::vector<Variable*>::const_iterator it = this->values_.begin(); it != this->values_.end(); it ++) {
    Variable *v = *it;
    v->resolve_types(ls);
  }

  this->type_->resolve_types(ls);
}

void InitializeType::create_trampoline_structs(LexicalScope *ls)
{
  return;
}

void InitializeType::initialize()
{
  // check that type is a projectionconstructor
  Assert(this->type_->num() == PROJECTION_CONSTRUCTOR_TYPE, "Error: cannot initialize a non-projection constructor type\n");
  
  ProjectionConstructorType *pct = dynamic_cast<ProjectionConstructorType*>(this->type_);
  Assert(pct != 0x0, "Error: dynamic cast to projection constructor type failed\n");

  pct->initialize(this->values_);
}

/* end */

