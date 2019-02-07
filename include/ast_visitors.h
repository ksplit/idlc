class Project;
class Module;
class Include;
class Require;
class Rpc;
class VisitNode;
class ASTVisitor;

class VisitNode {
  public:
	virtual void accept(ASTVisitor *visitor) = 0;
};

class ASTVisitor {
  public:
	virtual void visit (Project * node) = 0;
	virtual void visit (Module * node) = 0;
	virtual void visit (Include * node) = 0;
	virtual void visit (Require * node) = 0;
	virtual void visit (Rpc * node) = 0;
};

class ASTPrintVisitor : public ASTVisitor {
  public:
	ASTPrintVisitor();
	void visit(Project *p);
	void visit(Module *m);
	void visit(Include *i);	
	void visit(Require *rq);	
	void visit(Rpc *rp);	
};
