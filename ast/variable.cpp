#include "lcd_ast.h"
#include "code_gen.h"

/* global variable */

GlobalVariable::GlobalVariable(Type *type, const std::string& id, int pointer_count) :
  type_(type),
  id_(id),
  pointer_count_(pointer_count),
  marshal_info_(NULL),
  container_(NULL),
  accessor_(NULL)
{
}

GlobalVariable::GlobalVariable(const GlobalVariable& other)
{
  // copy type
  this->type_ = other.type_->clone();
  this->pointer_count_ = other.pointer_count_;
  this->accessor_ = other.accessor_;
  
  // copy id
  this->id_ = other.id_;
  // copy marshal info
  if (other.marshal_info_ != 0x0) {
    this->marshal_info_ = other.marshal_info_->clone(); // copy for real?
  } else {
    this->marshal_info_ = 0x0;
  }
  
  // copy container
  if (other.container_ != 0x0) {
    this->container_ = other.container_->clone();
  } else {
    this->container_ = 0x0;
  }
}

void GlobalVariable::prepare_marshal(MarshalPrepareVisitor *worker)
{
  if (this->container_ != 0x0) {
    this->container_->prepare_marshal(worker);
  }

  this->marshal_info_ = this->type_->accept(worker);
}

Type* GlobalVariable::type()
{
  return this->type_;
}

const std::string& GlobalVariable::identifier() const
{
  return this->id_;
}

void GlobalVariable::set_identifier(const std::string& id)
{
  this->id_ = id;
}

void GlobalVariable::set_accessor(Variable *v)
{
  this->accessor_ = v;
  
  if(this->container_ != 0x0) {
    this->container_->set_accessor(0x0);
  }

  if(this->type_->num() == PROJECTION_TYPE || this->type_->num() == PROJECTION_CONSTRUCTOR_TYPE) {
    ProjectionType *pt = dynamic_cast<ProjectionType*>(this->type_);
    Assert(pt != 0x0, "Error: dynamic cast to projection type failed\n");

    for (auto pf : *pt) {
      pf->set_accessor(this);
    }
  }
}

Variable* GlobalVariable::accessor()
{
  std::cout << "Error this operation is not allowed\n";
  return 0x0;
}

void GlobalVariable::set_marshal_info(Marshal_type *mt)
{
  this->marshal_info_ = mt;
}

Marshal_type* GlobalVariable::marshal_info()
{
  return this->marshal_info_;
}

int GlobalVariable::pointer_count()
{
  return this->pointer_count_;
}

void GlobalVariable::set_pointer_count(int pcount)
{
  this->pointer_count_ = pcount;
}

void GlobalVariable::resolve_types(LexicalScope *ls)
{
  // need to rewrite to account for initializetype
//  Type *last = this->type_;
//  Type *tmp = this->type_;
  
  if(this->type_->num() == INITIALIZE_TYPE) {
    Type *tmp = this->type_;
    InitializeType *it = 0x0;
    while(tmp->num() == INITIALIZE_TYPE) {
      it = dynamic_cast<InitializeType*>(tmp);
      Assert(it != 0x0, "Error: dynamic cast to initialize type failed\n");
      tmp = it->type_;
    }
    
    if(tmp->num() != UNRESOLVED_TYPE) {
      return;
    }

    int err;
    Type *t = ls->lookup(tmp->name(), &err);
    if(t == 0x0) {
      std::cout << "Error: could not resolve type " <<  tmp->name() << std::endl;
      return;
    }

    it->type_ = t;
    return;

  } else {

    // check if unresolved
    if(this->type_->num() != UNRESOLVED_TYPE) {
      return;
    }
    
    int err;
    Type *t = ls->lookup(this->type_->name(), &err);
    if(t == 0x0) {
      std::cout << "Error: could not resolve type " <<  this->type_->name() << std::endl;
      return;
    } 
    
    // and set
    this->type_ = t;
    return;
  }
}

void GlobalVariable::initialize_type()
{
  if ( this->type_->num() == INITIALIZE_TYPE ) {
    InitializeType *it = dynamic_cast<InitializeType*>(this->type_);
    Assert(it != 0x0, "Error: dynamic cast to Initialize type failed\n");
    
    it->initialize();
    this->type_ = it->type_;
  } else if (this->type_->num() == PROJECTION_TYPE || this->type_->num() == PROJECTION_CONSTRUCTOR_TYPE) {
    ProjectionType *pt = dynamic_cast<ProjectionType*>(this->type_);
    Assert(pt != 0x0, "Error: dynamic cast to Projection type failed\n");

    pt->initialize_type();
  }
}

void GlobalVariable::create_container_variable(LexicalScope *ls)
{
  if(this->pointer_count() <= 0 || (this->type_->num() != PROJECTION_TYPE && this->type_->num() != PROJECTION_CONSTRUCTOR_TYPE && this->type_->num() != INITIALIZE_TYPE)) {
    return;
  }
  Type *tmp = this->type_;

  if(this->type_->num() == INITIALIZE_TYPE) { // initialize type
    while(tmp->num() == INITIALIZE_TYPE) {
      InitializeType *it = dynamic_cast<InitializeType*>(tmp);
      Assert(it != 0x0, "Error: dynamic cast to initialize type failed\n");

      tmp = it->type_;
    }

    if(tmp->num() != PROJECTION_TYPE && tmp->num() != PROJECTION_CONSTRUCTOR_TYPE) {
      return;
    }
  }
  
  // lookup in scope the container for its type. 
  int err;
  Type *container_t = ls->lookup(container_name(tmp->name()), &err);
  Assert(container_t != 0x0, "Error: could not find container in scope\n");

  ProjectionType *container = dynamic_cast<ProjectionType*>(container_t);
  Assert(container != 0x0, "Error: could not dynamically cast to projection\n");

  // really need to make variable non abstract and get rid of unnecessary variables.

  const std::string& name = container_name(append_strings(std::string("_"), construct_list_vars(this)));
  Parameter *container_var = new Parameter(container, name, 1);
  container_var->set_in(this->in());
  container_var->set_out(this->out());

  // save. 
  this->container_ = container_var;
  
  ProjectionType *pt = dynamic_cast<ProjectionType*>(tmp);
  Assert(pt != 0x0, "Error: dynamic cast to projection type failed\n");

  for (auto pf : *pt) {
    pf->create_container_variable(ls);
  }
}

void GlobalVariable::set_in(bool b)
{
  std::cout << "this operation is not allowed\n";
}

void GlobalVariable::set_out(bool b)
{
  std::cout << "this operation is not allowed\n";
}

void GlobalVariable::set_alloc_caller(bool b)
{
  std::cout << "this operation is not allowed\n";
}

void GlobalVariable::set_alloc_callee(bool b)
{
  std::cout << "this operation is not allowed\n";
}

void GlobalVariable::set_dealloc_caller(bool b)
{
  std::cout << "this operation is not allowed\n";
}

void GlobalVariable::set_dealloc_callee(bool b)
{
  std::cout << "this operation is not allowed\n";
}

bool GlobalVariable::in()
{
  std::cout << "this operation is not allowed\n";
  return false;
}

bool GlobalVariable::out()
{
  std::cout << "this operation is not allowed\n";
  return false;
}

bool GlobalVariable::alloc_caller()
{
  std::cout << "this operation is not allowed\n";
  return false;
}
bool GlobalVariable::alloc_callee()
{
  std::cout << "this operation is not allowed\n";
  return false;
}

bool GlobalVariable::dealloc_caller()
{
  std::cout << "this operation is not allowed\n";
  return false;
}

bool GlobalVariable::dealloc_callee()
{
  std::cout << "this operation is not allowed\n";
  return false;
}

Variable* GlobalVariable::container()
{
  return this->container_;
}

void GlobalVariable::set_bind_caller(bool b)
{
  std::cout << "this operation is not allowed\n";
}

void GlobalVariable::set_bind_callee(bool b)
{
  std::cout << "this operation is not allowed\n";
}

bool GlobalVariable::bind_caller()
{
  std::cout << "this operation is not allowed\n";
  return false;
}

bool GlobalVariable::bind_callee()
{
  std::cout << "this operation is not allowed\n";
  return false;
}

// probably more functions needed

/* end */

/* parameter */
Parameter::Parameter() :
  in_(false),
  out_(false),
  alloc_callee_(false),
  alloc_caller_(false),
  dealloc_callee_(false),
  dealloc_caller_(false),
  bind_callee_(false),
  bind_caller_(false),
  type_(NULL),
  name_(NULL),
  marshal_info_(NULL),
  accessor_(NULL),
  pointer_count_(0),
  container_(NULL)
{
}

Parameter::Parameter(Type* type, const std::string& name, int pointer_count) :
  in_(false),
  out_(false),
  alloc_callee_(false),
  alloc_caller_(false),
  dealloc_callee_(false),
  dealloc_caller_(false),
  bind_callee_(false),
  bind_caller_(false),
  type_(type),
  name_(name),
  marshal_info_(NULL),
  accessor_(NULL),
  pointer_count_(pointer_count),
  container_(NULL)
{
}

Parameter::Parameter(const Parameter& other) :
  marshal_info_(NULL),
  accessor_(NULL)
{
  this->type_ = other.type_->clone();

  // clone name
  this->name_ = other.name_;
  this->pointer_count_ = other.pointer_count_;
  this->in_ = other.in_;
  this->out_ = other.out_;
  this->alloc_callee_ = other.alloc_callee_;
  this->alloc_caller_ = other.alloc_caller_;
  this->dealloc_callee_ = other.dealloc_callee_;
  this->dealloc_caller_ = other.dealloc_caller_;
  this->bind_callee_ = other.bind_callee_;
  this->bind_caller_ = other.bind_caller_;

  if(other.container_ != 0x0) {
    this->container_ = other.container_->clone();
  } else {
    this->container_ = 0x0;
  }
}

void Parameter::create_container_variable(LexicalScope *ls)
{
  // FIXME: some conditions in this monstrous if are meaningless. clean it up
  if (this->pointer_count() <= 0 ||
        (this->type_->num() != PROJECTION_TYPE &&
            this->type_->num() != PROJECTION_CONSTRUCTOR_TYPE &&
              this->type_->num() != INITIALIZE_TYPE &&
                this->type_->num() != FUNCTION_TYPE) ||
        (!this->bind_caller() && !this->bind_callee() &&
          !this->alloc_caller() && !this->alloc_callee() && !this->dealloc_callee())) {
    return;
  }
  Type *tmp = this->type_;

  if(this->type_->num() == INITIALIZE_TYPE) { // initialize type
    while(tmp->num() == INITIALIZE_TYPE) {
      InitializeType *it = dynamic_cast<InitializeType*>(tmp);
      Assert(it != 0x0, "Error: dynamic cast to initialize type failed\n");

      tmp = it->type_;
    }

    if(tmp->num() != PROJECTION_TYPE && tmp->num() != PROJECTION_CONSTRUCTOR_TYPE) {
      return;
    }
  }
  
  // lookup in scope the container for its type. 
  int err;
  Type *container_t = ls->lookup(container_name(tmp->name()), &err);
  std::cout << "Just looked up container " <<  container_name(tmp->name()) << std::endl;
  Assert(container_t != 0x0, "Error: could not find container in scope\n");

  if (this->type_->num() == PROJECTION_TYPE) {
  ProjectionType *container = dynamic_cast<ProjectionType*>(container_t);
  Assert(container != 0x0, "Error: could not dynamically cast to projection\n");

  // really need to make variable non abstract and get rid of unnecessary variables.

  const std::string& name = container_name(append_strings("_", construct_list_vars(this)));

  Parameter *container_var = new Parameter(container, name, 1);
  container_var->set_in(this->in());
  container_var->set_out(this->out());
  container_var->set_bind_callee(this->bind_callee());
  container_var->set_bind_caller(this->bind_caller());
  container_var->set_dealloc_callee(this->dealloc_callee());
  container_var->set_dealloc_caller(this->dealloc_caller());

  // save. 
  this->container_ = container_var;

  // recurse

  ProjectionType *pt = dynamic_cast<ProjectionType*>(tmp);
  Assert(pt != 0x0, "Error: dynamic cast to projection type failed\n");
  
  for (auto pf : *pt) {
    pf->create_container_variable(ls);
  }

  } else if (this->type_->num() == FUNCTION_TYPE) {
    ProjectionType *container = dynamic_cast<ProjectionType*>(container_t);
    Assert(container != 0x0, "Error: could not dynamically cast to projection\n");
    const std::string& name = container_name(append_strings("_", construct_list_vars(this)));

    Parameter *container_var = new Parameter(container, name, 1);
    container_var->set_in(this->in());
    container_var->set_out(this->out());

    // save.
    this->container_ = container_var;
  }
}

Variable* Parameter::container()
{
  return this->container_;
}

void Parameter::prepare_marshal(MarshalPrepareVisitor *worker)
{
  // if 
  if (this->container_ != 0x0) {
    this->container_->prepare_marshal(worker);
  }
  
  this->marshal_info_ = this->type_->accept(worker);
}

const std::string& Parameter::identifier() const
{
  return this->name_;
}

void Parameter::set_identifier(const std::string& id)
{
  this->name_ = id;
}

Type* Parameter::type()
{
  return this->type_;
}

int Parameter::pointer_count()
{
  return this->pointer_count_;
}

void Parameter::set_pointer_count(int pcount)
{
  this->pointer_count_ = pcount;
}

void Parameter::set_accessor(Variable *v)
{
  this->accessor_ = v;
  if(this->container_ != 0x0) {
    this->container_->set_accessor(0x0);
  }
  
  if(this->type_->num() == PROJECTION_TYPE || this->type_->num() == PROJECTION_CONSTRUCTOR_TYPE) {
    ProjectionType *pt = dynamic_cast<ProjectionType*>(this->type_);
    Assert(pt != 0x0, "Error: dynamic cast to projection type failed\n");
    
    for (auto pf : *pt) {
      pf->set_accessor(this);
    }
  }
}

/// TODO: Should the other conditions be handled?
void Parameter::modify_specs()
{
  if (this->container()) {
    /// If the spec is bind or dealloc
    if ((this->bind_caller() && this->bind_callee())
      || this->dealloc_callee()) {
      auto *container = dynamic_cast<ProjectionType*>(this->container()->type());
      auto *other_ref = container->get_field("other_ref");

      /// we should be marshalling other_ref
      other_ref->set_in(true);
    }
  }
}

Variable* Parameter::accessor()
{
  return this->accessor_;
}

void Parameter::set_marshal_info(Marshal_type* mt)
{
  this->marshal_info_ = mt;
}

Marshal_type* Parameter::marshal_info()
{
  return this->marshal_info_;
}

void Parameter::resolve_types(LexicalScope *ls)
{
   // need to rewrite to account for initializetype
//  Type *last = this->type_;
//  Type *tmp = this->type_;
  
  if(this->type_->num() == INITIALIZE_TYPE) {
    Type *tmp = this->type_;
    InitializeType *it = 0x0;
    while(tmp->num() == INITIALIZE_TYPE) {
      it = dynamic_cast<InitializeType*>(tmp);
      Assert(it != 0x0, "Error: dynamic cast to initialize type failed\n");
      tmp = it->type_;
    }
    
    if(tmp->num() != UNRESOLVED_TYPE) {
      return;
    }

    int err;
    Type *t = ls->lookup(tmp->name(), &err);
    if(t == 0x0) {
      std::cout << "Error: could not resolve type " <<  tmp->name() << std::endl;
      return;
    }

    it->type_ = t;
    return;

  } else {

    // check if unresolved
    if(this->type_->num() != UNRESOLVED_TYPE) {
      return;
    }
    
    int err;
    Type *t = ls->lookup(this->type_->name(), &err);
    if(t == 0x0) {
      std::cout << "Error: could not resolve type " <<  this->type_->name() << std::endl;
      return;
    } 
    
    // and set
    this->type_ = t;
    return;
  }
}

void Parameter::initialize_type()
{
  if ( this->type_->num() == INITIALIZE_TYPE ) {
    InitializeType *it = dynamic_cast<InitializeType*>(this->type_);
    Assert(it != 0x0, "Error: dynamic cast to Initialize type failed\n");
    
    it->initialize();
    this->type_ = it->type_;
  } else if (this->type_->num() == PROJECTION_TYPE || this->type_->num() == PROJECTION_CONSTRUCTOR_TYPE) {
    ProjectionType *pt = dynamic_cast<ProjectionType*>(this->type_);
    Assert(pt != 0x0, "Error: dynamic cast to Projection type failed\n");

    pt->initialize_type();
  }
}

void Parameter::set_in(bool b)
{
  this->in_ = b;
}

void Parameter::set_out(bool b)
{
  this->out_ = b;
}

void Parameter::set_alloc_caller(bool b)
{
  this->alloc_caller_ = b;
}

void Parameter::set_alloc_callee(bool b)
{
  this->alloc_callee_ = b;
}

void Parameter::set_dealloc_caller(bool b)
{
  this->dealloc_caller_ = b;
}

void Parameter::set_dealloc_callee(bool b)
{
  this->dealloc_callee_ = b;
}

bool Parameter::in()
{
  return this->in_;
}

bool Parameter::out()
{
  return this->out_;
}

bool Parameter::alloc_caller()
{
  return this->alloc_caller_;
}
bool Parameter::alloc_callee()
{
  return this->alloc_callee_;
}

bool Parameter::dealloc_caller()
{
  return this->dealloc_caller_;
}

bool Parameter::dealloc_callee()
{
  return this->dealloc_callee_;
}

void Parameter::set_bind_caller(bool b)
{
  this->bind_caller_ = b;
}

void Parameter::set_bind_callee(bool b)
{
  this->bind_callee_ = b;
}

bool Parameter::bind_caller()
{
  return this->bind_caller_;
}

bool Parameter::bind_callee()
{
  return this->bind_callee_;
}

/* end */

/* Return Variable */
ReturnVariable::ReturnVariable(Type *return_type, int pointer_count,
  const std::string& id) :
  name_(id),
  type_(return_type),
  marshal_info_(NULL),
  accessor_(NULL),
  pointer_count_(pointer_count),
  container_(NULL),
  in_(false),
  out_(false),
  alloc_caller_(false),
  alloc_callee_(false)
{
}

ReturnVariable::ReturnVariable(const ReturnVariable& other)
{
  // copy name
  this->name_ = other.name_;

  // copy type
  this->type_ = other.type_->clone();
  this->in_ = other.in_;
  this->out_ = other.out_;
  this->alloc_callee_ = other.alloc_callee_;
  this->alloc_caller_ = other.alloc_caller_;

  // copy marshal info
  if(other.marshal_info_ != 0x0) {
    this->marshal_info_ = other.marshal_info_->clone();
  } else {
    this->marshal_info_ = 0x0;
  }

  // copy accessor
  this->accessor_ = other.accessor_;

  this->pointer_count_ = other.pointer_count_;

  if(other.container_ != 0x0) {
    this->container_ = other.container_->clone();
  } else {
    this->container_ = 0x0;
  }
}

void ReturnVariable::create_container_variable(LexicalScope *ls)
{
    if(this->pointer_count() <= 0 || (this->type_->num() != PROJECTION_TYPE && this->type_->num() != PROJECTION_CONSTRUCTOR_TYPE && this->type_->num() != INITIALIZE_TYPE)) {
    return;
  }
  Type *tmp = this->type_;

  if(this->type_->num() == INITIALIZE_TYPE) { // initialize type

    while(tmp->num() == INITIALIZE_TYPE) {
      InitializeType *it = dynamic_cast<InitializeType*>(tmp);
      Assert(it != 0x0, "Error: dynamic cast to initialize type failed\n");

      tmp = it->type_;
    }

    if(tmp->num() != PROJECTION_TYPE && tmp->num() != PROJECTION_CONSTRUCTOR_TYPE) {
      return;
    }
  }

  // lookup in scope the container for its type. 
  int err;
  Type *container_t = ls->lookup(container_name(tmp->name()), &err);
  Assert(container_t != 0x0, "Error: could not find container in scope\n");

  ProjectionType *container = dynamic_cast<ProjectionType*>(container_t);
  Assert(container != 0x0, "Error: could not dynamically cast to projection\n");

  // really need to make variable non abstract and get rid of unnecessary variables.

  const std::string& name = container_name(append_strings("_", construct_list_vars(this)));

  ReturnVariable *container_var = new ReturnVariable(container, 1, name);
  container_var->set_in(this->in());
  container_var->set_out(this->out());
  container_var->set_alloc_caller(this->alloc_caller());

  // save. 
  this->container_ = container_var;

  ProjectionType *pt = dynamic_cast<ProjectionType*>(tmp);
  Assert(pt != 0x0, "Error: dynamic cast to projection type failed\n");
  
  for (auto pf : *pt) {
    pf->create_container_variable(ls);
  }
}

Variable* ReturnVariable::container()
{
  return this->container_;
}

void ReturnVariable::prepare_marshal(MarshalPrepareVisitor *worker)
{
  if (this->container_ != 0x0) {
    this->container_->prepare_marshal(worker);
  }
  
  this->marshal_info_ = this->type_->accept(worker);
}

void ReturnVariable::set_marshal_info(Marshal_type *mt)
{
  this->marshal_info_ = mt;
}

Marshal_type* ReturnVariable::marshal_info()
{
  return this->marshal_info_;
}

const std::string& ReturnVariable::identifier() const
{
  return this->name_;
}

void ReturnVariable::set_identifier(const std::string& id)
{
  this->name_ = id;
}

Type* ReturnVariable::type()
{
  return this->type_;
}

void ReturnVariable::set_accessor(Variable *v)
{
  this->accessor_ = v;

  if(this->container_ != 0x0) {
    this->container_->set_accessor(0x0);
  }

  if(this->type_->num() == PROJECTION_TYPE || this->type_->num() == PROJECTION_CONSTRUCTOR_TYPE) {
    ProjectionType *pt = dynamic_cast<ProjectionType*>(this->type_);
    Assert(pt != 0x0, "Error: dynamic cast to projection type failed\n");
    
    for (auto pf : *pt) {
      pf->set_accessor(this);
    }
  }
}

int ReturnVariable::pointer_count()
{
  return this->pointer_count_;
}

void ReturnVariable::set_pointer_count(int pcount)
{
  this->pointer_count_ = pcount;
}

void ReturnVariable::resolve_types(LexicalScope *ls)
{
 // need to rewrite to account for initializetype
//  Type *last = this->type_;
//  Type *tmp = this->type_;
  
  if(this->type_->num() == INITIALIZE_TYPE) {
    Type *tmp = this->type_;
    InitializeType *it = 0x0;
    while(tmp->num() == INITIALIZE_TYPE) {
      it = dynamic_cast<InitializeType*>(tmp);
      Assert(it != 0x0, "Error: dynamic cast to initialize type failed\n");
      tmp = it->type_;
    }
    
    if(tmp->num() != UNRESOLVED_TYPE) {
      return;
    }

    int err;
    Type *t = ls->lookup(tmp->name(), &err);
    if(t == 0x0) {
      std::cout << "Error: could not resolve type " <<  tmp->name() << std::endl;
      return;
    }

    it->type_ = t;
    return;

  } else {

    // check if unresolved
    if(this->type_->num() != UNRESOLVED_TYPE) {
      return;
    }
    
    int err;
    Type *t = ls->lookup(this->type_->name(), &err);
    if(t == 0x0) {
      std::cout << "Error: could not resolve type " <<  this->type_->name() << std::endl;
      return;
    } 
    
    // and set
    this->type_ = t;
    return;
  }
}

void ReturnVariable::initialize_type()
{
  if ( this->type_->num() == INITIALIZE_TYPE ) {
    InitializeType *it = dynamic_cast<InitializeType*>(this->type_);
    Assert(it != 0x0, "Error: dynamic cast to Initialize type failed\n");
    
    it->initialize();
    this->type_ = it->type_;
  } else if (this->type_->num() == PROJECTION_TYPE || this->type_->num() == PROJECTION_CONSTRUCTOR_TYPE) {
    ProjectionType *pt = dynamic_cast<ProjectionType*>(this->type_);
    Assert(pt != 0x0, "Error: dynamic cast to Projection type failed\n");

    pt->initialize_type();
  }
}

Variable* ReturnVariable::accessor()
{
  return this->accessor_;
}

void ReturnVariable::set_in(bool b)
{
  std::cout << "ReturnVariable::set_in: error this operation not allowed\n";
}

void ReturnVariable::set_out(bool b)
{
  std::cout << "ReturnVariable::set_out: error this operation not allowed\n";
}

void ReturnVariable::set_alloc_caller(bool b)
{
  alloc_caller_ = b;
}

void ReturnVariable::set_alloc_callee(bool b)
{
  alloc_callee_ = b;
}

void ReturnVariable::set_dealloc_caller(bool b)
{
  std::cout << "ReturnVariable::set_dealloc_caller: error this operation not allowed\n";
}

void ReturnVariable::set_dealloc_callee(bool b)
{
  std::cout << "ReturnVariable::set_dealloc_callee: error this operation not allowed\n";
}

bool ReturnVariable::in()
{
  return this->in_;
}

bool ReturnVariable::out()
{
  return this->out_;
}

bool ReturnVariable::alloc_caller()
{
  return this->alloc_caller_;
}

bool ReturnVariable::alloc_callee()
{
  return this->alloc_callee_;
}

bool ReturnVariable::dealloc_caller()
{
  std::cout << "ReturnVariable::dealloc_caller: error this operation not allowed\n";
  return false;
}

bool ReturnVariable::dealloc_callee()
{
  std::cout << "ReturnVariable::dealloc_callee: error this operation not allowed\n";
  return false;
}

void ReturnVariable::set_bind_caller(bool b)
{
  std::cout << "this operation is not allowed\n";
}

void ReturnVariable::set_bind_callee(bool b)
{
  std::cout << "this operation is not allowed\n";
}

bool ReturnVariable::bind_caller()
{
  std::cout << "this operation is not allowed\n";
  return false;
}

bool ReturnVariable::bind_callee()
{
  std::cout << "this operation is not allowed\n";
  return false;
}

/* end */

/* projection field */

ProjectionField::ProjectionField(Type* field_type, const std::string& field_name,
  int pointer_count) :
  in_(false),
  out_(false),
  alloc_callee_(false),
  alloc_caller_(false),
  dealloc_callee_(false),
  dealloc_caller_(false),
  bind_callee_(false),
  bind_caller_(false),
  type_(field_type),
  field_name_(field_name),
  accessor_(NULL),
  pointer_count_(pointer_count),
  marshal_info_(NULL),
  container_(NULL)
{
}

ProjectionField::ProjectionField(const ProjectionField& other)
{
  this->in_ = other.in_;
  this->out_ = other.out_;
  this->alloc_callee_ = other.alloc_callee_;
  this->alloc_caller_ = other.alloc_caller_;
  this->dealloc_callee_ = other.dealloc_callee_;
  this->dealloc_caller_ = other.dealloc_caller_;
  this->bind_callee_ = other.bind_callee_;
  this->bind_caller_ = other.bind_caller_;

  // copy Type
  this->type_ = other.type_->clone();
  // copy field name
  this->field_name_ = other.field_name_;

  this->pointer_count_ = other.pointer_count_;

  // copy marshal info
  if(other.marshal_info_ != 0x0) {
    this->marshal_info_ = other.marshal_info_->clone();
  } else {
    this->marshal_info_ = 0x0;
  }
  // copy container;
  if(other.container_ != 0x0) {
    this->container_ = other.container_->clone();
  } else {
    this->container_ = 0x0;
  }

  // copy accessor
  this->accessor_ = other.accessor_;
}

void ProjectionField::create_container_variable(LexicalScope *ls)
{
  if (this->pointer_count() <= 0
    || (this->type_->num() != PROJECTION_TYPE
      && this->type_->num() != PROJECTION_CONSTRUCTOR_TYPE
      && this->type_->num() != INITIALIZE_TYPE)
    || (!this->bind_caller() && !this->bind_callee() && !this->alloc_caller()
      && !this->alloc_callee())) {
    return;
  }
  Type *tmp = this->type_;
  
  if(this->type_->num() == INITIALIZE_TYPE) { // initialize type
    while(tmp->num() == INITIALIZE_TYPE) {
      InitializeType *it = dynamic_cast<InitializeType*>(tmp);
      Assert(it != 0x0, "Error: dynamic cast to initialize type failed\n");
      
      tmp = it->type_;
    }

    if(tmp->num() != PROJECTION_TYPE && tmp->num() != PROJECTION_CONSTRUCTOR_TYPE) {
      return;
    }
  }

  // lookup in scope the container for its type. 
  int err;
  Type *container_t = ls->lookup(container_name(tmp->name()), &err);
  std::cout << "looking up container " <<  container_name(tmp->name()) << std::endl;
  Assert(container_t != 0x0, "Error: could not find container in scope\n");

  ProjectionType *container = dynamic_cast<ProjectionType*>(container_t);
  Assert(container != 0x0, "Error: could not dynamically cast to projection\n");

  // really need to make variable non abstract and get rid of unnecessary variables.

  const std::string& name = container_name(append_strings("_", construct_list_vars(this)));

  ProjectionField *container_var = new ProjectionField(container, name, 1);
  container_var->set_in(this->in());
  container_var->set_out(this->out());
  container_var->set_bind_callee(this->bind_callee());
  container_var->set_bind_caller(this->bind_caller());
  container_var->set_dealloc_callee(this->dealloc_callee());
  container_var->set_dealloc_caller(this->dealloc_caller());


  // save. 
  this->container_ = container_var;

  ProjectionType *pt = dynamic_cast<ProjectionType*>(tmp);
  Assert(pt != 0x0, "Error: dynamic cast to projection type failed\n");
  
  for (auto pf : *pt) {
    pf->create_container_variable(ls);
  }
}

Variable* ProjectionField::container()
{
  return this->container_;
}

void ProjectionField::prepare_marshal(MarshalPrepareVisitor *worker)
{
  if (this->container_ != 0x0) {
    this->container_->prepare_marshal(worker);
  }

  this->marshal_info_ = this->type_->accept(worker);
}

Type* ProjectionField::type()
{
  return this->type_;
}

int ProjectionField::pointer_count()
{
  return this->pointer_count_;
}

void ProjectionField::set_pointer_count(int pcount)
{
  this->pointer_count_ = pcount;
}

const std::string& ProjectionField::identifier() const
{
  return this->field_name_;
}

void ProjectionField::set_identifier(const std::string& id)
{
  this->field_name_ = id;
}

void ProjectionField::set_accessor(Variable *v)
{
  this->accessor_ = v;

  if (this->container_ != 0x0) {
    this->container_->set_accessor(0x0);
  }
  
  if(this->type_->num() == PROJECTION_TYPE || this->type_->num() == PROJECTION_CONSTRUCTOR_TYPE) {
    ProjectionType *pt = dynamic_cast<ProjectionType*>(this->type_);
    Assert(pt != 0x0, "Error: dynamic cast to projection type failed\n");
    
    for (auto pf : *pt) {
      pf->set_accessor(this);
    }
  }
}

Variable* ProjectionField::accessor()
{
  return this->accessor_;
}

void ProjectionField::set_marshal_info(Marshal_type *mt)
{
  this->marshal_info_ = mt;
}

Marshal_type* ProjectionField::marshal_info()
{
  return this->marshal_info_;
}

void ProjectionField::resolve_types(LexicalScope *ls)
{
 // need to rewrite to account for initializetype
//  Type *last = this->type_;
//  Type *tmp = this->type_;
  
  if(this->type_->num() == INITIALIZE_TYPE) {
    Type *tmp = this->type_;
    InitializeType *it = 0x0;
    while(tmp->num() == INITIALIZE_TYPE) {
      it = dynamic_cast<InitializeType*>(tmp);
      Assert(it != 0x0, "Error: dynamic cast to initialize type failed\n");
      tmp = it->type_;
    }
    
    if(tmp->num() != UNRESOLVED_TYPE) {
      return;
    }

    int err;
    Type *t = ls->lookup(tmp->name(), &err);
    if(t == 0x0) {
      std::cout << "Error: could not resolve type " <<  tmp->name() << std::endl;
      return;
    }

    it->type_ = t;
    return;

  } else {

    // check if unresolved
    if(this->type_->num() != UNRESOLVED_TYPE) {
      return;
    }
    
    int err;
    Type *t = ls->lookup(this->type_->name(), &err);
    if(t == 0x0) {
      std::cout << "Error: could not resolve type " <<  this->type_->name() << std::endl;
      return;
    } 
    
    // and set
    this->type_ = t;
    return;
  }
}

void ProjectionField::initialize_type()
{
  if ( this->type_->num() == INITIALIZE_TYPE ) {
    InitializeType *it = dynamic_cast<InitializeType*>(this->type_);
    Assert(it != 0x0, "Error: dynamic cast to Initialize type failed\n");
    
    it->initialize();
    this->type_ = it->type_;
  } else if (this->type_->num() == PROJECTION_TYPE || this->type_->num() == PROJECTION_CONSTRUCTOR_TYPE) {
    ProjectionType *pt = dynamic_cast<ProjectionType*>(this->type_);
    Assert(pt != 0x0, "Error: dynamic cast to Projection type failed\n");

    pt->initialize_type();
  }
}

//ah note - the following is where the marshalling parameters are set
void ProjectionField::set_in(bool b)
{
  this->in_ = b;
}

void ProjectionField::set_out(bool b)
{
  this->out_ = b;
}

void ProjectionField::set_alloc_callee(bool b)
{
  this->alloc_callee_ = b;
}

void ProjectionField::set_alloc_caller(bool b)
{
  this->alloc_caller_ = b;
}

void ProjectionField::set_dealloc_caller(bool b)
{
  this->dealloc_caller_ = b;
}

void ProjectionField::set_dealloc_callee(bool b)
{
  this->dealloc_callee_ = b;
}

bool ProjectionField::in()
{
  return this->in_;
}

bool ProjectionField::out()
{
  return this->out_;
}

bool ProjectionField::alloc_callee()
{
  return this->alloc_callee_;
}

bool ProjectionField::alloc_caller()
{
  return this->alloc_caller_;
}

bool ProjectionField::dealloc_caller()
{
  return this->dealloc_caller_;
}

bool ProjectionField::dealloc_callee()
{
  return this->dealloc_callee_;
}

void ProjectionField::set_bind_caller(bool b)
{
  this->bind_caller_ = b;
}

void ProjectionField::set_bind_callee(bool b)
{
  this->bind_callee_ = b;
}

bool ProjectionField::bind_caller()
{
  return this->bind_caller_;
}

bool ProjectionField::bind_callee()
{
  return this->bind_callee_;
}

FPParameter::FPParameter(Type *type, int pointer_count) :
  type_(type),
  pointer_count_(pointer_count),
  marshal_info_(NULL),
  container_(NULL)
{
}

FPParameter::FPParameter(const FPParameter& other)
{
  // copy type
  this->type_ = other.type_->clone();

  // copy container
  this->container_ = other.container_->clone();

  // copy marshal info
  if (other.marshal_info_ != 0x0) {
    this->marshal_info_ = other.marshal_info_->clone();
  } else {
    this->marshal_info_ = 0x0;
  }

  this->pointer_count_ = other.pointer_count_;
}

void FPParameter::create_container_variable(LexicalScope *ls)
{
  if(this->pointer_count() <= 0 || (this->type_->num() != PROJECTION_TYPE && this->type_->num() != PROJECTION_CONSTRUCTOR_TYPE && this->type_->num() != INITIALIZE_TYPE)) {
    return;
  }
  Type *tmp = this->type_;
  
  if(this->type_->num() == INITIALIZE_TYPE) { // initialize type
    while(tmp->num() == INITIALIZE_TYPE) {
      InitializeType *it = dynamic_cast<InitializeType*>(tmp);
      Assert(it != 0x0, "Error: dynamic cast to initialize type failed\n");

      tmp = it->type_;
    }

    if(tmp->num() != PROJECTION_TYPE && tmp->num() != PROJECTION_CONSTRUCTOR_TYPE) {
      return;
    }
  }

  // lookup in scope the container for its type. 
  int err;
  Type *container_t = ls->lookup(container_name(tmp->name()), &err);
  Assert(container_t != 0x0, "Error: could not find container in scope\n");

  ProjectionType *container = dynamic_cast<ProjectionType*>(container_t);
  Assert(container != 0x0, "Error: could not dynamically cast to projection\n");

  // really need to make variable non abstract and get rid of unnecessary variables.

  const std::string& name = container_name(append_strings("_", construct_list_vars(this)));

  FPParameter *container_var = new FPParameter(container, 1);
  container_var->set_in(this->in());
  container_var->set_out(this->out());

  // save. 
  this->container_ = container_var;

  ProjectionType *pt = dynamic_cast<ProjectionType*>(tmp);
  Assert(pt != 0x0, "Error: dynamic cast to projection type failed\n");
  
  for (auto pf : *pt) {
    pf->create_container_variable(ls);
  }
}

Variable* FPParameter::container()
{
  return this->container_;
}

void FPParameter::prepare_marshal(MarshalPrepareVisitor *worker)
{
  if (this->container_ != 0x0) {
    this->container_->prepare_marshal(worker);
  }
  
  this->marshal_info_ = this->type_->accept(worker);
}

Type* FPParameter::type()
{
  return this->type_;
}

int FPParameter::pointer_count()
{
  return this->pointer_count_;
}

void FPParameter::set_pointer_count(int pcount)
{
  this->pointer_count_ = pcount;
}

const std::string& FPParameter::identifier() const
{
  Assert(1 == 0, "Error: operation not allowed on function pointer parameter\n");
  std::string *str = new std::string("");
  return *str;
}

void FPParameter::set_identifier(const std::string& id)
{
  return;
}

void FPParameter::set_marshal_info(Marshal_type *mt)
{
  this->marshal_info_ = mt;
}

Marshal_type* FPParameter::marshal_info()
{
  return this->marshal_info_;
}

void FPParameter::resolve_types(LexicalScope *ls)
{
 // need to rewrite to account for initializetype
//  Type *last = this->type_;
//  Type *tmp = this->type_;
  
  if(this->type_->num() == INITIALIZE_TYPE) {
    Type *tmp = this->type_;
    InitializeType *it = 0x0;
    while(tmp->num() == INITIALIZE_TYPE) {
      it = dynamic_cast<InitializeType*>(tmp);
      Assert(it != 0x0, "Error: dynamic cast to initialize type failed\n");
      tmp = it->type_;
    }
    
    if(tmp->num() != UNRESOLVED_TYPE) {
      return;
    }

    int err;
    Type *t = ls->lookup(tmp->name(), &err);
    if(t == 0x0) {
      std::cout << "Error: could not resolve type " <<  tmp->name() << std::endl;
      return;
    }

    it->type_ = t;
    return;

  } else {

    // check if unresolved
    if(this->type_->num() != UNRESOLVED_TYPE) {
      return;
    }
    
    int err;
    Type *t = ls->lookup(this->type_->name(), &err);
    if(t == 0x0) {
      std::cout << "Error: could not resolve type " <<  this->type_->name() << std::endl;
      return;
    } 
    
    // and set
    this->type_ = t;
    return;
  }
}

void FPParameter::initialize_type()
{
  if ( this->type_->num() == INITIALIZE_TYPE ) {
    InitializeType *it = dynamic_cast<InitializeType*>(this->type_);
    Assert(it != 0x0, "Error: dynamic cast to Initialize type failed\n");
    
    it->initialize();
    this->type_ = it->type_;
  } else if (this->type_->num() == PROJECTION_TYPE || this->type_->num() == PROJECTION_CONSTRUCTOR_TYPE) {
    ProjectionType *pt = dynamic_cast<ProjectionType*>(this->type_);
    Assert(pt != 0x0, "Error: dynamic cast to Projection type failed\n");

    pt->initialize_type();
  }
}

void FPParameter::set_in(bool b)
{
  std::cout << "this operation is now allowed\n";
}

void FPParameter::set_out(bool b)
{
  std::cout << "this operation is now allowed\n";
}

void FPParameter::set_alloc_caller(bool b)
{
  std::cout << "this operation is now allowed\n";
}

void FPParameter::set_alloc_callee(bool b)
{
  std::cout << "this operation is now allowed\n";
}

void FPParameter::set_dealloc_caller(bool b)
{
  std::cout << "this operation is now allowed\n";
}

void FPParameter::set_dealloc_callee(bool b)
{
  std::cout << "this operation is now allowed\n";
}

bool FPParameter::in()
{
  std::cout << "this operation is now allowed\n";
  return false;
}

bool FPParameter::out()
{
  std::cout << "this operation is now allowed\n";
  return false;
}

bool FPParameter::alloc_caller()
{
  std::cout << "this operation is now allowed\n";
  return false;
}
bool FPParameter::alloc_callee()
{
  std::cout << "this operation is now allowed\n";
  return false;
}

bool FPParameter::dealloc_caller()
{
  std::cout << "this operation is now allowed\n";
  return false;
}

bool FPParameter::dealloc_callee()
{
  std::cout << "this operation is now allowed\n";
  return false;
}

void FPParameter::set_bind_caller(bool b)
{
  std::cout << "this operation is not allowed\n";
}

void FPParameter::set_bind_callee(bool b)
{
  std::cout << "this operation is not allowed\n";
}

bool FPParameter::bind_caller()
{
  std::cout << "this operation is not allowed\n";
  return false;
}

bool FPParameter::bind_callee()
{
  std::cout << "this operation is not allowed\n";
  return false;
}

/* end */

