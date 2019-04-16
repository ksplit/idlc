#ifndef MARSHAL_OP_H
#define MARSHAL_OP_H

#define LCD_MAX_REGS 8
#define LCD_MAX_CAP_REGS 8

#define REG(x) "lcd_r" #x
#define STORE_REG(x) "lcd_set_r" #x
#define CREG(x) "lcd_cr" #x
#define STORE_CREG(x) "lcd_set_cr" #x
#define IPC_GET_REG(x) "fipc_get_reg" #x
#define IPC_SET_REG(x) "fipc_set_reg" #x

#include "ccst.h"
#include "lcd_ast.h"
#include <stdlib.h>

const std::string access_register_mapping(int register_index);
const std::string store_register_mapping(int register_index);
const std::string load_async_reg_mapping(int idx);
const std::string store_async_reg_mapping(int register_index);

class Type;
class Function;
class Channel;
class Parameter;
class IntegerType;
class ProjectionType;
class ProjectionField;
class VoidType;
class PointerType;
class Typedef;
class Rpc;
class File;
class TypeVisitor;
class CCSTCompoundStatement;
class ReturnVariable;
class Variable;
class UnresolvedType;
class ProjectionConstructorType;
class InitializeType;
class BoolType;
class DoubleType;
class FloatType;

class Registers {
  int regs_[LCD_MAX_REGS];
  int cap_regs_[LCD_MAX_CAP_REGS];

public:
  Registers();
  void init(Registers *r1, Registers *r2); // set union.
  void init(int regs_taken[], int len_regs_taken, int caps_taken[],
            int len_caps_taken);
  // finds next free register and set it as allocated;
  int allocate_next_free_register();
};

class Marshal_type {
public:
  virtual Marshal_type *clone() const = 0;
  virtual void set_register(int r) = 0;
  virtual int get_register() = 0;
};

class Marshal_float : public Marshal_type {
  int register_;

public:
  Marshal_float(int r);
  virtual Marshal_type *clone() const { return new Marshal_float(*this); }
  virtual void set_register(int r);
  virtual int get_register();
};

class Marshal_double : public Marshal_type {
  int register_;

public:
  Marshal_double(int r);
  virtual Marshal_type *clone() const { return new Marshal_double(*this); }
  virtual void set_register(int r);
  virtual int get_register();
};

class Marshal_bool : public Marshal_type {
  int register_;

public:
  Marshal_bool(int r);
  virtual Marshal_type *clone() const { return new Marshal_bool(*this); }
  virtual void set_register(int r);
  virtual int get_register();
};

class Marshal_projection : public Marshal_type {

public:
  Marshal_projection();
  virtual Marshal_type *clone() const { return new Marshal_projection(*this); }
  virtual void set_register(int r);
  virtual int get_register();
};

class Marshal_integer : public Marshal_type {
  int register_;

public:
  Marshal_integer(int r);
  virtual Marshal_type *clone() const { return new Marshal_integer(*this); }
  virtual void set_register(int r);
  virtual int get_register();
};

class Marshal_void : public Marshal_type {
public:
  Marshal_void();
  virtual Marshal_type *clone() const { return new Marshal_void(*this); }
  virtual void set_register(int r);
  virtual int get_register();
};

class Marshal_typedef : public Marshal_type {
  Marshal_type *true_type_;

public:
  Marshal_typedef(Marshal_type *type);
  Marshal_typedef(const Marshal_typedef &other);
  virtual Marshal_type *clone() const { return new Marshal_typedef(*this); }
  virtual void set_register(int r);
  virtual int get_register();
};

class MarshalPrepareVisitor {
public:
  Registers *registers_;
  MarshalPrepareVisitor(Registers *r);
  Marshal_type *visit(UnresolvedType *ut);
  Marshal_type *visit(Function *fp);
  Marshal_type *visit(Typedef *td);
  Marshal_type *visit(VoidType *vt);
  Marshal_type *visit(IntegerType *it);
  Marshal_type *visit(ProjectionType *pt);
  Marshal_type *visit(Channel *c);
  Marshal_type *visit(ProjectionConstructorType *pct);
  Marshal_type *visit(InitializeType *it);
  Marshal_type *visit(BoolType *bt);
  Marshal_type *visit(DoubleType *dt);
  Marshal_type *visit(FloatType *ft);
};

#endif
