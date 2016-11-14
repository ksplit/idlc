#include "ccst.h"
#include <string.h>

CCSTFile::CCSTFile(std::vector<CCSTExDeclaration*> defs)
{
  this->defs_ = defs;
}

void CCSTFile::write(std::ofstream& of, int indent)
{
  /*
    for(std::vector<T>::iterator it = v.begin(); it != v.end(); ++it) {
   */
  for(std::vector<CCSTExDeclaration*>::iterator it = defs_.begin(); it != defs_.end(); ++it)
    {
      CCSTExDeclaration *ex_dec = *it;
      ex_dec->write(of, indent);
    }
}

CCSTFuncDef::CCSTFuncDef(std::vector<CCSTDecSpecifier*> specifiers, CCSTDeclarator *ret, std::vector<CCSTDeclaration*> decs, CCSTCompoundStatement *body)
{
  this->specifiers_ = specifiers;
  this->ret_ = ret;
  this->decs_ = decs;
  this->body_ = body;
  this->attributes_ = std::vector<CCSTMacro*> ();
}

CCSTFuncDef::CCSTFuncDef(std::vector<CCSTDecSpecifier*> specifiers, CCSTDeclarator *ret, std::vector<CCSTDeclaration*> decs, CCSTCompoundStatement *body, std::vector<CCSTMacro*> attribs)
{
  this->specifiers_ = specifiers; 
  this->ret_ = ret; 
  this->decs_ = decs; 
  this->body_ = body;
  this->attributes_ = attribs;
}

void CCSTFuncDef::write(std::ofstream& of, int indent)
{
  for(std::vector<CCSTDecSpecifier*>::iterator it = specifiers_.begin(); it != specifiers_.end(); ++it)
    {
      CCSTDecSpecifier *ds = *it;
      ds->write(of, indent);
    }
  this->ret_->write(of, 0);

    for(std::vector<CCSTMacro*>::iterator it = attributes_.begin(); it != attributes_.end(); ++it) {
      CCSTMacro *mac = *it;
      of << " ";
      mac->write(of, 0);
    }

  for(std::vector<CCSTDeclaration*>::iterator it = decs_.begin(); it != decs_.end(); ++it)
    {
      CCSTDeclaration *ds = *it;
      ds->write(of, 0);
    }
  // write body
  of << "\n";
  of << "{\n";
  this->body_->write(of, indent+1);
  of << "\n}\n\n";
}

CCSTDeclaration::CCSTDeclaration(std::vector<CCSTDecSpecifier*> specifier, std::vector<CCSTInitDeclarator*> decs)
{
  this->specifier_ = specifier;
  this->decs_ = decs;
  this->attributes_ = std::vector<CCSTMacro*> ();
}

CCSTDeclaration::CCSTDeclaration(std::vector<CCSTDecSpecifier*> specifier,
			std::vector<CCSTMacro*> attribs,
			std::vector<CCSTInitDeclarator*> decs)
{
  this->specifier_ = specifier; 
  this->decs_ = decs;
  this->attributes_ = attribs;
}

CCSTStoClassSpecifier::CCSTStoClassSpecifier(sto_class_t val)
{
  this->val_ = val;
}

void CCSTStoClassSpecifier::write(std::ofstream& of, int indent)
{
  of << indentation(indent);
  switch (this->val_)
    {
    case auto_t:
      of << "auto ";
      break;
    case register_local_t:
      of << "register ";
      break;
    case static_t:
      of << "static ";
      break;
    case extern_t:
      of << "extern ";
      break;
    case typedef_t:
      of << "typedef ";
      break;
    default:
      of << "error";
      break;
    } 

}

CCSTSimpleTypeSpecifier::CCSTSimpleTypeSpecifier(TypeSpecifier type) :
  type(type)
{
}

void CCSTSimpleTypeSpecifier::write(std::ofstream& of, int indent)
{
  of << indentation(indent);
  switch (this->type)
    {
    case VoidTypeSpec:
      of << "void ";
      break;
    case CharTypeSpec:
      of << "char ";
      break;
    case ShortTypeSpec:
      of << "short ";
      break;
    case IntegerTypeSpec:
      of << "int ";
      break;
    case LongTypeSpec:
      of << "long ";
      break;
    case FloatTypeSpec:
      of << "float ";
      break;
    case DoubleTypeSpec:
      of << "double ";
      break;
    case SignedTypeSpec:
      of << "signed ";
      break;
    case UnsignedTypeSpec:
      of << "unsigned ";
      break;
    case BoolTypeSpec:
      of << "bool ";
      break;
    default:
      of << "error";
      break;
    } 
}

CCSTStructUnionSpecifier::CCSTStructUnionSpecifier(struct_union_t s_or_u, const std::string& id)
{
  this->s_or_u_ = s_or_u; 
  this->id_ = id;
}

CCSTStructUnionSpecifier::CCSTStructUnionSpecifier(struct_union_t s_or_u, const std::string& id, std::vector<CCSTStructDeclaration*> struct_dec)
{
  this->s_or_u_ = s_or_u; 
  this->id_ = id; 
  this->struct_dec_ = struct_dec;
}

CCSTStructUnionSpecifier::CCSTStructUnionSpecifier(struct_union_t s_or_u, std::vector<CCSTStructDeclaration*> struct_dec)
{
  this->s_or_u_ = s_or_u; 
  this->id_ = ""; 
  this->struct_dec_ = struct_dec;
}

void CCSTStructUnionSpecifier::write(std::ofstream& of, int indent)
{
  switch (this->s_or_u_)
    {
    case struct_t:
      {
	//  <struct-or-union> <identifier> what about this case
	of << indentation(indent) << "struct " << this->id_ << " ";
	break;
      }
    case union_t:
      {
	of << indentation(indent) << "union " << this->id_ << " ";
	break;
      }
    default:
      std::cout << "error";
      exit(0);
      break;
    }
  if(!this->struct_dec_.empty())
    {
      of << "{\n";
      for(std::vector<CCSTStructDeclaration*>::iterator it = struct_dec_.begin(); it != struct_dec_.end(); ++it)
	{
	  CCSTStructDeclaration *ds = *it;
	  ds->write(of, indent + 1);
	  of << ";\n";
	}
      of << "}";
    }
}

CCSTStructDeclaration::CCSTStructDeclaration(std::vector<CCSTSpecifierQual*> spec_qual, CCSTStructDecList *dec_list)
{
  this->spec_qual_ = spec_qual; 
  this->dec_list_ = dec_list;
}

void CCSTStructDeclaration::write(std::ofstream& of, int indent)
{
  of << indentation(indent);
  if(!this->spec_qual_.empty())
    {
      for(std::vector<CCSTSpecifierQual*>::iterator it = spec_qual_.begin(); it != spec_qual_.end(); ++it)
	{
	  CCSTSpecifierQual *ds = *it;
	  ds->write(of, 0);
	  //of << " ";
	  
	}
    }
  this->dec_list_->write(of, 0);
  
}

CCSTStructDecList::CCSTStructDecList()
{
  // todo
}

CCSTStructDecList::CCSTStructDecList(std::vector<CCSTStructDeclarator*> struct_decs)
{
  this->struct_decs_ = struct_decs;
}

void CCSTStructDecList::write(std::ofstream& of, int indent)
{
  of << indentation(indent);
  for(std::vector<CCSTStructDeclarator*>::iterator it = struct_decs_.begin(); it != struct_decs_.end(); ++it)
	{
	  CCSTStructDeclarator *ds = *it;
	  ds->write(of, 0);
	  of << ", "; // if last do not write ,
	}
}
CCSTStructDeclarator::CCSTStructDeclarator()
{
  this->dec_ = NULL;
  this->expr_ = NULL;
}

CCSTStructDeclarator::CCSTStructDeclarator(CCSTDeclarator *dec)
{
  this->dec_ = dec; 
  this->expr_ = NULL;
}

CCSTStructDeclarator::CCSTStructDeclarator(CCSTDeclarator *dec, CCSTConstExpr *expr)
{
  this->dec_ = dec; 
  this->expr_ = expr;
}

CCSTStructDeclarator::CCSTStructDeclarator(CCSTConstExpr *expr)
{
  this->dec_ = NULL; 
  this->expr_ = expr;
}

void CCSTStructDeclarator::write(std::ofstream& of, int indent)
{
  if(this->dec_ != NULL)
    {
      this->dec_->write(of, indent);
    }
  if(this->expr_ != NULL)
    {
      of << " : ";
      this->expr_->write(of, 0);
    }
}

CCSTDeclarator::CCSTDeclarator(CCSTPointer *pointer, CCSTDirectDeclarator *d_dec)
{
  this->pointer_ = pointer; 
  this->d_dec_ = d_dec;
}

void CCSTDeclarator::write(std::ofstream& of, int indent)
{
  
  if(this->pointer_ != NULL)
    {
      this->pointer_->write(of, indent);
    }
  if(this->d_dec_ == NULL)
    {
      std::cout << "error";
      exit(0);
    }
  this->d_dec_->write(of, 0);
}

CCSTPointer::CCSTPointer(std::vector<type_qualifier> type_q, CCSTPointer *p)
{
  this->type_q_ = type_q; 
  this->p_ = p;
}

CCSTPointer::CCSTPointer()
{
  this->p_ = NULL;
}

CCSTPointer::CCSTPointer(std::vector<type_qualifier> type_q)
{
  this->type_q_ = type_q; 
  this->p_ = NULL;
}

CCSTPointer::CCSTPointer(CCSTPointer *p)
{
  this->p_ = p;
}

void CCSTPointer::write(std::ofstream& of, int indent)
{
  of << indentation(indent) << "*";
  for(std::vector<type_qualifier>::iterator it = type_q_.begin(); it != type_q_.end(); ++it)
	{
	  type_qualifier tq =  *it;
	  switch (tq)
	    {
	    case const_t:
	      of << "const ";
	      break;
	    case volatile_t:
	      of << "volatile ";
	      break;
	    default:
	      break;
	    }
	}
  if(this->p_ != NULL)
    {
      this->p_->write(of, 0);
    }
}

CCSTDirectDecId::CCSTDirectDecId(const std::string& id)
{
  this->id_ = id;
}

void CCSTDirectDecId::write(std::ofstream& of, int indent)
{
  of << indentation(indent) << this->id_;
}

CCSTDirectDecDec::CCSTDirectDecDec(CCSTDeclarator *dec)
{
  this->dec_ = dec;
}

void CCSTDirectDecDec::write(std::ofstream& of, int indent)
{
  if(this->dec_ == NULL)
    {
      std::cout << "Error\n";
      exit(0);
    }
  of << indentation(indent) << "( "; // are there actually supposed to be parens?
  this->dec_->write(of, 0);
  of << " )";
}

CCSTDirectDecConstExpr::CCSTDirectDecConstExpr(CCSTDirectDeclarator *direct_dec, CCSTConstExpr *const_expr)
{
  this->direct_dec_ = direct_dec; 
  this->const_expr_ = const_expr;
}

CCSTDirectDecConstExpr::CCSTDirectDecConstExpr(CCSTDirectDeclarator *direct_dec)
{
 this->direct_dec_ = direct_dec; 
 this->const_expr_ = NULL; 
}

void CCSTDirectDecConstExpr::write(std::ofstream& of, int indent)
{
  if(this->direct_dec_ == NULL)
    {
      std::cout << "error\n";
      exit(0);
    }
  this->direct_dec_->write(of, indent);
  of << "[ ";
  if(this->const_expr_ != NULL)
    {
      this->const_expr_->write(of, 0);
    }
  of << "]";
}

CCSTDirectDecParamTypeList::CCSTDirectDecParamTypeList(CCSTDirectDeclarator *direct_dec, CCSTParamTypeList *p_t_list)
{
  this->direct_dec_ = direct_dec; 
  this->p_t_list_ = p_t_list;
}

void CCSTDirectDecParamTypeList::write(std::ofstream& of, int indent)
{
  if(this->direct_dec_ == NULL)
    {
      std::cout << "error\n";
      exit(0);
    }
  this->direct_dec_->write(of, indent);
  of << "(";
  if(this->p_t_list_ == NULL)
    {
      std::cout << "error\n";
      exit(0);
    }
  this->p_t_list_->write(of, 0);
  of << ")";
}

CCSTDirectDecIdList::CCSTDirectDecIdList(CCSTDirectDeclarator *direct_dec, const std::vector<std::string>& ids)
{
  this->direct_dec_ = direct_dec; 
  this->ids_ = ids;
}

void CCSTDirectDecIdList::write(std::ofstream& of, int indent)
{
  if(this->direct_dec_ == NULL)
    {
      std::cout << "error\n";
      exit(0);
    }
  of << indentation(indent) << "( ";
  for (std::vector<std::string>::const_iterator it = ids_.begin(); it != ids_.end(); ++it)
	{
	  of << " " << *it;
	  of << ", "; // should i be printing commas
	}
  of << " )";
}

CCSTCondExpr::CCSTCondExpr()
{
  // std::cout << "incomplete cond expr\n"; 
  //todo
}

CCSTCondExpr::CCSTCondExpr(CCSTLogicalOrExpr *log_or_expr, CCSTExpression *expr, CCSTCondExpr *cond_expr)
{
  this->log_or_expr_ = log_or_expr; 
  this->expr_ = expr; 
  this->cond_expr_ = cond_expr;
}

void CCSTCondExpr::write(std::ofstream& of, int indent)
{
  std::cout << "incomplete cond expr\n";
  //todo
}

CCSTConstExpr::CCSTConstExpr(CCSTCondExpr *cond_expr)
{
  this->cond_expr_ = cond_expr;
}

void CCSTConstExpr::write(std::ofstream& of, int indent)
{
  if(this->cond_expr_ == NULL)
    {
      std::cout << "error\n";
      exit(0);
    }
  of << "const todo";
  
}

CCSTLogicalOrExpr::CCSTLogicalOrExpr()
{
  
  //todo
}

CCSTLogicalOrExpr::CCSTLogicalOrExpr(CCSTLogicalOrExpr *or__, CCSTLogicalAndExpr *and__)
{
  this->and_ = and__; 
  this->or_ = or__;
}

void CCSTLogicalOrExpr::write(std::ofstream& of, int indent)
{
  if(this->or_ != NULL)
    {
      if(this->and_ == NULL)
	{
	  std::cout << "error\n";
	  exit(0);
	}
      of << indentation(indent) << "( ";
      this->or_->write(of, 0);
      of << " )";
      of << " || ";
      of << "( ";
      this->and_->write(of, 0);
      of << " )";
    }
  else 
    {
      // of << "( ";
      this->and_->write(of, indent);
      // of << " )";
    }
}

CCSTLogicalAndExpr::CCSTLogicalAndExpr()
{
  //todo
}

CCSTLogicalAndExpr::CCSTLogicalAndExpr(CCSTLogicalAndExpr *and__, CCSTInclusiveOrExpr *or__)
{
  this->and_ = and__; this->or_ = or__;
}

void CCSTLogicalAndExpr::write(std::ofstream& of, int indent)
{
  if(this->and_ != NULL)
    {
      if(this->or_ == NULL)
	{
	  std::cout << "error\n";
	  exit(0);
	}
      of << indentation(indent)  << "( ";
      this->and_->write(of, 0);
      of << " )";
      of << " && ";
      of << "( ";
      this->or_->write(of, 0);
      of << " )";
    }
  else
    {
      //  of << "( ";
      this->or_->write(of, indent);
      //    of << " )";
    }
}

CCSTInclusiveOrExpr::CCSTInclusiveOrExpr()
{
  //todo
}

CCSTInclusiveOrExpr::CCSTInclusiveOrExpr(CCSTInclusiveOrExpr *in_or, CCSTXorExpr *xor__)
{
  this->in_or_ = in_or; 
  this->xor_ = xor__;
}

void CCSTInclusiveOrExpr::write(std::ofstream& of, int indent)
{
  if(this->in_or_ != NULL)
    {
      if(this->xor_ == NULL)
	{
	  std::cout << "error\n";
	  exit(0);
	}
      of << indentation(indent)  << "( ";
      this->in_or_->write(of, 0);
      of << " )";
      of << " | ";
      of << "( ";
      this->xor_->write(of, 0);
      of << " )";
    }
  else
    {
      this->xor_->write(of, indent);
    }
}

CCSTXorExpr::CCSTXorExpr()
{
  //todo
}

CCSTXorExpr::CCSTXorExpr(CCSTXorExpr *xor__, CCSTAndExpr *and__)
{
  this->xor_ = xor__; 
  this->and_ = and__;
}

void CCSTXorExpr::write(std::ofstream& of, int indent)
{
  if(this->xor_ != NULL)
    {
      of << indentation(indent)  << "( ";
      this->xor_->write(of, 0);
      of << " )";
      of << " ^ ";
      of << "( ";
      this->and_->write(of, 0);
      of << " )";
    }
  else
    {
      // the if else not needed, because and expression inherits from xor expr
      this->and_->write(of, indent);
    }
}

CCSTAndExpr::CCSTAndExpr()
{
  //todo
}

CCSTAndExpr::CCSTAndExpr(CCSTAndExpr *and__, CCSTEqExpr *eq)
{
  this->and_ = and__; 
  this->eq_ = eq;
}

void CCSTAndExpr::write(std::ofstream& of, int indent)
{
  if(this->and_ == NULL || this->eq_ == NULL)
    {
      std::cout << "error\n";
      exit(0);
    }
  of << indentation(indent)  << "( ";
  this->and_->write(of, 0);
  of << " )";
  of << " & ";
  of << "( ";
  this->eq_->write(of, 0);
  of << " )";
}

CCSTEqExpr::CCSTEqExpr()
{
  //todo
}

CCSTEqExpr::CCSTEqExpr(bool equal, CCSTEqExpr *eq_expr, CCSTRelationalExpr *r_expr)
{
  this->equal_ = equal; 
  this->eq_expr_ = eq_expr; 
  this->r_expr_ = r_expr;
}

void CCSTEqExpr::write(std::ofstream& of, int indent)
{
  if(this->eq_expr_ == NULL || this->r_expr_ == NULL)
    {
      std::cout << "error\n";
      exit(0);
    }
  if(equal_)
    {
      of << indentation(indent)  << "( ";
      this->eq_expr_->write(of, 0);
      of << " )";
      of << " == ";
      of << "( ";
      this->r_expr_->write(of, 0);
      of << " )";
    }
  else
    {
      of << indentation(indent)  << "( ";
      this->eq_expr_->write(of, 0);
      of << " )";
      of << " != ";
      of << "( ";
      this->r_expr_->write(of, 0);
      of << " )";
    }
}

CCSTRelationalExpr::CCSTRelationalExpr()
{
  //todo
}

CCSTRelationalExpr::CCSTRelationalExpr(relational_op op, CCSTRelationalExpr *r_expr, CCSTShiftExpr *s_expr)
{
  this->op_ = op; 
  this->r_expr_ = r_expr; 
  this->s_expr_ = s_expr;
}

void CCSTRelationalExpr::write(std::ofstream& of, int indent)
{
  if(this->r_expr_ == NULL || this->s_expr_ == NULL)
    {
      std::cout << "error\n";
      exit(0);
    }
  of << indentation(indent)  << "( ";
  this->r_expr_->write(of, 0);
  of << " )";
  switch (this->op_)
    {
    case lessthan_t:
      of << " < ";
      break;
    case greaterthan_t:
      of << " > ";
      break;
    case lessthaneq_t:
      of << " <= ";
      break;
    case greaterthaneq_t:
      of << " >= ";
      break;
    default:
      {
	std::cout << "error\n";
	exit(0);
      }
    }
  of << "( ";
  this->s_expr_->write(of, 0);
  of << " )";
}

CCSTShiftExpr::CCSTShiftExpr()
{
  //todo
}

CCSTShiftExpr::CCSTShiftExpr(shift_op shift, CCSTShiftExpr *s_expr, CCSTAdditiveExpr *a_expr)
{
  this->shift_ = shift; 
  this->s_expr_ = s_expr; 
  this->a_expr_ = a_expr;
}

void CCSTShiftExpr::write(std::ofstream& of, int indent)
{
  if(this->s_expr_ == NULL || this->a_expr_ == NULL)
    {
      std::cout << "error\n";
      exit(0);
    }
  of << indentation(indent)  << "( ";
  this->s_expr_->write(of, 0);
  of << " )";
  switch (this->shift_)
    {
    case leftshift_t:
      of << " << ";
      break;
    case rightshift_t:
      of << " >> ";
      break;
    default:
      {
	std::cout << "error\n";
	exit(0);
      } 
    }
  of << "( ";
  this->a_expr_->write(of, 0);
  of << " )";
}

CCSTAdditiveExpr::CCSTAdditiveExpr()
{
  //todo
}

CCSTAdditiveExpr::CCSTAdditiveExpr(additive_op op, CCSTAdditiveExpr *a_expr, CCSTMultExpr *m_expr)
{
  this->op_ = op; 
  this->a_expr_ = a_expr; 
  this->m_expr_ = m_expr;
}

void CCSTAdditiveExpr::write(std::ofstream& of, int indent)
{ 
  if(this->a_expr_ == NULL || this->m_expr_ == NULL)
    {
      std::cout << "error\n";
      exit(0);
    }
  of << indentation(indent)  << "( ";
  this->a_expr_->write(of, 0);
  of << " )";
  switch (this->op_)
    {
    case plus_t:
      of << " + ";
      break;
    case minus_t:
      of << " - ";
      break;
    default:
      {
	std::cout << "error\n";
	exit(0);
      }
    }
  of << "( ";
  this->m_expr_->write(of, 0);
  of << " )";
}

CCSTMultExpr::CCSTMultExpr()
{
  //todo
}

CCSTMultExpr::CCSTMultExpr(mult_op op, CCSTMultExpr *m_expr, CCSTCastExpr *c_expr)
{
  this->op_ = op; 
  this->m_expr_ = m_expr; 
  this->c_expr_ = c_expr;
}

void CCSTMultExpr::write(std::ofstream& of, int indent)
{
  if(this->m_expr_ == NULL || this->c_expr_ == NULL)
    {
      std::cout << "error\n";
      exit(0);
    }
  of << indentation(indent)  << "( ";
  this->m_expr_->write(of, 0);
  of << " )";
  switch (this->op_)
    {
    case multiply_t:
      of << " * ";
      break;
    case divide_t:
      of << " / ";
      break;
    case mod_t:
      of << " % ";
      break;
    default:
      {
	std::cout << "error\n";
	exit(0);
      }
    }
  of << "( ";
  this->c_expr_->write(of, 0);
  of << " )";
}

CCSTCastExpr::CCSTCastExpr()
{
  //todo
}

CCSTCastExpr::CCSTCastExpr(CCSTTypeName *cast_type, CCSTCastExpr *cast_expr)
{
  this->cast_type_ = cast_type; 
  this->cast_expr_ = cast_expr;
}

void CCSTCastExpr::write(std::ofstream& of, int indent)
{
  if(this->cast_type_ == NULL || this->cast_expr_ == NULL)
    {
      std::cout << "error\n";
      exit(0);
    }
  of << indentation(indent)  << "( ";
  this->cast_type_->write(of, 0);
  of << " )";
  this->cast_expr_->write(of, 0);
}

CCSTUnaryExprCastExpr::CCSTUnaryExprCastExpr(CCSTUnaryOp *unary_op, CCSTCastExpr *cast_expr)
{
  this->unary_op_ = unary_op; 
  this->cast_expr_ = cast_expr;
}

void CCSTUnaryExprCastExpr::write(std::ofstream& of, int indent)
{
  if(this->unary_op_ == NULL || this->cast_expr_ == NULL)
    {
      std::cout << "error\n";
      exit(0);
    }
  this->unary_op_->write(of, indent);
  this->cast_expr_->write(of, 0);
}

CCSTUnaryExprOpOp::CCSTUnaryExprOpOp(incr_decr_ops op, CCSTUnaryExpr *unary_expr)
{
  this->unary_expr_ = unary_expr; 
  this->op_ = op;
}

void CCSTUnaryExprOpOp::write(std::ofstream& of, int indent)
{
  if(this->unary_expr_ == NULL)
    {
      std::cout << "error\n";
      exit(0);
    }
  switch (this->op_)
    {
    case increment_t:
      of << indentation(indent) << "++ ";
      break;
    case decrement_t:
      of << indentation(indent) << "-- ";
      break;
    default:
      {
	std::cout << "error\n";
	exit(0);
      }
    }
  this->unary_expr_->write(of, 0);
}

CCSTUnaryExprSizeOf::CCSTUnaryExprSizeOf(CCSTUnaryExpr *unary_expr)
{
  this->unary_expr_ = unary_expr; 
  this->type_name_ = NULL;
}

CCSTUnaryExprSizeOf::CCSTUnaryExprSizeOf(CCSTTypeName *type_name)
{
  this->type_name_  = type_name; 
  this->unary_expr_ = NULL;
}

void CCSTUnaryExprSizeOf::write(std::ofstream& of, int indent)
{
  if(this->unary_expr_ != NULL)
    {
      of << indentation(indent) << "sizeof";
      of << "( ";
      this->unary_expr_->write(of, 0);
      of << " )";
    }
  else if(this->type_name_ != NULL)
    {
      of << indentation(indent) << "sizeof";
      of << "( ";
      this->type_name_->write(of, 0);
      of << " )";
    }
  else
    {
      std::cout << "error\n";
      exit(0);
    }
}

CCSTPostFixExprOpOp::CCSTPostFixExprOpOp(CCSTPostFixExpr *post_fix_expr, incr_decr_ops op)
{
  this->post_fix_expr_ = post_fix_expr; 
  this->op_ = op;
}

// is this correct?
void CCSTPostFixExprOpOp::write(std::ofstream& of, int indent)
{
  if(this->post_fix_expr_ == NULL)
    {
      std::cout << "error\n";
      exit(0);
    }
  this->post_fix_expr_->write(of, indent);
  of << " ";
  switch (this->op_)
    {
    case increment_t:
      of << "++";
      break;
    case decrement_t:
      of << "--";
      break;
    default:
      {
	std::cout << "error\n";
	exit(0);
      }
    }
}

CCSTPostFixExprAccess::CCSTPostFixExprAccess(CCSTPostFixExpr *post_fix_expr, accessor op, const std::string& id)
{
  this->post_fix_expr_ = post_fix_expr; 
  this->op_ = op;
  this->id_ = id;
}

void CCSTPostFixExprAccess::write(std::ofstream& of, int indent)
{
  if(this->post_fix_expr_ == 0x0 || this->id_ == "")
    {
      std::cout << "error\n";
      exit(0);
    }
  this->post_fix_expr_->write(of, indent);
  switch (this->op_)
    {
    case pointer_access_t:
      of << "->";
      break;
    case object_access_t:
      of << ".";
      break;
    default:
      {
	std::cout << "error\n";
	exit(0);
      }
    }
  of << this->id_;
}

CCSTPostFixExprExpr::CCSTPostFixExprExpr(CCSTPostFixExpr *post_fix_expr, CCSTExpression *expr)
{
  this->post_fix_expr_ = post_fix_expr; 
  this->expr_ = expr;
}

void CCSTPostFixExprExpr::write(std::ofstream& of, int indent)
{
  if(this->post_fix_expr_ == NULL || this->expr_ == NULL)
    {
      std::cout << "error\n";
      exit(0);
    }
  this->post_fix_expr_->write(of, indent);
  of << "[ ";
  this->expr_->write(of, 0);
  of << " ]";
}

CCSTPostFixExprAssnExpr::CCSTPostFixExprAssnExpr(CCSTPostFixExpr *post_fix_expr, std::vector<CCSTAssignExpr*> args)
{
  this->post_fix_expr_ = post_fix_expr; 
  this->args_ = args;
}

void CCSTPostFixExprAssnExpr::write(std::ofstream& of, int indent)
{
  if(this->post_fix_expr_ == NULL)
    {
      std::cout << "error\n";
      exit(0);
    }
  
  this->post_fix_expr_->write(of, indent);
  of << "(";
  std::vector<CCSTAssignExpr*> args = this->args_;
  if(!args.empty())
    {
      args.at(0)->write(of, 0);
      
      for(std::vector<CCSTAssignExpr*>::iterator it = args.begin()+1; it != args.end(); ++it)
	{
	  of <<", ";
	  CCSTAssignExpr *arg = *it;
	  arg->write(of, 0);
	 }
    }
  of << ")";
  
}

CCSTPrimaryExpr::CCSTPrimaryExpr()
{
  //todo
}

CCSTPrimaryExpr::CCSTPrimaryExpr(CCSTExpression *expr)
{
  this->expr_ = expr;
}

void CCSTPrimaryExpr::write(std::ofstream& of, int indent)
{
  if(this->expr_ == NULL)
    {
      std::cout << "error\n";
      exit(0);
    }
  of << indentation(indent)  << "( ";
  this->expr_->write(of, 0);
  of << " )";
}

CCSTString::CCSTString(const std::string& string)
{
  this->string_ = string;
}

CCSTString::CCSTString()
{
  //todo
}

void CCSTString::write(std::ofstream& of, int indent)
{
  // how should this be stored exactly?
  of << indentation(indent) << "\"" << this->string_ << "\"";
}

CCSTPrimaryExprId::CCSTPrimaryExprId()
{
  //todo
}

CCSTPrimaryExprId::CCSTPrimaryExprId(const std::string& id)
{
  this->id_ = id;
}

void CCSTPrimaryExprId::write(std::ofstream& of, int indent)
{
  of << indentation(indent) << this->id_;
}

CCSTInteger::CCSTInteger()
{
  this->integer_ = -1;
}

CCSTInteger::CCSTInteger(int i)
{
  this->integer_ = i;
}

void CCSTInteger::write(std::ofstream& of, int indent)
{
  of << indentation(indent) << this->integer_;
}

CCSTChar::CCSTChar()
{
  //todo
}

CCSTChar::CCSTChar(char c)
{
  this->c_ = c;
}

void CCSTChar::write(std::ofstream& of, int indent)
{
  of << indentation(indent) << this->c_;
}

CCSTFloat::CCSTFloat(float f)
{
  this->f_ = f;
  this->float_ = true;
}

CCSTFloat::CCSTFloat(double d)
{
  this->d_ = d;
  this->float_ = false;
}

CCSTFloat::CCSTFloat()
{
  //todo
}

void CCSTFloat::write(std::ofstream& of, int indent)
{
  if(float_)
    {
      of << indentation(indent) << this->f_;
    }
  else
    {
      of << indentation(indent) << this->d_;
    }
}

CCSTEnumConst::CCSTEnumConst(const std::string& enum_val)
{
  this->enum_val_ = enum_val;
}

CCSTEnumConst::CCSTEnumConst()
{
  //todo
}

void CCSTEnumConst::write(std::ofstream& of, int indent)
{
  of << indentation(indent) << this->enum_val_;
}

CCSTExpression::CCSTExpression()
{
  //todo
}

CCSTExpression::CCSTExpression(std::vector<CCSTAssignExpr*> assn)
{
  this->assn_exprs_ = assn;
}

void CCSTExpression::write(std::ofstream& of, int indent)
{
   for(std::vector<CCSTAssignExpr*>::iterator it = assn_exprs_.begin(); it != assn_exprs_.end(); ++it)
	{
	  CCSTAssignExpr *assn = *it;
	  assn->write(of, indent);
	}
}

CCSTAssignExpr::CCSTAssignExpr()
{
  this->unary_expr_ = 0x0; 
  this->assn_op_ = 0x0; 
  this->assn_expr_ = 0x0;
}

CCSTAssignExpr::CCSTAssignExpr(CCSTUnaryExpr *unary_expr, CCSTAssignOp *assn_op, CCSTAssignExpr *assn_expr)
{
  this->unary_expr_ = unary_expr; 
  this->assn_op_ = assn_op; 
  this->assn_expr_ = assn_expr;
}

void CCSTAssignExpr::write(std::ofstream& of, int indent)
{
  if(this->unary_expr_ == NULL || this->assn_op_ == NULL || this->assn_expr_ == NULL)
    {
      std::cout << "error\n";
      exit(0);
    }
  this->unary_expr_->write(of, indent);
  of << " ";
  this->assn_op_->write(of, 0);
  of << " ";
  this->assn_expr_->write(of, 0);
}

CCSTAssignOp::CCSTAssignOp(assign_op op)
{
  this->op_ = op;
}

CCSTAssignOp::CCSTAssignOp()
{
  //todo
}

void CCSTAssignOp::write(std::ofstream& of, int indent)
{
  switch (this->op_)
    {
    case equal_t:
      of << "=";
      break;
    case mult_eq_t:
      of << "*=";
      break;
    case div_eq_t:
      of << "/=";
      break;
    case mod_eq_t:
      of << "%=";
      break;
    case plus_eq_t:
      of << "+=";
      break;
    case minus_eq_t:
      of << "-=";
      break;
    case lshift_eq_t:
      of << "<<=";
      break;
    case rshift_eq_t:
      of << ">>=";
      break;
    case and_eq_t:
      of << "&=";
      break;
    case xor_eq_t:
      of << "^=";
      break;
    case or_eq_t:
      of << "|=";
      break;
    default:
      {
	std::cout << "error\n";
	exit(0);
      }
    }
}

CCSTUnaryOp::CCSTUnaryOp(unary_op op)
{
  this->op_ = op;
}

CCSTUnaryOp::CCSTUnaryOp()
{
  
}

void CCSTUnaryOp::write(std::ofstream& of, int indent)
{
  switch (this->op_)
    {
    case unary_bit_and_t:
      of << "&";
      break;
    case unary_mult_t:
      of << "*";
      break;
    case unary_plus_t:
      of << "+";
      break;
    case unary_minus_t:
      of << "-";
      break;
    case unary_tilde_t:
      of << "~";
      break;
    case unary_bang_t:
      of << "!";
      break;
    default:
      {
	std::cout << "error\n";
	exit(0);
      }
    }
}

CCSTTypeName::CCSTTypeName()
{
  //todo
}

CCSTTypeName::CCSTTypeName(std::vector<CCSTSpecifierQual*> spec_quals, CCSTAbstDeclarator *abs_dec)
{
  this->spec_quals_ = spec_quals; 
  this->abs_dec_ = abs_dec;
}

void CCSTTypeName::write(std::ofstream& of, int indent)
{
   for(std::vector<CCSTSpecifierQual*>::iterator it = spec_quals_.begin(); it != spec_quals_.end(); ++it)
	{
	  CCSTSpecifierQual *qual = *it;
	  qual->write(of, 0);
	  of << " ";
	}
   if(this->abs_dec_ != NULL)
     {
       this->abs_dec_->write(of, 0);
     }
}

CCSTParamTypeList::CCSTParamTypeList()
{
  //todo
}

CCSTParamTypeList::CCSTParamTypeList(CCSTParamList *p_list, bool ellipsis)
{
  this->p_list_ = p_list;
  this->ellipsis_ = ellipsis;
}

void CCSTParamTypeList::write(std::ofstream& of, int indent)
{
  if(this->p_list_ == NULL)
    {
      std::cout << "error\n";
      exit(0);
    }
  this->p_list_->write(of, indent);
  of << " , ";
  of << "...";
}

CCSTParamList::CCSTParamList()
{
  //todo
}

CCSTParamList::CCSTParamList(std::vector<CCSTParamDeclaration*> p_dec)
{
  this->p_dec_ = p_dec;
}

void CCSTParamList::write(std::ofstream& of, int indent)
{
  if(!p_dec_.empty())
    {
      p_dec_.at(0)->write(of, indent);
      
      for(std::vector<CCSTParamDeclaration*>::iterator it = p_dec_.begin()+1; it != p_dec_.end(); ++it)
	{
	  of <<", ";
	  CCSTParamDeclaration *dec = *it;
	  dec->write(of, 0);
	}
    }
}

CCSTParamDeclaration::CCSTParamDeclaration()
{
  //todo
}

CCSTParamDeclaration::CCSTParamDeclaration(std::vector<CCSTDecSpecifier*> dec_specs)
{
  this->dec_specs_ = dec_specs;
  this->dec_ = NULL;
  this->abs_dec_ = NULL;
}

CCSTParamDeclaration::CCSTParamDeclaration(std::vector<CCSTDecSpecifier*> dec_specs, CCSTDeclarator *dec)
{
  this->dec_specs_ = dec_specs; 
  this->dec_ = dec; 
  this->abs_dec_ = NULL;
}

CCSTParamDeclaration::CCSTParamDeclaration(std::vector<CCSTDecSpecifier*> dec_specs, CCSTAbstDeclarator *abs_dec)
{
  this->dec_specs_ = dec_specs; 
  this->abs_dec_ = abs_dec; 
  this->dec_ = NULL;
}

void CCSTParamDeclaration::write(std::ofstream& of, int indent)
{
  for(std::vector<CCSTDecSpecifier*>::iterator it = dec_specs_.begin(); it != dec_specs_.end(); ++it)
    {
      CCSTDecSpecifier *spec = *it;
      spec->write(of, 0);
    }
  
  if(this->dec_ == NULL && this->abs_dec_ == NULL)
    {
      // write nothing
    }
  else if(this->dec_ == NULL)
    {
      if(this->abs_dec_ == NULL)
 	{
	  std::cout << "error\n";
	  exit(0);
	}
      this->abs_dec_->write(of, 0);
    }
  else
    {
      this->dec_->write(of, 0);
    }
  
}

CCSTAbstDeclarator::CCSTAbstDeclarator()
{
  //todo
}

CCSTAbstDeclarator::CCSTAbstDeclarator(CCSTPointer *p, CCSTDirectAbstDeclarator *d_abs_dec)
{
  this->p_ = p; 
  this->d_abs_dec_ = d_abs_dec;
}

void CCSTAbstDeclarator::write(std::ofstream& of, int indent)
{
  if(this->p_ == NULL)
    {
      std::cout << "error\n";
      exit(0);
    }
  this->p_->write(of, indent);
  of << " ";
  if(this->d_abs_dec_ != NULL)
    {
      this->d_abs_dec_->write(of, 0);
    }
}

CCSTDirectAbstDeclarator::CCSTDirectAbstDeclarator()
{
  //todo
}

CCSTDirectAbstDeclarator::CCSTDirectAbstDeclarator(CCSTAbstDeclarator *abs_dec)
{
  this->abs_dec_ = abs_dec;
}

CCSTDirectAbstDeclarator::CCSTDirectAbstDeclarator(CCSTDirectAbstDeclarator *d_abs_dec, CCSTConstExpr *const_expr)
{
  this->d_abs_dec_ = d_abs_dec; 
  this->const_expr_ = const_expr;
}

CCSTDirectAbstDeclarator::CCSTDirectAbstDeclarator(CCSTDirectAbstDeclarator *d_abs_dec, CCSTParamTypeList *param_type_list)
{
  this->d_abs_dec_ = d_abs_dec; 
  this->param_type_list_ = param_type_list;
}

void CCSTDirectAbstDeclarator::write(std::ofstream& of, int indent)
{
  /*
    <direct-abstract-declarator> ::=  ( <abstract-declarator> )
    | {<direct-abstract-declarator>}? [ {<constant-expression>}? ]
    | {<direct-abstract-declarator>}? ( {<parameter-type-list>|? )
  */
  // TODO
  if(this->d_abs_dec_ == NULL && this->const_expr_ == NULL && this->param_type_list_ == NULL)
    {
      of << indentation(indent)  << "( ";
      this->abs_dec_->write(of, 0);
      of << " )";
    }
  else
    {
      if(this->d_abs_dec_ == NULL)
	{
	  std::cout << "error\n";
	  exit(0);
	}
      this->d_abs_dec_->write(of, indent);
      if(this->const_expr_ == NULL)
	{
	  
	  if(this->param_type_list_ == NULL)
	    {
	      std::cout << "error\n";
	      exit(0);
	    }
	  of << indentation(indent)  << "( ";
	  this->param_type_list_->write(of, 0);
	  of << " )";
	}
      else
	{
	  of << indentation(indent) << "[ ";
	  this->const_expr_->write(of, 0);
	  of << " ] ";
	}
    }
}

CCSTEnumSpecifier::CCSTEnumSpecifier(const std::string& id, CCSTEnumeratorList *el)
{
  this->id_ = id; 
  this->el_ = el;
}

CCSTEnumSpecifier::CCSTEnumSpecifier(const std::string& id)
{
  this->id_ = id;
  this->el_ = NULL;
}

CCSTEnumSpecifier::CCSTEnumSpecifier(CCSTEnumeratorList *el)
{
  this->el_ = el;
  this->id_ = "";
}

CCSTEnumSpecifier::CCSTEnumSpecifier()
{
  //todo
}

void CCSTEnumSpecifier::write(std::ofstream& of, int indent)
{
  if(this->el_ == NULL)
    {
      of << indentation(indent) << "enum ";
      of << this->id_;
    }
  else
    {
      of << indentation(indent) << "enum ";
      of << this->id_;
      of << " {\n";
      this->el_->write(of, 1);
      of << "\n}";
    }
}

CCSTEnumeratorList::CCSTEnumeratorList()
{
  //todo
}

CCSTEnumeratorList::CCSTEnumeratorList(std::vector<CCSTEnumerator*> *list)
{
  this->list_ = list;
}

void CCSTEnumeratorList::write(std::ofstream& of, int indent)
{
  if(!list_->empty())
    {
      list_->at(0)->write(of, indent);
      for(std::vector<CCSTEnumerator*>::iterator it = list_->begin()+1; it != list_->end(); ++it)
	{
	  of << ",\n";
	  CCSTEnumerator *l = *it;
	  l->write(of, indent);
	}
    }
}

CCSTEnumerator::CCSTEnumerator(const std::string& id, CCSTConstExpr *ce)
{
  this->id_ = id; 
  this->ce_ = ce;
}

CCSTEnumerator::CCSTEnumerator(const std::string& id)
{
  this->id_ = id; 
  this->ce_ = NULL;
}

void CCSTEnumerator::write(std::ofstream& of, int indent)
{
  if(this->ce_ == NULL)
    {
      of << indentation(indent) << this->id_;
    }
  else
    {
      of << indentation(indent) << this->id_;
      of << " = ";
      this->ce_->write(of, 0);
    }
}

CCSTTypedefName::CCSTTypedefName(const std::string& name)
{
  this->id_ = name;
}

void CCSTTypedefName::write(std::ofstream& of, int indent)
{
  of << this->id_ << indentation(indent);
}


void CCSTDeclaration::write(std::ofstream& of, int indent)
{
  for(std::vector<CCSTDecSpecifier*>::iterator it = specifier_.begin(); it != specifier_.end(); ++it)
    {
      CCSTDecSpecifier *dec_spec = *it;
      dec_spec->write(of, indent);
      //of << " ";
    }


  for(std::vector<CCSTMacro*>::iterator it = attributes_.begin(); it != attributes_.end(); ++it) {
    CCSTMacro *mac = *it;
    std::cout << "--> Writing linkage attribute\n";
    mac->write(of, 0);
  }

  for(std::vector<CCSTInitDeclarator*>::iterator it = decs_.begin(); it != decs_.end(); ++it)
    {
      CCSTInitDeclarator *init_dec = *it;
      init_dec->write(of, 0);
      //of << " ";
    }
  of << ";";
  of << "\n";
}

CCSTInitDeclarator::CCSTInitDeclarator(CCSTDeclarator *dec, CCSTInitializer *init)
{
  this->dec_ = dec; 
  this->init_ = init;
}

CCSTInitDeclarator::CCSTInitDeclarator(CCSTDeclarator *dec)
{
  this->dec_ = dec; 
  this->init_ = NULL;
}

void CCSTInitDeclarator::write(std::ofstream& of, int indent)
{
  // does inheritence cover just declarator case?
  if(this->dec_ == NULL || this->init_ == NULL)
    {
      std::cout << "error\n";
      exit(0);
    }
  this->dec_->write(of, indent);
  of << " = ";
  this->init_->write(of, 0);
}

CCSTInitializer::CCSTInitializer(CCSTAssignExpr *assn_expr)
{
  this->assn_expr_ = assn_expr;
  this->init_list_ = 0x0;
}

CCSTInitializer::CCSTInitializer(CCSTInitializerList *init_list)
{
  this->init_list_ = init_list;
  this->assn_expr_ = 0x0;
}

void CCSTInitializer::write(std::ofstream& of, int indent)
{
  /*
    
<initializer> ::= <assignment-expression>
                | { <initializer-list> }
                | { <initializer-list> , }
   */

  // TODO
  if(this->assn_expr_ == NULL)
    {
      if(this->init_list_ == NULL)
	{
	  std::cout << "error\n";
	  exit(0);
	}
      of << indentation(indent) << "{ ";
      this->init_list_->write(of, 0);
      of << " }";
    }
  else
    {
      this->assn_expr_->write(of, indent);
    }
}

CCSTInitializerList::CCSTInitializerList()
{
  //todo
}

CCSTInitializerList::CCSTInitializerList(std::vector<CCSTInitializer*> init_list)
{
  this->init_list_ = init_list;
}

void CCSTInitializerList::write(std::ofstream& of, int indent)
{
  // TODO
  for(std::vector<CCSTInitializer*>::iterator it = init_list_.begin(); it != init_list_.end(); ++it)
    {
      CCSTInitializer *init = *it;
      init->write(of, 0);
      of << ", ";
    }
  
}

CCSTPreprocessor::CCSTPreprocessor(const std::string&path, bool relative)
{
  this->pathname = new std::string(path);
  this->relative = relative;
}

void CCSTPreprocessor::write(std::ofstream& of, int indent)
{
	of << "#include ";
	if (this->relative)
		of << "\"";
	else
		of << "<";
	of << this->pathname;

	if (this->relative)
		of << "\"\n";
	else
		of << ">\n";
}

CCSTCompoundStatement::CCSTCompoundStatement(std::vector<CCSTDeclaration*> decs, std::vector<CCSTStatement*> s)
{
  this->declarations_ = decs; 
  this->statements_ = s;
}

void CCSTCompoundStatement::write(std::ofstream& of, int indent)
{

  for(std::vector<CCSTDeclaration*>::iterator it = this->declarations_.begin(); it != declarations_.end(); ++it)
    {
      CCSTDeclaration *dec = *it;
      
      dec->write(of, indent);
      //    of << "\n";
    }
  for(std::vector<CCSTStatement*>::iterator it = this->statements_.begin(); it != statements_.end(); ++it)
    {
      CCSTStatement *state = *it;
      state->write(of, indent);
      //   of << "\n";
    }
}

CCSTPlainLabelStatement::CCSTPlainLabelStatement(const std::string& id, CCSTStatement *stmnt)
{
  this->id_ = id; 
  this->stmnt_ = stmnt;
}

void CCSTPlainLabelStatement::write(std::ofstream& of, int indent)
{
  of << this->id_ << ":\n";
  this->stmnt_->write(of, indent);
}

CCSTCaseStatement::CCSTCaseStatement(CCSTCondExpr *c, CCSTStatement *body)
{
  this->case_label_ = c; 
  this->body_ = body;
}

void CCSTCaseStatement::write(std::ofstream& of, int indent)
{
  of << indentation(indent) << "case ";
  this->case_label_->write(of, 0);
  of << ":\n";
  this->body_->write(of, indent+1);
  of << "\n";
}

void CCSTDefaultLabelStatement::write(std::ofstream& of, int indent)
{
  of << indentation(indent) << "default:\n";
  this->body_->write(of, indent+1);
  of << "\n";
}

CCSTDefaultLabelStatement::CCSTDefaultLabelStatement(CCSTStatement* body)
{
  this->body_ = body;
}

CCSTExprStatement::CCSTExprStatement()
{
}

CCSTExprStatement::CCSTExprStatement(CCSTExpression *expr)
{
  this->expr_ = expr;
}

void CCSTExprStatement::write(std::ofstream& of, int indent)
{
  // weird why the semicolon with no expression
  if(this->expr_ != NULL)
    {
      this->expr_->write(of, indent);
    }
  of << ";\n";
}

CCSTIfStatement::CCSTIfStatement(CCSTExpression *cond, CCSTStatement *body)
{
  this->cond_ = cond; 
  this->body_ = body;
}

void CCSTIfStatement::write(std::ofstream& of, int indent)
{
  of << indentation(indent) << "if (";
  if(this->cond_ == NULL)
    {
      std::cout << "error\n";
      exit(0);
    }
  this->cond_->write(of, 0);
  of << ") {\n";
  this->body_->write(of, indent+1);
  of << indentation(indent) << "}\n";
}

CCSTIfElseStatement::CCSTIfElseStatement(CCSTExpression *cond, CCSTStatement *if_body, CCSTStatement *else_body)
{
  this->cond_ = cond; 
  this->if_body_ = if_body; 
  this->else_body_ = else_body;
}

void CCSTIfElseStatement::write(std::ofstream& of, int indent)
{
  of << indentation(indent) << "if";
  of << "( ";
  if(this->cond_ == NULL)
    {
      std::cout << "error\n";
      exit(0);
    }
  this->cond_->write(of, 0);
  of << " )";
  of << " {\n";
  this->if_body_->write(of, indent+1);
  of << "\n";
  of << indentation(indent) << "}";
  of << " else";
  of << " {\n";
  this->else_body_->write(of, indent+1);
  of << "\n" << indentation(indent) << "}\n";
}

CCSTSwitchStatement::CCSTSwitchStatement(CCSTExpression *expr, CCSTStatement *body)
{
  this->expr_ = expr; 
  this->body_ = body;
}

void CCSTSwitchStatement::write(std::ofstream& of, int indent)
{
  if(this->expr_ == NULL)
    {
      std::cout << "error\n";
      exit(0);
    }
  of << indentation(indent) << "switch ";
  of << "(";
  this->expr_->write(of, 0);
  of << ")";
  of << " {\n";
  this->body_->write(of, indent+1); // all cases?
  of << indentation(indent) << "}\n";
}

CCSTWhileLoop::CCSTWhileLoop(CCSTExpression *cond, CCSTStatement *body)
{
  this->cond_ = cond; 
  this->body_ = body;
}

void CCSTWhileLoop::write(std::ofstream& of, int indent)
{
  of << indentation(indent) << "while";
  of << "( ";
  this->cond_->write(of, 0);
  of << " )";
  of << " {\n";
  this->body_->write(of, indent+1);
  of << "\n";
  of << indentation(indent) << "}";
}

CCSTDoLoop::CCSTDoLoop(CCSTStatement *body, CCSTExpression *cond)
{
  this->body_ = body; 
  this->cond_ = cond;
}

void CCSTDoLoop::write(std::ofstream& of, int indent)
{
  of << indentation(indent) << "do ";
  of << "{\n";
  this->body_->write(of, indent+1);
  of << "\n" << indentation(indent) << "} ";
  of << "while";
  of << "( ";
  this->cond_->write(of, 0);
  of << " )";
  of << ";\n";
}

CCSTForLoop::CCSTForLoop(CCSTExpression *init, CCSTExpression *cond, CCSTExpression *up, CCSTStatement *body)
{
  this->init_ = init; 
  this->cond_ = cond; 
  this->up_ = up; 
  this->body_ = body;
}

void CCSTForLoop::write(std::ofstream& of, int indent)
{
  of << indentation(indent) << "for (";
  if(this->init_ != NULL)
    this->init_->write(of, 0);
  of << ";";
  if(this->cond_ != NULL)
    this->cond_->write(of, 0);
  of << ";";
  if(this->up_ != NULL)
    this->up_->write(of, 0);
  of << ") {\n";
  this->body_->write(of, indent+1);
  of << indentation(indent) << "}\n";
}

CCSTGoto::CCSTGoto(const std::string& id)
{
  this->identifier_ = id;
}

void CCSTGoto::write(std::ofstream& of, int indent)
{
  of << indentation(indent) << "goto " << this->identifier_ << ";\n";
}

CCSTContinue::CCSTContinue()
{
}

void CCSTContinue::write(std::ofstream& of, int indent)
{
  of << indentation(indent) << "continue;" ;// write continue ;
}

CCSTBreak::CCSTBreak()
{
}

void CCSTBreak::write(std::ofstream& of, int indent)
{
  of << indentation(indent) << "break;";
}

CCSTReturn::CCSTReturn()
{
  this->expr_ = NULL;
}
CCSTReturn::CCSTReturn(CCSTExpression *expr)
{
  this->expr_ = expr;
}

void CCSTReturn::write(std::ofstream& of, int indent)
{
  if(this->expr_ == NULL)
    {
      of << indentation(indent)  << "return;";
    }
  else
    {
      of << indentation(indent) << "return ";
      this->expr_->write(of, 0);
      of << ";\n";
    }
}

void CCSTMacro::write(std::ofstream& of, int indent)
{
  std::vector<CCSTAssignExpr*> args = this->data_args;

  of << indentation(indent) << this->macro_name << "(";

  if(!args.empty())
    {
      args.at(0)->write(of, 0);

      for(std::vector<CCSTAssignExpr*>::iterator it = args.begin()+1; it != args.end(); ++it)
	  {
	    of <<", ";
	    CCSTAssignExpr *arg = *it;
	    arg->write(of, 0);
	  }
    }
  if (this->is_terminal)
	  of << ");\n";
  else
	  of << ")\n";
}

// FIXME: How to handle this efficiently for multiple
// levels? Surely we cannot waste memory by allocating.
// May be some macro magic?
const std::vector<std::string> indents = {
"",			    //0
"\t",			    //1
"\t\t",			    //2
"\t\t\t",		    //3
"\t\t\t\t",		    //4
"\t\t\t\t\t",		//5
"\t\t\t\t\t\t",		//6
"\t\t\t\t\t\t\t",	//7
"\t\t\t\t\t\t\t\t"	//8
};

const std::string indentation(unsigned int level)
{
  if (level < indents.size())
    return indents[level];
  else
    return indents.back();
}
