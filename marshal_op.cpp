#include "marshal_op.h"

const static std::vector<std::string> register_access_mapping_ =
  { REG(0), REG(1), REG(2), REG(3), REG(4), REG(5), REG(6), REG(7) };

const static std::vector<std::string> register_store_mapping_ =
  { STORE_REG(0), STORE_REG(1), STORE_REG(2), STORE_REG(3), STORE_REG(4),
  STORE_REG(5), STORE_REG(6), STORE_REG(7) };

const static std::vector<std::string> cr_reg_access_mapping_ =
  { CREG(0), CREG(1), CREG(2), CREG(3), CREG(4), CREG(5), CREG(6), CREG(7) };

const static std::vector<std::string> cr_reg_store_mapping_ =
  { STORE_CREG(0), STORE_CREG(1), STORE_CREG(2), STORE_CREG(3), STORE_CREG(4),
  STORE_CREG(5), STORE_CREG(6), STORE_CREG(7) };

const static std::vector<std::string> ipc_reg_access_mapping_ =
  { IPC_GET_REG(0), IPC_GET_REG(1), IPC_GET_REG(2), IPC_GET_REG(3),
  IPC_GET_REG(4), IPC_GET_REG(5), IPC_GET_REG(6), IPC_GET_REG(7) };

const static std::vector<std::string> ipc_reg_store_mapping_ =
  { IPC_SET_REG(0), IPC_SET_REG(1), IPC_SET_REG(2), IPC_SET_REG(3),
  IPC_SET_REG(4), IPC_SET_REG(5), IPC_SET_REG(6), IPC_SET_REG(7) };

const std::string access_register_mapping(int idx)
{
  Assert(idx < LCD_MAX_REGS, "Illegal register access\n");
  return register_access_mapping_[idx];
}

const std::string store_register_mapping(int idx)
{
  Assert(idx < LCD_MAX_REGS, "Illegal register access\n");
  return register_store_mapping_[idx];
}

const std::string load_async_reg_mapping(int idx)
{
  Assert(idx < LCD_MAX_REGS, "Illegal register access\n");
  return ipc_reg_access_mapping_[idx];
}

const std::string store_async_reg_mapping(int idx)
{
  Assert(idx < LCD_MAX_REGS, "Illegal register access\n");
  return ipc_reg_store_mapping_[idx];
}

/* Registers code*/
/* 
 * Finds next free register, then
 * sets it as used
 */
Registers::Registers() :
  regs_(),
  cap_regs_()
{
}

void Registers::init(int regs_taken[], int len1, int caps_taken[], int len2)
{
  int i;
  for(i = 0; i < LCD_MAX_REGS && i < len1; i ++) {
    regs_[i] = regs_taken[i];
  }

  for(i = 0; i < LCD_MAX_CAP_REGS && i < len2; i ++) {
    cap_regs_[i] = caps_taken[i];
  }
}

void Registers::init(Registers *r1, Registers *r2)
{
  int i;
  for(i = 0; i < LCD_MAX_REGS; i ++) {
    regs_[i] = r1->regs_[i] + r2->regs_[i]; 
  }

  for(i = 0; i < LCD_MAX_CAP_REGS; i ++) {
    cap_regs_[i] = r1->cap_regs_[i] + r2->cap_regs_[i];
  }
}

int Registers::allocate_next_free_register()
{
  int i;
  for(i = 0; i < LCD_MAX_REGS; i ++)
    {
      if(!regs_[i])
	{
	  regs_[i] = 1;
	  return i;
	}
    }
  std::cout << "add Assert();";
  return -1;
}

Marshal_projection::Marshal_projection(int r) :
  register_(r)
{
}

void Marshal_projection::set_register(int r)
{
  register_ = r;
}

int Marshal_projection::get_register()
{
  return register_;
}

Marshal_integer::Marshal_integer(int r)
{
  this->register_ = r;
}

void Marshal_integer::set_register(int r)
{
  this->register_ = r;
}

int Marshal_integer::get_register()
{
  return this->register_;
}

Marshal_void::Marshal_void()
{
}

void Marshal_void::set_register(int r)
{
  Assert(1 == 0, "Error: this operation is not allowed\n");
}

int Marshal_void::get_register()
{
  /// FIXME: Ideally void pointers should be transferred using sync mechanism
  /// Since the code uses the same methodology of getting registers for all
  /// kinds of variable, allocate a register.
  //Assert(1 == 0, "Error: this operation is now allowed\n");
  return 3;
}

Marshal_typedef::Marshal_typedef(Marshal_type *type)
{
  this->true_type_ = type;
}

Marshal_typedef::Marshal_typedef(const Marshal_typedef& other)
{
  this->true_type_ = other.true_type_->clone();
}

void Marshal_typedef::set_register(int r)
{
  true_type_->set_register(r);
}

int Marshal_typedef::get_register()
{
  return this->true_type_->get_register();
}

Marshal_float::Marshal_float(int r)
{
  this->register_ = r;
}

void Marshal_float::set_register(int r)
{
  this->register_ = r;
}

int Marshal_float::get_register()
{
  return this->register_;
}

Marshal_double::Marshal_double(int r)
{
  this->register_ = r;
}

void Marshal_double::set_register(int r)
{
  this->register_ = r;
}

int Marshal_double::get_register()
{
  return this->register_;
}

Marshal_bool::Marshal_bool(int r)
{
  this->register_ = r;
}

void Marshal_bool::set_register(int r)
{
  this->register_ = r;
}

int Marshal_bool::get_register()
{
  return this->register_;
}

/* Marshal Prepare visitor code */

MarshalPrepareVisitor::MarshalPrepareVisitor(Registers *r)
{
  this->registers_ = r;
}

Marshal_type* MarshalPrepareVisitor::visit(UnresolvedType *ut)
{
  Assert(1 == 0, "Error: cannot marshal an unresolved type\n");
  return NULL;
}

Marshal_type* MarshalPrepareVisitor::visit(Channel *c)
{
  std::cout << "marshal prepare visitor channel todo!\n";
  return NULL;
}

Marshal_type* MarshalPrepareVisitor::visit(Function *fp)
{
  std::cout << "Error: cannot allocate a register for functino pointer\n";
  return NULL;
}

Marshal_type* MarshalPrepareVisitor::visit(Typedef *td)
{
  Marshal_type *tmp = td->type()->accept(this);
  return new Marshal_typedef(tmp);
}

Marshal_type* MarshalPrepareVisitor::visit(VoidType *vt)
{
  std::cout << "Error: cannot allocate a register for void type\n";
  return new Marshal_void();
}

Marshal_type* MarshalPrepareVisitor::visit(IntegerType *it)
{
  int r = this->registers_->allocate_next_free_register();
  
  if (r == -1) {
    Assert(1 == 0, "Error: have run out of registers\n");
  }
  
  return new Marshal_integer(r);
}

Marshal_type* MarshalPrepareVisitor::visit(ProjectionType *pt)
{
  // this doesn't work.
  /*
  std::vector<ProjectionField*> fields = pt->fields();
  
  for(std::vector<ProjectionField*>::iterator it = fields.begin(); it != fields.end(); it ++) {
    ProjectionField *pf = *it;
    if (pf == 0x0) {
      Assert(1 == 0, "Error: null pointer in MarshalPrepareVisit visit ProjectionType\n");
    }
    
    pf->set_marshal_info( pf->type()->accept(this) );
  }
  */
  int r = this->registers_->allocate_next_free_register();

  return new Marshal_projection(r);
}

Marshal_type* MarshalPrepareVisitor::visit(ProjectionConstructorType *pct)
{
  int r = this->registers_->allocate_next_free_register();
  return new Marshal_projection(r);
}

Marshal_type* MarshalPrepareVisitor::visit(InitializeType *it)
{
  Assert( 1 == 0, "Error: cannot prepare marshal for initialize type\n");
  return NULL;
}

Marshal_type* MarshalPrepareVisitor::visit(BoolType *bt)
{
  int r = this->registers_->allocate_next_free_register();
  
  if (r == -1) {
    Assert(1 == 0, "Error: have run out of registers\n");
  }
  
  return new Marshal_bool(r);
}

Marshal_type* MarshalPrepareVisitor::visit(DoubleType *dt)
{
  int r = this->registers_->allocate_next_free_register();
  
  if (r == -1) {
    Assert(1 == 0, "Error: have run out of registers\n");
  }
  
  return new Marshal_double(r);
}

Marshal_type* MarshalPrepareVisitor::visit(FloatType *ft)
{
  int r = this->registers_->allocate_next_free_register();
  
  if (r == -1) {
    Assert(1 == 0, "Error: have run out of registers\n");
  }
  
  return new Marshal_float(r);
}
