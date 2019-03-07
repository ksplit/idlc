#ifndef CAST_H
#define CAST_H

#include "assert.h"
#include "lcd_ast.h"
#include <fstream>

// ah note - this file contains the definitions of all the constructs that
// should be present in the output program

class CCSTFile;
class CCSTAssignExpr;
class CCSTExDeclaration;
class CCSTFuncDef;
class CCSTDeclaration;
class CCSTDecSpecifier;
class CCSTStoClassSpecifier;
class CCSTTypeSpecifier;
class CCSTSimpleTypeSpecifier;
class CCSTStructUnionSpecifier;
class CCSTStructDeclaration;
// class CCSTSpecifierQual;
class CCSTStructDecList;
class CCSTStructDeclarator;
// class CCSTDeclarator;
class CCSTPointer;
class CCSTDirectDeclarator;
class CCSTDirectDecId;
class CCSTDirectDecDec;
class CCSTDirectDecConstExpr;
class CCSTDirectDecParamTypeList;
class CCSTDirectDecIdList;
class CCSTConstExpr;
class CCSTCondExpr;
class CCSTLogicalOrExpr;

class CCSTLogicalAndExpr;
class CCSTInclusiveOrExpr;
class CCSTXorExpr;

class CCSTAndExpr;
class CCSTEqExpr;
class CCSTRelationalExpr;
class CCSTShiftExpr;
class CCSTAdditiveExpr;
class CCSTMultExpr;
class CCSTCastExpr;
class CCSTUnaryExpr;
class CCSTUnaryExprCastExpr;
class CCSTUnaryExprOpOp;
class CCSTUnaryExprSizeOf;
class CCSTPostFixExpr;
class CCSTPostFixExprOpOp;
class CCSTPostFixExprAccess;
class CCSTPostFixExprExpr;
class CCSTPostFixExprAssnExpr;
class CCSTPrimaryExpr;
class CCSTString;
class CCSTPrimaryExprId;
class CCSTConstant;

class CCSTInteger;
class CCSTChar;
class CCSTFloat;
class CCSTEnumConst;
// class CCSTExpression;
// class CCSTAssignExpr;
class CCSTAssignOp;
class CCSTUnaryOp;

class CCSTTypeName;
class CCSTParamTypeList;
class CCSTParamList;
class CCSTParamDeclaration;
// class CCSTAbstDeclarator;
class CCSTDirectAbstDeclarator;
class CCSTEnumSpecifier;
class CCSTEnumeratorList;
class CCSTEnumerator;
class CCSTTypedefName;
class CCSTExprStatement;

class CCSTDeclaration;
class CCSTInitDeclarator;
class CCSTInitializer;
class CCSTInitializerList;
class CCSTCompoundStatement;
class CCSTStatement;
class CCSTLabeledStatement;
class CCSTPlainLabelStatement;
class CCSTCaseStatement;
class CCSTSelectionStatement;
class CCSTIfStatement;
class CCSTIfElseStatement;
class CCSTSwitchStatement;
class CCSTIterationStmnt;

class CCSTWhileLoop;
class CCSTDoLoop;

class CCSTForLoop;
class CCSTJumpStmnt;
class CCSTGoto;
class CCSTContinue;

class CCSTBreak;
class CCSTReturn;

class CCSTMacro;

const std::string indentation(unsigned int level);

class CCSTFile {
  std::vector<CCSTExDeclaration *> defs_;

public:
  CCSTFile(std::vector<CCSTExDeclaration *> defs);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTExDeclaration {
  /*
   <external-declaration> ::= <function-definition>
   | <declaration>
   */
public:
  virtual void write(std::ofstream &of, int indent) = 0;
};

class CCSTDeclarator;
class CCSTInitDeclarator {
  /*

   <init-declarator> ::= <declarator>
   | <declarator> = <initializer>
   */
  CCSTDeclarator *dec_;
  CCSTInitializer *init_;

public:
  CCSTInitDeclarator(){};
  CCSTInitDeclarator(CCSTDeclarator *dec, CCSTInitializer *init);
  CCSTInitDeclarator(CCSTDeclarator *dec);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTDeclarator : public CCSTInitDeclarator // this seems incorrect
{
  /*
   <declarator> ::= {<pointer>}? <direct-declarator>
   */
  CCSTPointer *pointer_;
  CCSTDirectDeclarator *d_dec_;

public:
  CCSTDeclarator(){};
  CCSTDeclarator(CCSTPointer *pointer, CCSTDirectDeclarator *d_dec);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTFuncDef : public CCSTExDeclaration {
  /* <function-definition> ::=
   {<declaration-specifier>}* <declarator> {<declaration>}* <compound-statement>
   */
  std::vector<CCSTDecSpecifier *> specifiers_;
  CCSTDeclarator *ret_;
  std::vector<CCSTDeclaration *> decs_;
  CCSTCompoundStatement *body_;
  std::vector<CCSTMacro *> attributes_;

public:
  CCSTFuncDef(std::vector<CCSTDecSpecifier *> specifiers, CCSTDeclarator *ret,
              std::vector<CCSTDeclaration *> decs, CCSTCompoundStatement *body);
  CCSTFuncDef(std::vector<CCSTDecSpecifier *> specifiers, CCSTDeclarator *ret,
              std::vector<CCSTDeclaration *> decs, CCSTCompoundStatement *body,
              std::vector<CCSTMacro *> attribs);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTDeclaration : public CCSTExDeclaration {
  /*
   <declaration> ::=  {<declaration-specifier>}+ {<init-declarator>}*
   */

public:
  virtual ~CCSTDeclaration() {}
  std::vector<CCSTDecSpecifier *> specifier_;
  std::vector<CCSTInitDeclarator *> decs_;
  std::vector<CCSTMacro *> attributes_;
  CCSTDeclaration(std::vector<CCSTDecSpecifier *> specifier,
                  std::vector<CCSTInitDeclarator *> decs);
  CCSTDeclaration(std::vector<CCSTDecSpecifier *> specifier,
                  std::vector<CCSTMacro *> attribs,
                  std::vector<CCSTInitDeclarator *> decs);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTDecSpecifier {
  /*
   <declaration-specifier> ::= <storage-class-specifier>
   | <type-specifier>
   | <type-qualifier>
   */
public:
  virtual void write(std::ofstream &of, int indent) = 0;
};

enum sto_class_t { auto_t, register_local_t, static_t, extern_t, typedef_t };

class CCSTStoClassSpecifier : public CCSTDecSpecifier {
  // is this even encessary?
  /*
   <storage-class-specifier> ::= auto
   | register
   | static
   | extern
   | typedef
   */
  sto_class_t val_;

public:
  CCSTStoClassSpecifier(sto_class_t val);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTSpecifierQual : public CCSTDecSpecifier {
  /*
   <specifier-qualifier> ::= <type-specifier>
   | <type-qualifier>
   */
public:
  virtual void write(std::ofstream &of, int indent) = 0;
};

class CCSTTypeSpecifier
    : public CCSTSpecifierQual // slightly different from c_bnf
{
  /*
   <type-specifier> ::= void
   | char
   | short
   | int
   | long
   | float
   | double
   | signed
   | unsigned
   | <struct-or-union-specifier>
   | <enum-specifier>
   | <typedef-name>

   */
public:
  virtual void write(std::ofstream &of, int indent) = 0;
};

class CCSTSimpleTypeSpecifier : public CCSTTypeSpecifier {
public:
  enum TypeSpecifier {
    VoidTypeSpec = 0,
    CharTypeSpec,
    ShortTypeSpec,
    IntegerTypeSpec,
    LongTypeSpec,
    FloatTypeSpec,
    DoubleTypeSpec,
    SignedTypeSpec,
    UnsignedTypeSpec,
    OtherTypeSpec,
    BoolTypeSpec
  };
  TypeSpecifier type;

  CCSTSimpleTypeSpecifier(TypeSpecifier type);
  virtual void write(std::ofstream &of, int indent);
};

enum struct_union_t { struct_t, union_t };
// probably unecessary

class CCSTStructUnionSpecifier : public CCSTTypeSpecifier {
  /*
   <struct-or-union-specifier> ::= <struct-or-union> <identifier> {
   {<struct-declaration>}+ } | <struct-or-union> { {<struct-declaration>}+ } |
   <struct-or-union> <identifier>
   */
  struct_union_t s_or_u_;
  std::string id_;
  std::vector<CCSTStructDeclaration *> struct_dec_;

public:
  CCSTStructUnionSpecifier(struct_union_t s_or_u, const std::string &id);
  CCSTStructUnionSpecifier(struct_union_t s_or_u, const std::string &id,
                           std::vector<CCSTStructDeclaration *> struct_dec);
  CCSTStructUnionSpecifier(struct_union_t s_or_u,
                           std::vector<CCSTStructDeclaration *> struct_dec);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTStructDeclaration {
  /*
   <struct-declaration> ::= {<specifier-qualifier>}* <struct-declarator-list>
   */
  std::vector<CCSTSpecifierQual *> spec_qual_;
  CCSTStructDecList *dec_list_;

public:
  CCSTStructDeclaration(std::vector<CCSTSpecifierQual *> spec_qual,
                        CCSTStructDecList *dec_list);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTStructDecList {
  /*
   <struct-declarator-list> ::= <struct-declarator>
   | <struct-declarator-list> , <struct-declarator>
   */
  std::vector<CCSTStructDeclarator *> struct_decs_;

public:
  CCSTStructDecList();
  CCSTStructDecList(std::vector<CCSTStructDeclarator *> struct_decs);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTStructDeclarator : public CCSTStructDecList {
  /*
   <struct-declarator> ::= <declarator>
   | <declarator> : <constant-expression>
   | : <constant-expression>
   */
  CCSTDeclarator *dec_;
  CCSTConstExpr *expr_;

public:
  CCSTStructDeclarator();
  CCSTStructDeclarator(CCSTDeclarator *dec);
  CCSTStructDeclarator(CCSTDeclarator *dec, CCSTConstExpr *expr);
  CCSTStructDeclarator(CCSTConstExpr *expr);
  virtual void write(std::ofstream &of, int indent);
};

// probably does not need to be a class.
enum type_qualifier { none_t, const_t, volatile_t };

class CCSTAbstDeclarator {
  /*
   <abstract-declarator> ::= <pointer>
   | <pointer> <direct-abstract-declarator>
   | <direct-abstract-declarator>
   */
  CCSTPointer *p_;
  CCSTDirectAbstDeclarator *d_abs_dec_;

public:
  CCSTAbstDeclarator();
  CCSTAbstDeclarator(CCSTPointer *p, CCSTDirectAbstDeclarator *d_abs_dec);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTPointer : public CCSTAbstDeclarator {
  /*
   <pointer> ::= * {<type-qualifier>}* {<pointer>}?
   */
  std::vector<type_qualifier> type_q_;
  CCSTPointer *p_;

public:
  CCSTPointer(std::vector<type_qualifier> type_q, CCSTPointer *p);
  CCSTPointer();
  CCSTPointer(std::vector<type_qualifier> type_q);
  CCSTPointer(CCSTPointer *p);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTDirectDeclarator {
  /*
   <direct-declarator> ::= <identifier>
   | ( <declarator> )
   | <direct-declarator> [ {<constant-expression>}? ]
   | <direct-declarator> ( <parameter-type-list> )
   | <direct-declarator> ( {<identifier>}* )
   */
public:
  virtual void write(std::ofstream &of, int indent) = 0;
};

class CCSTDirectDecId : public CCSTDirectDeclarator {
  /*
   <direct-declarator> ::= <identifier>
   | ( <declarator> )
   | <direct-declarator> [ {<constant-expression>}? ]
   | <direct-declarator> ( <parameter-type-list> )
   | <direct-declarator> ( {<identifier>}* )
   */
  std::string id_;

public:
  CCSTDirectDecId(const std::string &id);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTDirectDecDec : public CCSTDirectDeclarator {
  /*
   <direct-declarator> ::= <identifier>
   | ( <declarator> )
   | <direct-declarator> [ {<constant-expression>}? ]
   | <direct-declarator> ( <parameter-type-list> )
   | <direct-declarator> ( {<identifier>}* )
   */
  CCSTDeclarator *dec_;

public:
  CCSTDirectDecDec(CCSTDeclarator *dec);
  virtual void write(std::ofstream &of, int indent);
};
class CCSTDirectDecConstExpr : public CCSTDirectDeclarator {
  /*
   <direct-declarator> ::= <identifier>
   | ( <declarator> )
   | <direct-declarator> [ {<constant-expression>}? ]
   | <direct-declarator> ( <parameter-type-list> )
   | <direct-declarator> ( {<identifier>}* )
   */
  CCSTDirectDeclarator *direct_dec_;
  CCSTConstExpr *const_expr_;

public:
  CCSTDirectDecConstExpr(CCSTDirectDeclarator *direct_dec,
                         CCSTConstExpr *const_expr);
  CCSTDirectDecConstExpr(CCSTDirectDeclarator *direct_dec);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTDirectDecParamTypeList : public CCSTDirectDeclarator {
  /*
   <direct-declarator> ::= <identifier>
   | ( <declarator> )
   | <direct-declarator> [ {<constant-expression>}? ]
   | <direct-declarator> ( <parameter-type-list> )
   | <direct-declarator> ( {<identifier>}* )
   */
  CCSTDirectDeclarator *direct_dec_;
  CCSTParamTypeList *p_t_list_;

public:
  CCSTDirectDecParamTypeList(CCSTDirectDeclarator *direct_dec,
                             CCSTParamTypeList *p_t_list);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTDirectDecIdList : public CCSTDirectDeclarator {
  /*
   <direct-declarator> ::= <identifier>
   | ( <declarator> )
   | <direct-declarator> [ {<constant-expression>}? ]
   | <direct-declarator> ( <parameter-type-list> )
   | <direct-declarator> ( {<identifier>}* )
   */
  CCSTDirectDeclarator *direct_dec_;
  std::vector<std::string> ids_;

public:
  CCSTDirectDecIdList(CCSTDirectDeclarator *direct_dec,
                      const std::vector<std::string> &ids);
  virtual void write(std::ofstream &of, int indent);
};

// is this right?
class CCSTConstExpr {
  /*
   <constant-expression> ::= <conditional-expression>
   */
  CCSTCondExpr *cond_expr_;

public:
  CCSTConstExpr(CCSTCondExpr *cond_expr);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTStatement {
  /*
   <statement> ::= <labeled-statement>
   | <expression-statement>
   | <compound-statement>
   | <selection-statement>
   | <iteration-statement>
   | <jump-statement>
   */
public:
  virtual void write(std::ofstream &of, int indent) = 0;
};

class CCSTPreprocessor : public CCSTExDeclaration {
  /*
   * control-line = include <pathname>
   * 		| include "pathname"
   */
  std::string pathname;
  bool relative;

public:
  CCSTPreprocessor(const std::string &path, bool relative);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTExpression;

class CCSTExprStatement : public CCSTStatement {
  /*
   <expression-statement> ::= {<expression>}? ;
   */
  CCSTExpression *expr_;

public:
  CCSTExprStatement();
  CCSTExprStatement(CCSTExpression *expr);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTExpression {
  /*
   <expression> ::= <assignment-expression>
   | <expression> , <assignment-expression>
   */
  std::vector<CCSTAssignExpr *> assn_exprs_;

public:
  CCSTExpression();
  CCSTExpression(std::vector<CCSTAssignExpr *> assn);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTAssignExpr : public CCSTExpression {
  /*
   <assignment-expression> ::= <conditional-expression>
   | <unary-expression> <assignment-operator> <assignment-expression>
   */

  CCSTUnaryExpr *unary_expr_;
  CCSTAssignOp *assn_op_;
  CCSTAssignExpr *assn_expr_;

public:
  CCSTAssignExpr();
  CCSTAssignExpr(CCSTUnaryExpr *unary_expr, CCSTAssignOp *assn_op,
                 CCSTAssignExpr *assn_expr);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTCondExpr : public CCSTAssignExpr {
  /*
   <conditional-expression> ::= <logical-or-expression>
   | <logical-or-expression> ? <expression> : <conditional-expression>
   */
  CCSTLogicalOrExpr *log_or_expr_;
  CCSTExpression *expr_;
  CCSTCondExpr *cond_expr_;

public:
  CCSTCondExpr();
  CCSTCondExpr(CCSTLogicalOrExpr *log_or_expr, CCSTExpression *expr,
               CCSTCondExpr *cond_expr);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTLogicalOrExpr : public CCSTCondExpr {
  /*
   <logical-or-expression> ::= <logical-and-expression>
   | <logical-or-expression || <logical-and-expression>
   */
  CCSTLogicalAndExpr *and_;
  CCSTLogicalOrExpr *or_;

public:
  CCSTLogicalOrExpr();
  CCSTLogicalOrExpr(CCSTLogicalOrExpr *or__, CCSTLogicalAndExpr *and__);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTLogicalAndExpr : public CCSTLogicalOrExpr {
  /*
   <logical-and-expression> ::= <inclusive-or-expression>
   | <logical-and-expression && <inclusive-or-expression>
   */
  CCSTLogicalAndExpr *and_;
  CCSTInclusiveOrExpr *or_;

public:
  CCSTLogicalAndExpr();
  CCSTLogicalAndExpr(CCSTLogicalAndExpr *and__, CCSTInclusiveOrExpr *or__);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTInclusiveOrExpr : public CCSTLogicalAndExpr {
  /*
   <inclusive-or-expression> ::= <exclusive-or-expression>
   | <inclusive-or-expression> | <exclusive-or-expression>
   */
  CCSTInclusiveOrExpr *in_or_;
  CCSTXorExpr *xor_;

public:
  CCSTInclusiveOrExpr();
  CCSTInclusiveOrExpr(CCSTInclusiveOrExpr *in_or, CCSTXorExpr *xor__);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTXorExpr : public CCSTInclusiveOrExpr {
  /*
   <exclusive-or-expression> ::= <and-expression>
   | <exclusive-or-expression> ^ <and-expression>
   */
  CCSTXorExpr *xor_;
  CCSTAndExpr *and_;

public:
  CCSTXorExpr();
  CCSTXorExpr(CCSTXorExpr *xor__, CCSTAndExpr *and__);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTAndExpr : public CCSTXorExpr {
  /*

   <and-expression> ::= <equality-expression>
   | <and-expression> & <equality-expression>
   */
  CCSTAndExpr *and_;
  CCSTEqExpr *eq_;

public:
  CCSTAndExpr();
  CCSTAndExpr(CCSTAndExpr *and__, CCSTEqExpr *eq);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTEqExpr : public CCSTAndExpr {
  /*
   <equality-expression> ::= <relational-expression>
   | <equality-expression> == <relational-expression>
   | <equality-expression> != <relational-expression>
   */
  bool equal_;
  CCSTEqExpr *eq_expr_;
  CCSTRelationalExpr *r_expr_;

public:
  CCSTEqExpr();
  CCSTEqExpr(bool equal, CCSTEqExpr *eq_expr, CCSTRelationalExpr *r_expr);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTShiftExpr;

enum relational_op { lessthan_t, greaterthan_t, lessthaneq_t, greaterthaneq_t };
class CCSTRelationalExpr : public CCSTEqExpr {
  /*
   <relational-expression> ::= <shift-expression>
   | <relational-expression> < <shift-expression>
   | <relational-expression> > <shift-expression>
   | <relational-expression> <= <shift-expression>
   | <relational-expression> >= <shift-expression>
   */
  relational_op op_;
  CCSTRelationalExpr *r_expr_;
  CCSTShiftExpr *s_expr_;

public:
  CCSTRelationalExpr();
  CCSTRelationalExpr(relational_op op, CCSTRelationalExpr *r_expr,
                     CCSTShiftExpr *s_expr);
  virtual void write(std::ofstream &of, int indent);
};

enum shift_op { leftshift_t, rightshift_t };

class CCSTShiftExpr : public CCSTRelationalExpr {
  /*
   <shift-expression> ::= <additive-expression>
   | <shift-expression> << <additive-expression>
   | <shift-expression> >> <additive-expression>
   */
  shift_op shift_;
  CCSTShiftExpr *s_expr_;
  CCSTAdditiveExpr *a_expr_;

public:
  CCSTShiftExpr();
  CCSTShiftExpr(shift_op shift, CCSTShiftExpr *s_expr,
                CCSTAdditiveExpr *a_expr);
  virtual void write(std::ofstream &of, int indent);
};

enum additive_op { plus_t, minus_t };
class CCSTAdditiveExpr : public CCSTShiftExpr {
  /*
   <additive-expression> ::= <multiplicative-expression>
   | <additive-expression> + <multiplicative-expression>
   | <additive-expression> - <multiplicative-expression>
   */
  additive_op op_;
  CCSTAdditiveExpr *a_expr_;
  CCSTMultExpr *m_expr_;

public:
  CCSTAdditiveExpr();
  CCSTAdditiveExpr(additive_op op, CCSTAdditiveExpr *a_expr,
                   CCSTMultExpr *m_expr);
  virtual void write(std::ofstream &of, int indent);
};

enum mult_op { multiply_t, divide_t, mod_t };

class CCSTMultExpr : public CCSTAdditiveExpr {
  /*
   <multiplicative-expression> ::= <cast-expression>
   | <multiplicative-expression> * <cast-expression>
   | <multiplicative-expression> / <cast-expression>
   | <multiplicative-expression> % <cast-expression>
   */
  mult_op op_;
  CCSTMultExpr *m_expr_;
  CCSTCastExpr *c_expr_;

public:
  CCSTMultExpr();
  CCSTMultExpr(mult_op op, CCSTMultExpr *m_expr, CCSTCastExpr *c_expr);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTCastExpr : public CCSTMultExpr {
  /*
   <cast-expression> ::= <unary-expression>
   | ( <type-name> ) <cast-expression>
   */
  CCSTTypeName *cast_type_;
  CCSTCastExpr *cast_expr_;

public:
  CCSTCastExpr();
  CCSTCastExpr(CCSTTypeName *cast_type, CCSTCastExpr *cast_expr);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTUnaryExpr : public CCSTCastExpr {
  /*
   <unary-expression> ::= <postfix-expression>
   | ++ <unary-expression>
   | -- <unary-expression>
   | <unary-operator> <cast-expression>
   | sizeof <unary-expression>
   | sizeof <type-name>
   */
public:
  virtual void write(std::ofstream &of, int indent) = 0;
};

class CCSTUnaryExprCastExpr : public CCSTUnaryExpr {
  /*
   <unary-expression> ::= <postfix-expression>
   | ++ <unary-expression>
   | -- <unary-expression>
   | <unary-operator> <cast-expression>
   | sizeof <unary-expression>
   | sizeof <type-name>
   */
  // *name
  CCSTUnaryOp *unary_op_;
  CCSTCastExpr *cast_expr_;

public:
  CCSTUnaryExprCastExpr();
  CCSTUnaryExprCastExpr(CCSTUnaryOp *unary_op, CCSTCastExpr *cast_expr);
  virtual void write(std::ofstream &of, int indent);
};

enum incr_decr_ops { increment_t, decrement_t };
class CCSTUnaryExprOpOp : public CCSTUnaryExpr {
  /*
   <unary-expression> ::= <postfix-expression>
   | ++ <unary-expression>
   | -- <unary-expression>
   | <unary-operator> <cast-expression>
   | sizeof <unary-expression>
   | sizeof <type-name>
   */
  incr_decr_ops op_;
  CCSTUnaryExpr *unary_expr_;

public:
  CCSTUnaryExprOpOp();
  CCSTUnaryExprOpOp(incr_decr_ops op, CCSTUnaryExpr *unary_expr);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTUnaryExprSizeOf : public CCSTUnaryExpr {
  /*
   <unary-expression> ::= <postfix-expression>
   | ++ <unary-expression>
   | -- <unary-expression>
   | <unary-operator> <cast-expression>
   | sizeof <unary-expression>
   | sizeof <type-name>
   */
  CCSTUnaryExpr *unary_expr_;
  CCSTTypeName *type_name_;

public:
  CCSTUnaryExprSizeOf();
  CCSTUnaryExprSizeOf(CCSTUnaryExpr *unary_expr);
  CCSTUnaryExprSizeOf(CCSTTypeName *type_name);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTPostFixExpr : public CCSTUnaryExpr {
  /*
   <postfix-expression> ::= <primary-expression>
   | <postfix-expression> [ <expression> ]
   | <postfix-expression> ( {<assignment-expression>}* )
   | <postfix-expression> . <identifier>
   | <postfix-expression> -> <identifier>
   | <postfix-expression> ++
   | <postfix-expression> --
   */
public:
  virtual void write(std::ofstream &of, int indent) = 0;
};

class CCSTPostFixExprOpOp : public CCSTPostFixExpr {
  /*
   <postfix-expression> ::= <primary-expression>
   | <postfix-expression> [ <expression> ]
   | <postfix-expression> ( {<assignment-expression>}* )
   | <postfix-expression> . <identifier>
   | <postfix-expression> -> <identifier>
   | <postfix-expression> ++
   | <postfix-expression> --
   */
  CCSTPostFixExpr *post_fix_expr_;
  incr_decr_ops op_;

public:
  CCSTPostFixExprOpOp();
  CCSTPostFixExprOpOp(CCSTPostFixExpr *post_fix_expr, incr_decr_ops op);
  virtual void write(std::ofstream &of, int indent);
};

enum accessor { pointer_access_t, object_access_t };

class CCSTPostFixExprAccess : public CCSTPostFixExpr {
  /*
   <postfix-expression> ::= <primary-expression>
   | <postfix-expression> [ <expression> ]
   | <postfix-expression> ( {<assignment-expression>}* )
   | <postfix-expression> . <identifier>
   | <postfix-expression> -> <identifier>
   | <postfix-expression> ++
   | <postfix-expression> --
   */
  accessor op_;
  CCSTPostFixExpr *post_fix_expr_;
  std::string id_;

public:
  CCSTPostFixExprAccess();
  CCSTPostFixExprAccess(CCSTPostFixExpr *post_fix_expr, accessor op,
                        const std::string &id);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTPostFixExprExpr : public CCSTPostFixExpr {
  /*
   <postfix-expression> ::= <primary-expression>
   | <postfix-expression> [ <expression> ]
   | <postfix-expression> ( {<assignment-expression>}* )
   | <postfix-expression> . <identifier>
   | <postfix-expression> -> <identifier>
   | <postfix-expression> ++
   | <postfix-expression> --
   */
  CCSTPostFixExpr *post_fix_expr_;
  CCSTExpression *expr_;

public:
  CCSTPostFixExprExpr();
  CCSTPostFixExprExpr(CCSTPostFixExpr *post_fix_expr, CCSTExpression *expr);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTPostFixExprAssnExpr : public CCSTPostFixExpr {
  /*
   <postfix-expression> ::= <primary-expression>
   | <postfix-expression> [ <expression> ]
   | <postfix-expression> ( {<assignment-expression>}* )
   | <postfix-expression> . <identifier>
   | <postfix-expression> -> <identifier>
   | <postfix-expression> ++
   | <postfix-expression> --
   */
  CCSTPostFixExpr *post_fix_expr_;
  std::vector<CCSTAssignExpr *> args_;

public:
  CCSTPostFixExprAssnExpr();
  CCSTPostFixExprAssnExpr(CCSTPostFixExpr *post_fix_expr,
                          std::vector<CCSTAssignExpr *> args);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTPrimaryExpr : public CCSTPostFixExpr {
  /*
   <primary-expression> ::= <identifier>
   | <constant>
   | <string>
   | ( <expression> )
   */
  CCSTExpression *expr_;

public:
  CCSTPrimaryExpr();
  CCSTPrimaryExpr(CCSTExpression *expr);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTString : public CCSTPrimaryExpr {
  std::string string_;

public:
  CCSTString();
  CCSTString(const std::string &string);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTPrimaryExprId : public CCSTPrimaryExpr {
  std::string id_;

public:
  CCSTPrimaryExprId();
  CCSTPrimaryExprId(const std::string &id);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTConstant : public CCSTPrimaryExpr {
  /*
   <constant> ::= <integer-constant>
   | <character-constant>
   | <floating-constant>
   | <enumeration-constant>
   */
public:
  virtual void write(std::ofstream &of, int indent) = 0;
};

class CCSTInteger : public CCSTConstant {
  int integer_;

public:
  CCSTInteger();
  CCSTInteger(int i);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTChar : public CCSTConstant {

  char c_;

public:
  CCSTChar();
  CCSTChar(char c);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTFloat : public CCSTConstant {

  float f_;
  double d_;
  bool float_;

public:
  CCSTFloat();
  CCSTFloat(float f);
  CCSTFloat(double d);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTEnumConst : public CCSTConstant {

  // values in enum?
  std::string enum_val_;

public:
  CCSTEnumConst();
  CCSTEnumConst(const std::string &enum_val);
  virtual void write(std::ofstream &of, int indent);
};

enum assign_op {
  equal_t,
  mult_eq_t,
  div_eq_t,
  mod_eq_t,
  plus_eq_t,
  minus_eq_t,
  lshift_eq_t,
  rshift_eq_t,
  and_eq_t,
  xor_eq_t,
  or_eq_t
};

class CCSTAssignOp {
  /*
   <assignment-operator> ::= =
   | *=
   | /=
   | %=
   | +=
   | -=
   | <<=
   | >>=
   | &=
   | ^=
   | |=

   */
  assign_op op_;

public:
  CCSTAssignOp();
  CCSTAssignOp(assign_op op);
  virtual void write(std::ofstream &of, int indent);
};

enum unary_op {
  unary_bit_and_t,
  unary_mult_t,
  unary_plus_t,
  unary_minus_t,
  unary_tilde_t,
  unary_bang_t
};

class CCSTUnaryOp {
  // probably overkill
  /*
   <unary-operator> ::= &
   | *
   | +
   | -
   | ~
   | !
   */
  unary_op op_;

public:
  CCSTUnaryOp();
  CCSTUnaryOp(unary_op op);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTTypeName {
  /*
   <type-name> ::= {<specifier-qualifier>}+ {<abstract-declarator>}?
   */
  std::vector<CCSTSpecifierQual *> spec_quals_;
  CCSTAbstDeclarator *abs_dec_;

public:
  CCSTTypeName();
  CCSTTypeName(std::vector<CCSTSpecifierQual *> spec_quals,
               CCSTAbstDeclarator *abs_dec);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTParamTypeList {
  /*
   <parameter-type-list> ::= <parameter-list>
   | <parameter-list> , ...
   */
  CCSTParamList *p_list_;
  bool ellipsis_;

public:
  CCSTParamTypeList();
  CCSTParamTypeList(CCSTParamList *p_list, bool ellipsis);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTParamList : public CCSTParamTypeList {
  /*
   <parameter-list> ::= <parameter-declaration>
   | <parameter-list> , <parameter-declaration>

   */
  std::vector<CCSTParamDeclaration *> p_dec_;

public:
  CCSTParamList();
  CCSTParamList(std::vector<CCSTParamDeclaration *> p_dec);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTParamDeclaration : public CCSTParamList {
  /*
   <parameter-declaration> ::= {<declaration-specifier>}+ <declarator>
   | {<declaration-specifier>}+ <abstract-declarator>
   | {<declaration-specifier>}+
   */
  std::vector<CCSTDecSpecifier *> dec_specs_;
  CCSTDeclarator *dec_;
  CCSTAbstDeclarator *abs_dec_;

public:
  CCSTParamDeclaration();
  CCSTParamDeclaration(std::vector<CCSTDecSpecifier *> dec_specs);
  CCSTParamDeclaration(std::vector<CCSTDecSpecifier *> dec_specs,
                       CCSTDeclarator *dec);
  CCSTParamDeclaration(std::vector<CCSTDecSpecifier *> dec_specs,
                       CCSTAbstDeclarator *abs_dec);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTDirectAbstDeclarator : public CCSTAbstDeclarator {
  /*
   <direct-abstract-declarator> ::=  ( <abstract-declarator> )
   | {<direct-abstract-declarator>}? [ {<constant-expression>}? ]
   | {<direct-abstract-declarator>}? ( {<parameter-type-list>|? )
   */

  CCSTAbstDeclarator *abs_dec_;
  CCSTDirectAbstDeclarator *d_abs_dec_;
  CCSTConstExpr *const_expr_;
  CCSTParamTypeList *param_type_list_;

public:
  CCSTDirectAbstDeclarator();
  CCSTDirectAbstDeclarator(CCSTAbstDeclarator *abs_dec);
  CCSTDirectAbstDeclarator(CCSTDirectAbstDeclarator *d_abs_dec,
                           CCSTConstExpr *const_expr);
  CCSTDirectAbstDeclarator(CCSTDirectAbstDeclarator *d_abs_dec,
                           CCSTParamTypeList *param_type_list);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTEnumSpecifier : public CCSTTypeSpecifier {
  /*
   <enum-specifier> ::= enum <identifier> { <enumerator-list> }
   | enum { <enumerator-list> }
   | enum <identifier>
   */
  std::string id_;
  CCSTEnumeratorList *el_;

public:
  CCSTEnumSpecifier();
  CCSTEnumSpecifier(const std::string &id, CCSTEnumeratorList *el);
  CCSTEnumSpecifier(const std::string &id);
  CCSTEnumSpecifier(CCSTEnumeratorList *el);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTEnumeratorList {
  /*

   <enumerator-list> ::= <enumerator>
   | <enumerator-list> , <enumerator>
   */
  std::vector<CCSTEnumerator *> *list_;

public:
  CCSTEnumeratorList();
  CCSTEnumeratorList(std::vector<CCSTEnumerator *> *list);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTEnumerator : public CCSTEnumeratorList {
  /*
   <enumerator> ::= <identifier>
   | <identifier> = <constant-expression>
   */
  std::string id_;
  CCSTConstExpr *ce_;

public:
  CCSTEnumerator(const std::string &id, CCSTConstExpr *ce);
  CCSTEnumerator(const std::string &id);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTTypedefName : public CCSTTypeSpecifier {
  /*
   <typedef-name> ::= <identifier>
   */
  std::string id_;

public:
  CCSTTypedefName(const std::string &name);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTInitializerList {
  /*
   <initializer-list> ::= <initializer>
   | <initializer-list> , <initializer>
   */
  std::vector<CCSTInitializer *> init_list_;

public:
  CCSTInitializerList();
  CCSTInitializerList(std::vector<CCSTInitializer *> init_list);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTInitializer : public CCSTInitializerList {
  /*

   <initializer> ::= <assignment-expression>
   | { <initializer-list> }
   | { <initializer-list> , }
   */
  CCSTAssignExpr *assn_expr_;
  CCSTInitializerList *init_list_;

public:
  CCSTInitializer(CCSTAssignExpr *assn_expr);
  CCSTInitializer(CCSTInitializerList *init_list);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTCompoundStatement : public CCSTStatement {
  /*
   <compound-statement> ::= { {<declaration>}* {<statement>}* }
   */
  // is this a body? not necessarily
  std::vector<CCSTDeclaration *> declarations_;
  std::vector<CCSTStatement *> statements_;
  std::vector<CCSTStatement *> lbl_statements;

public:
  CCSTCompoundStatement(std::vector<CCSTDeclaration *> decs,
                        std::vector<CCSTStatement *> s);

  CCSTCompoundStatement(std::vector<CCSTDeclaration *> decs,
                        std::vector<CCSTStatement *> s,
                        std::vector<CCSTStatement *> lbls);

  void add_statement(CCSTStatement *s) { this->statements_.push_back(s); }
  virtual void write(std::ofstream &of, int indent);
  virtual ~CCSTCompoundStatement() {}

  const std::vector<CCSTDeclaration *> &getdeclarations() const {
    return declarations_;
  }

  const std::vector<CCSTStatement *> &getstatements() const {
    return statements_;
  }

  const std::vector<CCSTStatement *> &getlblstatements() const {
    return lbl_statements;
  }
};

class CCSTLabeledStatement : public CCSTStatement {
  /*
   <labeled-statement> ::= <identifier> : <statement>
   | case <constant-expression> : <statement>
   | default : <statement>
   */
public:
  virtual void write(std::ofstream &of, int indent) = 0;
};

class CCSTDefaultLabelStatement : public CCSTLabeledStatement {
  CCSTStatement *body_;

public:
  CCSTDefaultLabelStatement(CCSTStatement *body);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTPlainLabelStatement : public CCSTLabeledStatement {
  std::string id_;
  CCSTStatement *stmnt_;

public:
  CCSTPlainLabelStatement(const std::string &id, CCSTStatement *stmnt);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTCaseStatement : public CCSTLabeledStatement {
  CCSTCondExpr *case_label_;
  CCSTStatement *body_;

public:
  CCSTCaseStatement(CCSTCondExpr *c, CCSTStatement *body);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTSelectionStatement : public CCSTStatement {
  /*
   <selection-statement> ::= if ( <expression> ) <statement>
   | if ( <expression> ) <statement> else <statement>
   | switch ( <expression> ) <statement>
   */
  virtual void write(std::ofstream &of, int indent) = 0;
};

class CCSTIfStatement : public CCSTSelectionStatement {
  CCSTExpression *cond_;
  CCSTStatement *body_;

public:
  CCSTIfStatement(CCSTExpression *cond, CCSTStatement *body);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTIfElseStatement : public CCSTSelectionStatement {
  CCSTExpression *cond_;
  CCSTStatement *if_body_;
  CCSTStatement *else_body_;

public:
  CCSTIfElseStatement(CCSTExpression *cond, CCSTStatement *if_body,
                      CCSTStatement *else_body);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTSwitchStatement : public CCSTSelectionStatement {
  CCSTExpression *expr_;
  CCSTStatement *body_;

public:
  CCSTSwitchStatement(CCSTExpression *expr, CCSTStatement *body);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTIterationStmnt : public CCSTStatement {
  /*
   <iteration-statement> ::= while ( <expression> ) <statement>
   | do <statement> while ( <expression> ) ;
   | for ( {<expression>}? ; {<expression>}? ; {<expression>}? ) <statement>
   */
public:
  virtual void write(std::ofstream &of, int indent) = 0;
};

class CCSTWhileLoop : public CCSTIterationStmnt {
  CCSTExpression *cond_;
  CCSTStatement *body_;

public:
  CCSTWhileLoop(CCSTExpression *cond, CCSTStatement *body);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTDoLoop : public CCSTIterationStmnt {
  CCSTExpression *cond_;
  CCSTStatement *body_;

public:
  CCSTDoLoop(CCSTStatement *body, CCSTExpression *cond);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTForLoop : public CCSTIterationStmnt {
  CCSTExpression *init_;
  CCSTExpression *cond_;
  CCSTExpression *up_;
  CCSTStatement *body_;

public:
  CCSTForLoop(CCSTExpression *init, CCSTExpression *cond, CCSTExpression *up,
              CCSTStatement *body);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTJumpStmnt : public CCSTStatement {
  /*
   <jump-statement> ::= goto <identifier> ;
   | continue ;
   | break ;
   | return {<expression>}? ;
   */

public:
  virtual void write(std::ofstream &of, int indent) = 0;
};

class CCSTGoto : public CCSTJumpStmnt {
  std::string identifier_;

public:
  CCSTGoto(const std::string &id);
  virtual void write(std::ofstream &of, int indent);
};

class CCSTContinue : public CCSTJumpStmnt {
public:
  CCSTContinue();
  virtual void write(std::ofstream &of, int indent);
};

class CCSTBreak : public CCSTJumpStmnt {
public:
  CCSTBreak();
  virtual void write(std::ofstream &of, int indent);
};

class CCSTReturn : public CCSTJumpStmnt {
  CCSTExpression *expr_;

public:
  CCSTReturn(CCSTExpression *expr);
  CCSTReturn();
  virtual void write(std::ofstream &of, int indent);
};

class CCSTMacro : public CCSTExDeclaration, public CCSTStatement {
  std::string macro_name;
  std::vector<CCSTAssignExpr *> data_args;
  CCSTCompoundStatement *cstatement;
  bool is_terminal;

public:
  CCSTMacro(const std::string &name, std::vector<CCSTAssignExpr *> data_args,
            bool is_terminal)
      : macro_name(name), data_args(data_args), cstatement(nullptr),
        is_terminal(is_terminal) {}

  CCSTMacro(const std::string &name, CCSTCompoundStatement *cstatement,
            bool is_terminal)
      : macro_name(name), data_args(), cstatement(cstatement),
        is_terminal(is_terminal) {}
  virtual void write(std::ofstream &of, int indent);
};
#endif
