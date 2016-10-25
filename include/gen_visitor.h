#ifndef GEN_VISITOR
#define GEN_VISITOR

#include "visitor.h"

class GenVisitor : public ASTVisitor
{
  FILE* out_f_;
 public:
  virtual void visit(File *f) =0;
  virtual void visit(Rpc *rpc) =0;
  virtual void visit(Parameter * p) =0;
  virtual void visit(ProjectionType * p) =0;
  virtual void visit(ProjectionField * pf) =0;
  virtual void visit(IntegerType * it) =0;
  virtual void visit(PointerType * it) =0;
  virtual void visit(Typedef* td) =0;
};

class HeaderVisitor : public GenVisitor
{
  //FILE* out_f_;
 public:
  HeaderVisitor(FILE* f);
  virtual void visit(File *f);
  virtual void visit(Rpc *rpc);
  virtual void visit(Parameter * p);
  virtual void visit(ProjectionType * p);
  virtual void visit(ProjectionField * pf);
  virtual void visit(IntegerType * it);
  virtual void visit(PointerType * it);
  virtual void visit(Typedef* td);
};

class SourceVisitor : public GenVisitor
{
  // FILE* out_f_;
 public:
  SourceVisitor(FILE* f);
  virtual void visit(File *f);
  virtual void visit(Rpc *rpc);
  virtual void visit(Parameter * p);
  virtual void visit(ProjectionType * p);
  virtual void visit(ProjectionField * pf);
  virtual void visit(IntegerType * it);
  virtual void visit(PointerType * it);
  virtual void visit(Typedef* td);
};

#endif
