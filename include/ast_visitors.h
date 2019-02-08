// ========================================================================= //
// IDL Compiler Infrastructure for LCDs					     //
// ========================================================================= //

// ast_visitors.h:
// ==============
// The header file defines VisitNode class, which is implemented by IDL AST
// host nodes in "ast/lcd_ast.cpp". This header file also defines all types of
// AST visitors, for different host nodes of the AST. 

class Project;
class Module;
class Include;
class Require;
class Rpc;
class ReturnVariable;
class VisitNode;
class ASTVisitor;

// VisitNode - any node of the AST that accepts a visit by a visitor must
// implement this class. These implementations are in ast/lcd_ast.cpp
class VisitNode {
  public:
	virtual void accept(ASTVisitor *visitor) = 0;
};

// ASTVisitor - any visitor that visits a node of the AST must implement this
// class. These implementations are in the corresponding .cpp files of visitors
// in "passes" folder, e.g., passes/astprint.cpp.
class ASTVisitor {
  public:
	virtual void visit (Project * node) = 0;
	virtual void visit (Module * node) = 0;
	virtual void visit (Include * node) = 0;
	virtual void visit (Require * node) = 0;
	virtual void visit (Rpc * node) = 0;
	virtual void visit (ReturnVariable * node) = 0;
};

// ASTPrintVisitor - a visitor class that prints info of every node it visits.
// It is implemented in passes/astprint.cpp.
class ASTPrintVisitor : public ASTVisitor {
  public:
	ASTPrintVisitor();
	void visit(Project *p);
	void visit(Module *m);
	void visit(Include *i);	
	void visit(Require *rq);	
	void visit(Rpc *rp);	
	void visit(ReturnVariable *rv);	
};
