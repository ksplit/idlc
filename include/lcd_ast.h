#ifndef LCD_AST_H
#define LCD_AST_H

#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <memory>
#include "marshal_op.h"
#include "symbol_table.h"
#include "ccst.h"

//ah note - this file contains the defintions of all the constructs that should be present in the output program

class MarshalPrepareVisitor;
class CCSTStatement;
class CCSTTypeName;
class Marshal_type;
class TypeVisitor;
class MarshalVisitor;
class TypeNameVisitor;
class AllocateTypeVisitor;
class Variable;
class Type;
class Parameter;
class Rpc;
class ProjectionType;
class Project;

enum PrimType {pt_char_t, pt_short_t, pt_int_t, pt_long_t, pt_longlong_t, pt_capability_t};
enum type_k {};

typedef enum {
  TYPEDEF_TYPE = 1,
  INTEGER_TYPE,
  PROJECTION_TYPE,
  VOID_TYPE,
  CHANNEL_TYPE,
  FUNCTION_TYPE,
  UNRESOLVED_TYPE,
  PROJECTION_CONSTRUCTOR_TYPE,
  INITIALIZE_TYPE,
  BOOL_TYPE,
  DOUBLE_TYPE,
  FLOAT_TYPE,
} types_t;

class ASTVisitor;
class Channel;



class LexicalScope 
{
  static LexicalScope *globalScope;
 public:
  LexicalScope *outer_scope_;
  std::map<std::string, Type*> type_definitions_;
  std::map<std::string, Variable*> variables_;
  std::map<std::pair<std::string, std::vector<Parameter*> >, Rpc*> rpc_definitions_; // rpc or function pointer. why do we keep this? 

  std::vector<std::string> identifiers_; // new
  std::vector<LexicalScope*> inner_scopes_;
  // List of channels under this scope
  std::vector<Channel*> channels;
  // Active channel for this scope
  Channel *activeChannel;

  LexicalScope();
  LexicalScope(LexicalScope *outer_scope);
  virtual ~LexicalScope() {}
  std::vector<Rpc*> rpc_in_scope();
  bool insert(Rpc *r);
  bool insert(Variable *v);

  bool insert_identifier(const std::string& id);
  bool contains_identifier(const std::string& id);

  Variable* lookup_variable(const std::string& sym, int* err);
  Type* lookup(const std::string& sym, int* err);
  bool insert(const std::string& sym, Type* type);
  bool contains(const std::string& symbol);
  virtual void set_outer_scope(LexicalScope *ls);
  void add_inner_scope(LexicalScope *ls);
  void add_inner_scopes(std::vector<LexicalScope*> scopes);
  std::map<std::string, Type*> type_definitions();
  std::vector<LexicalScope*> inner_scopes();
  LexicalScope* outer_scope();
  void resolve_types();
  void create_trampoline_structs();
  std::vector<Rpc*> function_pointer_to_rpc();
  std::map<std::string, Type*> all_type_definitions();
  std::map<std::string, Type*> all_types_outer();
  std::map<std::string, Type*> all_types_inner();

  Channel* getactiveChannel() const
  {
    return activeChannel;
  }

  void setactiveChannel(Channel* activeChannel)
  {
    this->activeChannel = activeChannel;
  }

  static LexicalScope* getGlobalScope();
};

class Type 
{
 public:
  virtual Type* clone() const = 0;
  virtual Marshal_type* accept(MarshalPrepareVisitor *worker) = 0;
  virtual CCSTTypeName* accept(TypeNameVisitor *worker) = 0;
  virtual CCSTStatement* accept(TypeVisitor *worker, Variable *v) = 0;
  virtual int num() = 0;
  virtual const std::string& name() const = 0;
  virtual void resolve_types(LexicalScope *ls) = 0;
  virtual void create_trampoline_structs(LexicalScope *ls) = 0;
  virtual ~Type() {}
};

class FloatType : public Type
{
  public:
  FloatType();
  std::string type_name;
  virtual Type* clone() const { return new FloatType(*this); }
  virtual Marshal_type* accept(MarshalPrepareVisitor *worker);
  virtual CCSTTypeName* accept(TypeNameVisitor *worker);
  virtual CCSTStatement* accept(TypeVisitor *worker, Variable *v);
  virtual int num();
  virtual const std::string& name() const;
  virtual void resolve_types(LexicalScope *ls);
  virtual void create_trampoline_structs(LexicalScope *ls);
};

class DoubleType : public Type
{
 public:
  DoubleType();
  std::string type_name;
  virtual Type* clone() const { return new DoubleType(*this); }
  virtual Marshal_type* accept(MarshalPrepareVisitor *worker);
  virtual CCSTTypeName* accept(TypeNameVisitor *worker);
  virtual CCSTStatement* accept(TypeVisitor *worker, Variable *v);
  virtual int num();
  virtual const std::string& name() const;
  virtual void resolve_types(LexicalScope *ls);
  virtual void create_trampoline_structs(LexicalScope *ls);
};

class BoolType : public Type
{
  public:
  BoolType();
  std::string type_name;
  virtual Type* clone() const { return new BoolType(*this); }
  virtual Marshal_type* accept(MarshalPrepareVisitor *worker);
  virtual CCSTTypeName* accept(TypeNameVisitor *worker);
  virtual CCSTStatement* accept(TypeVisitor *worker, Variable *v);
  virtual int num();
  virtual const std::string& name() const;
  virtual void resolve_types(LexicalScope *ls);
  virtual void create_trampoline_structs(LexicalScope *ls);
};

class InitializeType : public Type
{
 public:
  std::string type_name;
  Type* type_; // this is the type that WILL be initialized.  
  std::vector<Variable*> values_; // this is what will initialize the type
  InitializeType(Type *type, std::vector<Variable*> init_values);
  InitializeType(const InitializeType& other);
  virtual Type* clone() const { return new InitializeType(*this); }
  virtual Marshal_type* accept(MarshalPrepareVisitor *worker);
  virtual CCSTTypeName* accept(TypeNameVisitor *worker);
  virtual CCSTStatement* accept(TypeVisitor *worker, Variable *v);
  virtual int num();
  virtual const std::string& name() const;
  virtual void resolve_types(LexicalScope *ls);
  virtual void create_trampoline_structs(LexicalScope *ls);

  void initialize();
};

class UnresolvedType : public Type
{
 public:
  std::string type_name_;
  UnresolvedType(const std::string& type_name);
  UnresolvedType(const UnresolvedType& other);
  virtual Type* clone() const { return new UnresolvedType(*this); }
  virtual Marshal_type* accept(MarshalPrepareVisitor *worker);
  virtual CCSTTypeName* accept(TypeNameVisitor *worker); // need to add unresolved type to these visitors.
  virtual CCSTStatement* accept(TypeVisitor *worker, Variable *v);
  virtual int num();
  virtual const std::string& name() const;
  virtual void resolve_types(LexicalScope *ls);
  virtual void create_trampoline_structs(LexicalScope *ls);
};

class Variable 
{
 public:
  virtual Variable* clone() const = 0;
  virtual Type* type() = 0;
  virtual const std::string& identifier() const = 0;
  virtual void set_identifier(const std::string& id) = 0;
  virtual void set_accessor(Variable *v) = 0;
  virtual Variable* accessor() = 0;
  virtual void set_marshal_info(Marshal_type *mt) = 0;
  virtual Marshal_type* marshal_info() = 0;
  virtual int pointer_count() = 0;
  virtual void set_pointer_count(int pcount) = 0;
  virtual void prepare_marshal(MarshalPrepareVisitor *worker) = 0;
  virtual void resolve_types(LexicalScope *ls) = 0;
  virtual void create_container_variable(LexicalScope *ls) = 0;
  virtual void initialize_type() = 0;

  virtual void set_in(bool b) = 0;
  virtual void set_out(bool b) = 0;
  virtual void set_alloc_caller(bool b) = 0;
  virtual void set_alloc_callee(bool b) = 0;
  virtual void set_dealloc_caller(bool b) = 0;
  virtual void set_dealloc_callee(bool b) = 0;
  virtual void set_bind_caller(bool b) = 0;
  virtual void set_bind_callee(bool b) = 0;

  virtual bool in() = 0;
  virtual bool out() = 0;
  virtual bool alloc_caller() = 0;
  virtual bool alloc_callee() = 0;
  virtual bool dealloc_caller() = 0;
  virtual bool dealloc_callee() = 0;
  virtual bool bind_caller() = 0;
  virtual bool bind_callee() = 0;

  virtual Variable* container() = 0;
  virtual ~Variable() {}
};

class GlobalVariable : public Variable
{
 public:
  Type *type_;
  std::string id_;
  int pointer_count_;
  Marshal_type *marshal_info_;
  Variable *container_;
  Variable *accessor_;
  GlobalVariable(Type *type, const std::string& id, int pointer_count);
  GlobalVariable(const GlobalVariable& other);
  virtual Variable* clone() const { return new GlobalVariable(*this); }
  virtual Variable* container();
  virtual void prepare_marshal(MarshalPrepareVisitor *worker);
  virtual void resolve_types(LexicalScope *ls);
  virtual void create_container_variable(LexicalScope *ls);
  virtual Type* type();
  virtual const std::string& identifier() const;
  virtual void set_identifier(const std::string& id);
  virtual void set_accessor(Variable *v);
  virtual Variable* accessor();
  virtual void set_marshal_info(Marshal_type *mt);
  virtual Marshal_type* marshal_info();
  virtual int pointer_count();
  virtual void set_pointer_count(int pcount);
  virtual void initialize_type();

  virtual void set_in(bool b);
  virtual void set_out(bool b);
  virtual void set_alloc_caller(bool b);
  virtual void set_alloc_callee(bool b);
  virtual void set_dealloc_caller(bool b);
  virtual void set_dealloc_callee(bool b);
  virtual void set_bind_caller(bool b);
  virtual void set_bind_callee(bool b);

  virtual bool in();
  virtual bool out();
  virtual bool alloc_caller();
  virtual bool alloc_callee();
  virtual bool dealloc_caller();
  virtual bool dealloc_callee();
  virtual bool bind_caller();
  virtual bool bind_callee();
};

class Parameter : public Variable
{
 public:
  bool in_;
  bool out_;
  bool alloc_callee_;
  bool alloc_caller_;
  bool dealloc_callee_;
  bool dealloc_caller_;
  bool bind_callee_;
  bool bind_caller_;
  
  Type* type_;
  std::string name_;
  Marshal_type *marshal_info_;
  Variable *accessor_;
  int pointer_count_;
  Variable *container_;
  Parameter();
  Parameter(Type* type, const std::string& name, int pointer_count);
  Parameter(const Parameter& other);
  virtual Variable* clone() const { return new Parameter(*this); }
  virtual Variable* container();
  virtual void prepare_marshal(MarshalPrepareVisitor *worker);
  virtual void resolve_types(LexicalScope *ls);
  void modify_specs();
  virtual void create_container_variable(LexicalScope *ls);
  virtual Type* type();
  virtual void set_marshal_info(Marshal_type* mt);
  virtual Marshal_type* marshal_info(); 
  virtual const std::string& identifier() const;
  virtual void set_identifier(const std::string& id);
  virtual void set_accessor(Variable *v);
  virtual Variable* accessor();
  virtual int pointer_count();
  virtual void set_pointer_count(int pcount);
  virtual void initialize_type();
  
  virtual void set_in(bool b);
  virtual void set_out(bool b);
  virtual void set_alloc_caller(bool b);
  virtual void set_alloc_callee(bool b);
  virtual void set_dealloc_caller(bool b);
  virtual void set_dealloc_callee(bool b);  
  virtual void set_bind_caller(bool b);
  virtual void set_bind_callee(bool b);

  virtual bool in();
  virtual bool out();
  virtual bool alloc_caller();
  virtual bool alloc_callee();
  virtual bool dealloc_caller();
  virtual bool dealloc_callee();
  virtual bool bind_caller();
  virtual bool bind_callee();
};


// a parameter without a name
// add to variables.cpp
class FPParameter : public Parameter
{
 public:
  Type *type_;
  int pointer_count_;
  Marshal_type *marshal_info_;
  Variable *container_;
  FPParameter(Type *type, int pointer_count);
  FPParameter(const FPParameter& other);
  virtual ~FPParameter() {}
  virtual Variable* clone() const { return new FPParameter(*this); }
  virtual Variable* container();
  virtual Type* type();
  virtual const std::string& identifier() const;
  virtual void set_identifier(const std::string& id);
  virtual int pointer_count();
  virtual void set_pointer_count(int pcount);
  virtual void set_marshal_info(Marshal_type *mt);
  virtual Marshal_type* marshal_info();
  virtual void prepare_marshal(MarshalPrepareVisitor *worker);
  virtual void resolve_types(LexicalScope *ls);
  virtual void create_container_variable(LexicalScope *ls);
  virtual void initialize_type();

  virtual void set_in(bool b);
  virtual void set_out(bool b);
  virtual void set_alloc_caller(bool b);
  virtual void set_alloc_callee(bool b);
  virtual void set_dealloc_caller(bool b);
  virtual void set_dealloc_callee(bool b);
  virtual void set_bind_caller(bool b);
  virtual void set_bind_callee(bool b);

  virtual bool in();
  virtual bool out();
  virtual bool alloc_caller();
  virtual bool alloc_callee();
  virtual bool dealloc_caller();
  virtual bool dealloc_callee();
  virtual bool bind_caller();
  virtual bool bind_callee();
};

class ReturnVariable : public Variable
{
 public:
  std::string name_; // to be decided by a name space or something
  Type* type_;
  Marshal_type *marshal_info_;
  Variable* accessor_;
  int pointer_count_;
  Variable *container_;
  bool in_;
  bool out_;
  bool alloc_caller_;
  bool alloc_callee_;

  ReturnVariable();
  ReturnVariable(Type* return_type, int pointer_count, const std::string& id);
  ReturnVariable(const ReturnVariable& other);
  virtual ~ReturnVariable() {}
  virtual Variable* clone() const { return new ReturnVariable(*this); }
  virtual Variable *container();
  virtual void set_marshal_info(Marshal_type *mt);
  virtual Marshal_type* marshal_info();
  virtual void prepare_marshal(MarshalPrepareVisitor *worker);
  virtual void resolve_types(LexicalScope *ls);
  virtual void create_container_variable(LexicalScope *ls);
  virtual const std::string& identifier() const;
  virtual void set_identifier(const std::string& id);
  virtual Type* type();
  virtual void set_accessor(Variable *v);
  virtual Variable* accessor();
  virtual int pointer_count();
  virtual void set_pointer_count(int pcount);
  virtual void initialize_type();

  virtual void set_in(bool b);
  virtual void set_out(bool b);
  virtual void set_alloc_caller(bool b);
  virtual void set_alloc_callee(bool b);
  virtual void set_dealloc_caller(bool b);
  virtual void set_dealloc_callee(bool b);
  virtual void set_bind_caller(bool b);
  virtual void set_bind_callee(bool b);

  virtual bool in();
  virtual bool out();
  virtual bool alloc_caller();
  virtual bool alloc_callee();
  virtual bool dealloc_caller();
  virtual bool dealloc_callee();
  virtual bool bind_caller();
  virtual bool bind_callee();
};

class Function : public Type
{
  typedef std::vector<Parameter*>::iterator iterator;
  typedef std::vector<Parameter*>::const_iterator const_iterator;
 public:
  std::string identifier_;
  ReturnVariable *return_var_;
  std::vector<Parameter*> parameters_;
  LexicalScope *current_scope_;
  Function(const std::string& id, ReturnVariable *return_var, std::vector<Parameter*> parameters, LexicalScope *ls);
  Function(const Function& other);
  virtual ~Function() {}
  virtual Type* clone() const {return new Function(*this); }
  virtual Marshal_type* accept(MarshalPrepareVisitor *worker);
  virtual CCSTTypeName* accept(TypeNameVisitor *worker);
  virtual CCSTStatement* accept(TypeVisitor *worker, Variable *v);
  virtual int num();
  virtual const std::string& name() const;
  virtual void resolve_types(LexicalScope *ls);
  Rpc* to_rpc(ProjectionType *pt);
  virtual void create_trampoline_structs(LexicalScope *ls);
  iterator begin() { return parameters_.begin(); }
  iterator end() { return parameters_.end(); }
  const_iterator begin() const { return parameters_.begin(); }
  const_iterator end() const { return parameters_.end(); }
};
 
class Typedef : public Type
{
 public:
  Type* type_;
  std::string alias_;
  std::string marshal_info_;
  std::string identifier_;
  
  Typedef(const std::string& id, const std::string& alias, Type* type);
  Typedef(const Typedef& other);
  virtual ~Typedef() {}
  virtual Type* clone() const { return new Typedef(*this); }
  virtual Marshal_type* accept(MarshalPrepareVisitor *worker);
  virtual CCSTTypeName* accept(TypeNameVisitor *worker);
  virtual CCSTStatement* accept(TypeVisitor *worker, Variable *v);
  virtual const std::string& name() const;
  Type* type();
  const std::string& alias() const;
  virtual int num();
  virtual void resolve_types(LexicalScope *ls);
  // virtual void marshal();
  virtual void create_trampoline_structs(LexicalScope *ls);
};

class Channel : public Type 
{
 public:
  enum ChannelType {
    AsyncChannel = 0,
    SyncChannel,
    Unknown
  };

  Channel *hostChannel;
  ChannelType chType;
  std::string chName;

  Channel() :hostChannel(NULL), chType(Unknown) {}
  Channel(const std::string& name, ChannelType, Channel*);
  Channel(const Channel& other);
  virtual Type* clone() const { return new Channel(*this); }
  virtual Marshal_type* accept(MarshalPrepareVisitor *worker);
  virtual CCSTTypeName* accept(TypeNameVisitor *worker);
  virtual CCSTStatement* accept(TypeVisitor *worker, Variable *v);
  virtual const std::string& name() const;
  virtual int num();
  virtual void resolve_types(LexicalScope *ls);
  virtual void create_trampoline_structs(LexicalScope *ls);

  void setChannelType(ChannelType type) {
    this->chType = type;
  }

  const ChannelType getChannelType() const {
    return this->chType;
  }

  Channel* gethostChannel() const
  {
    return hostChannel;
  }

  void sethostChannel(Channel* hostChannel)
  {
    this->hostChannel = hostChannel;
  }
};

class VoidType : public Type
{
 public:
  std::string type_name;
  VoidType();
  VoidType(const VoidType& other);
  virtual Type* clone() const { return new VoidType(*this); }
  virtual Marshal_type* accept(MarshalPrepareVisitor *worker);
  virtual CCSTTypeName* accept(TypeNameVisitor *worker);
  virtual CCSTStatement* accept(TypeVisitor *worker, Variable *v);
  virtual const std::string& name() const;
  virtual int num();
  virtual void resolve_types(LexicalScope *ls);
  virtual void create_trampoline_structs(LexicalScope *ls);
};

class IntegerType : public Type
{
 public:
  std::string type_name;
  bool unsigned_;
  PrimType type_;
  int size_;
  IntegerType(PrimType type, bool un, int size);
  IntegerType(const IntegerType& other);
  virtual Type* clone() const { return new IntegerType(*this); }
  virtual Marshal_type* accept(MarshalPrepareVisitor *worker);
  virtual CCSTTypeName* accept(TypeNameVisitor *worker);
  virtual CCSTStatement* accept(TypeVisitor *worker, Variable *v);
  virtual const std::string& name() const;
  PrimType int_type();
  bool is_unsigned();
  virtual int num();
  virtual void resolve_types(LexicalScope *ls);
  ~IntegerType(){ std::cout << "inttype destructor\n"; }
  virtual void create_trampoline_structs(LexicalScope *ls);
};

class ProjectionField : public Variable //?
{
 public:
  bool in_;
  bool out_;
  bool alloc_callee_;
  bool alloc_caller_;
  bool dealloc_callee_;
  bool dealloc_caller_;
  bool bind_callee_;
  bool bind_caller_;

  Type* type_;
  std::string field_name_;
  Variable *accessor_; // 
  int pointer_count_;
  Marshal_type *marshal_info_;
  Variable *container_;
  ProjectionField(Type* field_type, const std::string& field_name, int pointer_count);
  virtual ~ProjectionField() {}
  ProjectionField(const ProjectionField& other);
  virtual Variable* clone() const { return new ProjectionField(*this); }
  virtual Variable *container();
  virtual Type* type();
  virtual const std::string& identifier() const;
  virtual void set_identifier(const std::string& id);
  virtual void set_accessor(Variable *v);
  virtual Variable* accessor();
  virtual void set_marshal_info(Marshal_type *mt); // add to .cpp file
  virtual Marshal_type* marshal_info(); // make sure all variables have
  virtual void prepare_marshal(MarshalPrepareVisitor *worker);
  virtual void resolve_types(LexicalScope *ls);
  virtual void create_container_variable(LexicalScope *ls);
  virtual int pointer_count();
  virtual void set_pointer_count(int pcount);
  virtual void initialize_type();

  virtual void set_in(bool b);
  virtual void set_out(bool b);
  virtual void set_alloc_caller(bool b);
  virtual void set_alloc_callee(bool b);
  virtual void set_dealloc_caller(bool b);
  virtual void set_dealloc_callee(bool b);
  virtual void set_bind_caller(bool b);
  virtual void set_bind_callee(bool b);

  virtual bool in();
  virtual bool out();
  virtual bool alloc_caller();
  virtual bool alloc_callee();
  virtual bool dealloc_caller();
  virtual bool dealloc_callee();
  virtual bool bind_caller();
  virtual bool bind_callee();

};

class ProjectionType : public Type // complex type
{
 public:
  std::vector<ProjectionField*> channels_;
  std::string id_;
  std::string real_type_;
  std::vector<ProjectionField*> fields_;
  typedef std::vector<ProjectionField*>::iterator iterator;
  typedef std::vector<ProjectionField*>::const_iterator const_iterator;

  ProjectionType();
  ProjectionType(const std::string& id, const std::string& real_type, std::vector<ProjectionField*> fields, std::vector<ProjectionField*> channels);
  ProjectionType(const std::string& id, const std::string& real_type, std::vector<ProjectionField*> fields);
  ProjectionType(const ProjectionType& other);
  virtual Type* clone() const { return new ProjectionType(*this); }
  virtual Marshal_type* accept(MarshalPrepareVisitor *worker);
  virtual CCSTTypeName* accept(TypeNameVisitor *worker);
  virtual CCSTStatement* accept(TypeVisitor *worker, Variable *v);
  virtual const std::string& name() const;
  const std::string real_type() const;
  std::vector<ProjectionField*> fields();
  virtual int num();
  virtual void resolve_types(LexicalScope *ls);
  ~ProjectionType(){ std::cout << "projection type destructor\n"; }
  virtual void create_trampoline_structs(LexicalScope *ls);
  ProjectionField* get_field(const std::string& field_name);
  void initialize_type();
  iterator begin() { return fields_.begin(); }
  iterator end() { return fields_.end(); }
  const_iterator begin() const { return fields_.begin(); }
  const_iterator end() const { return fields_.end(); }
};

class ProjectionConstructorType : public ProjectionType 
{
 public:
  std::vector<std::pair<Variable*, Variable*> > channel_params_;
  ProjectionConstructorType(const std::string& id, const std::string& real_type, std::vector<ProjectionField*> fields, std::vector<ProjectionField*> channel_fields, std::vector<ProjectionField*> channel_params);
  ProjectionConstructorType(const ProjectionConstructorType& other);
  virtual int num();
  virtual Type* clone() const { return new ProjectionConstructorType(*this); }
  virtual void resolve_types(LexicalScope *ls);
  void initialize(std::vector<Variable*> chans);
};

class Rpc 
{
  unsigned int tag_;
  SymbolTable *symbol_table_;
  ReturnVariable *explicit_return_;
  LexicalScope *current_scope_;
  /* -------------- */
  std::string name_;
  std::string enum_str;
  std::vector<Parameter* > parameters_;
  
  bool function_pointer_defined_;
  std::vector<Variable*> marshal_projection_parameters(ProjectionType *pt, const std::string& direction);
  typedef std::vector<Parameter*>::iterator iterator;

 public:
  std::vector<Parameter*> hidden_args_;
  Rpc(ReturnVariable *return_var, const std::string& name, std::vector<Parameter* > parameters, LexicalScope *current_scope);
  void copy_types();
  void modify_specs();
  unsigned int tag();
  void set_tag(unsigned int t);
  void set_function_pointer_defined(bool b);
  void set_hidden_args(std::vector<Parameter*> hidden_args);
  bool function_pointer_defined();
  const std::string name() const;
  const std::string& enum_name() const;
  const std::string callee_name() const;
  std::vector<Parameter*> parameters();
  ReturnVariable *return_variable();
  SymbolTable* symbol_table();
  void prepare_marshal();
  void resolve_types();
  void create_trampoline_structs();
  void create_container_variables();
  void set_accessors();
  void initialize_types();
  void set_copy_container_accessors();
  LexicalScope *current_scope();
  iterator begin() { return parameters_.begin(); }
  iterator end() { return parameters_.end(); }

  const LexicalScope* getcurrentscope() const
  {
    return current_scope_;
  }
};

class Module 
{
  // const std::string& verbatim_;
  std::string module_name_;
  LexicalScope *module_scope_;
  std::vector<GlobalVariable*> channels_;
   // create these from the channels in the constructor.
  std::vector<Rpc*> rpc_definitions_;
  typedef std::vector<Rpc*>::iterator iterator;

 public:
  std::vector<GlobalVariable*> cspaces_;
  GlobalVariable *channel_group;
  Module(const std::string& id, std::vector<Rpc*> rpc_definitions, std::vector<GlobalVariable*> globals, LexicalScope *ls);
  std::vector<Rpc*> rpc_definitions();  
  std::vector<GlobalVariable*> channels();
  LexicalScope *module_scope();
  void prepare_marshal();
  void resolve_types();
  void modify_specs();
  void function_pointer_to_rpc();
  void create_trampoline_structs();
  void generate_function_tags(Project *p);
  void create_container_variables();
  void copy_types();
  void set_accessors();
  void initialize_types();
  void set_copy_container_accessors();
  const std::string identifier();
  iterator begin() { return rpc_definitions_.begin(); }
  iterator end() { return rpc_definitions_.end(); }
};

class Include 
{
  bool relative_; // true if "" false for <>
  std::string path_;
 public:
  Include(bool relative, const std::string& path);
  std::string get_path() {
	  return this->path_;
  }
  bool is_relative() {
	  return this->relative_;
  }
};

class Project
{
  LexicalScope *project_scope_;
  std::vector<Module*> project_modules_;
  std::vector<Include*> project_includes_;
  unsigned int last_tag_;
  typedef std::vector<Module*>::iterator iterator;
  
 public:
  Project(LexicalScope *scope, std::vector<Module*> modules, std::vector<Include*> includes);
  void prepare_marshal();
  void resolve_types();
  void function_pointer_to_rpc();
  void create_trampoline_structs();
  void generate_function_tags();
  void create_container_variables();
  void copy_types();
  void set_accessors();
  void modify_specs();
  void initialize_types();
  void set_copy_container_accessors();
  std::vector<Module*> modules();
  unsigned int get_next_tag();
  std::vector<Include*> includes();
  iterator begin() { return project_modules_.begin(); }
  iterator end() { return project_modules_.end(); }
};

class TypeNameVisitor // generates CCSTTypeName for each type.
{
 public:
  CCSTTypeName* visit(UnresolvedType *ut);
  CCSTTypeName* visit(Typedef *td);
  CCSTTypeName* visit(VoidType *vt);
  CCSTTypeName* visit(IntegerType *it);
  CCSTTypeName* visit(ProjectionType *pt);
  CCSTTypeName* visit(Function *fp);
  CCSTTypeName* visit(Channel *c);
  CCSTTypeName* visit(ProjectionConstructorType *pct);
  CCSTTypeName* visit(InitializeType *it);
  CCSTTypeName* visit(BoolType *bt);
  CCSTTypeName* visit(DoubleType *dt);
  CCSTTypeName* visit(FloatType *ft);
};

class TypeVisitor
{
 public:
  virtual CCSTStatement* visit(UnresolvedType *ut, Variable *v) = 0;
  virtual CCSTStatement* visit(Function *fp, Variable *v) = 0;
  virtual CCSTStatement* visit(Typedef *td, Variable *v) = 0;
  virtual CCSTStatement* visit(VoidType *vt, Variable *v) = 0;
  virtual CCSTStatement* visit(IntegerType *it, Variable *v) = 0;
  virtual CCSTStatement* visit(ProjectionType *pt, Variable *v) = 0;
  virtual CCSTStatement* visit(Channel *c, Variable *v) = 0;
  virtual CCSTStatement* visit(ProjectionConstructorType *pct, Variable *v) = 0;
  virtual CCSTStatement* visit(InitializeType *it, Variable *v) = 0;
  virtual CCSTStatement* visit(BoolType *bt, Variable *v) = 0;
  virtual CCSTStatement* visit(DoubleType *dt, Variable *v) = 0;
  virtual CCSTStatement* visit(FloatType *ft, Variable *v) = 0;
  virtual ~TypeVisitor() {}
};

class AllocateTypeVisitor : public TypeVisitor    
{
 public:
  AllocateTypeVisitor();
  virtual CCSTStatement* visit(UnresolvedType *ut, Variable *v);
  virtual CCSTStatement* visit(Function *fp, Variable *v);
  virtual CCSTStatement* visit(Typedef *td, Variable *v);
  virtual CCSTStatement* visit(VoidType *vt, Variable *v);
  virtual CCSTStatement* visit(IntegerType *it, Variable *v);
  virtual CCSTStatement* visit(ProjectionType *pt, Variable *v);
  virtual CCSTStatement* visit(Channel *c, Variable *v);
  virtual CCSTStatement* visit(ProjectionConstructorType *pct, Variable *v);
  virtual CCSTStatement* visit(InitializeType *it, Variable *v);
  virtual CCSTStatement* visit(BoolType *bt, Variable *v);
  virtual CCSTStatement* visit(DoubleType *dt, Variable *v);
  virtual CCSTStatement* visit(FloatType *ft, Variable *v);
  virtual ~AllocateTypeVisitor() {}
};

#endif
