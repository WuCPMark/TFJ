#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <iostream>

#include "ast.h"
#include "codegen.h"
#include "banerjee.h"


static int constantId = 0;
static int Id = 0;

using namespace std;
using namespace llvm;

//enum op_t {ADD=0, SUB, MUL, DIV, LT, LTE, GT, GTE, EQ, NEQ, ASSIGN, SEL}; 

static const std::string scalarOpNames[] = {"+", "-", "*", "/",
					    "<", "<=", ">", ">=", 
					    "==", "!="};

#define NUM_VEC_OPS 11
static const std::string vecOpNames[NUM_VEC_OPS] = 
  {"vecAdd", "vecSub", "vecMul", "vecDiv", 
   "vecLT", "vecLTE", "vecGT", "vecGTE",
   "vecEq", "vecNeq", "vecSel"};

int getConstantId()
{
  int rc = constantId;
  constantId++;
  return rc;
}

int getId()
{
  int rc = Id;
  Id++;
  return rc;
}


astSubscript::astSubscript()
{
  offset = 0;
  isIndirect = false;
  indirectRef = 0;
}

astSubscript::astSubscript(int offset)
{
  this->offset = offset;
  isIndirect = false;
  indirectRef = 0;
}

astSubscript::astSubscript(MemRef *indirectRef)
{
  offset = 0;
  isIndirect = true;
  this->indirectRef = indirectRef;
}

bool AssignExpr::permuteIt(int first, int last,
		      vector<int> &assigned,
		      vector<int> &unassigned)
{
  int s = unassigned.size();
  for(int i = 0; i < s; i++)
    {
      vector<int> a;
      vector<int> u;

      for(vector<int>::iterator vit = assigned.begin();
	  vit != assigned.end(); vit++)
	{
	  a.push_back(*vit);
	}
      for(vector<int>::iterator vit = unassigned.begin();
	  vit != unassigned.end(); vit++)
	{
	  u.push_back(*vit);
	}
      
      int f = u.back();
      u.pop_back();
      
      a.push_back(f);
      
      bool r = permuteIt(first, last, a, u);
      if(r == true) {
	return true;
      }
    }

  if(s == 0) {
    ForStmt* tempLoops[MAX_DEPTH+1];

    for(int i = 0; i < assigned.size(); i++)
      {
	int idx = assigned[i];
	tempLoops[first+i] = inLoops[idx]; 
      }

    if(SIMDizable(tempLoops[last])) 
      {
	dbgPrintf("found good permutation\n");
	for(int i = 0; i < assigned.size(); i++)
	  dbgPrintf("%d ", assigned[i]);
	dbgPrintf("\n");

	for(int i = first; i <= last; i++)
	  {
	    inLoops[i] = tempLoops[i];
	  }
	return true;
      } 
    else 
      {
	return false;
      }
  }

  return false;
}


void AssignExpr::findBestLoopPermutation(int first, int last)
{
  dbgPrintf("find best with first=%d, last=%d\n", first, last);
  for(int i = first; i <= last; i++)
    {
      vector<int> assigned;
      vector<int> unassigned;

      assigned.push_back(i);
      for(int j = first; j <= last; j++)
	{
	  if(j==i) 
	    {
	      continue;
	    }
	  else 
	    {
	      unassigned.push_back(j);
	    }
	}

      permuteIt(first, last, assigned, unassigned);


    }
  

  //ae->SIMDizable(ll);
  
}


Value * astSubscript::getLLVM(llvm::IRBuilder<> *llvmBuilder,
			      std::map<ForStmt*, llvm::PHINode*> &iVarMap)
{
  //if(llvmBuilder == 0)
  // dbgPrintf("llvmBuilder = %p\n", llvmBuilder);

  Value *llvmOffset = 
    ConstantInt::get(Type::getInt32Ty(getGlobalContext()),offset);
  Value *llvmZero = 
    ConstantInt::get(Type::getInt32Ty(getGlobalContext()),0);
  Value *llvmIdx =
    llvmBuilder->CreateAdd(llvmOffset, llvmZero);
  
  for(std::map<ForStmt*, int>::iterator it = coeffs.begin();
      it != coeffs.end(); it++)
    {
      ForStmt *f = it->first;
      if(it->second != 0)
	{
	  PHINode *phiIndVar = NULL;
	  phiIndVar = iVarMap[it->first];
	  if(phiIndVar == 0)
	    {
	      dbgPrintf("(mapSize=%lu): couldn't find induction variable (%s)\n",
		     iVarMap.size(),(it->first)->inductionVar.c_str());
	      exit(-1);
	    }
	  
	  Value *llvmCoeff = 
	    ConstantInt::get(Type::getInt32Ty(getGlobalContext()),it->second);
	 
	  Value *llvmAddrMul =
	    llvmBuilder->CreateMul(phiIndVar, llvmCoeff);
	    	    	 	  
	  llvmIdx = llvmBuilder->CreateAdd(llvmIdx, llvmAddrMul);
	}
    }
    return llvmIdx;
  
}


 
std::string astSubscript::getLinearString()
{
  std::stringstream ss0;
  ss0 << offset;
  std::string s;
  s += "( " + ss0.str();;
  for(std::map<ForStmt*, int>::iterator it = coeffs.begin();
      it != coeffs.end(); it++)
    {
      if(it->second != 0)
	{
	  std::stringstream ss1;
	  ss1 << it->second;
	  s += "+ " + ss1.str() + "*" + (*it->first).inductionVar.c_str();
	}
    }
  s += " )";
  return s;
}

Value * astSubscript::getLLVM(llvm::IRBuilder<> *llvmBuilder,
			      std::map<ForStmt*, llvm::PHINode*> &iVarMap, 
			      llvm::PHINode *iVar,
			      int amt)
{
  Value *llvmOffset = 
    ConstantInt::get(Type::getInt32Ty(getGlobalContext()),offset);
  Value *llvmZero = 
    ConstantInt::get(Type::getInt32Ty(getGlobalContext()),0);
  Value *llvmIdx =
    llvmBuilder->CreateAdd(llvmOffset, llvmZero);

  for(std::map<ForStmt*, int>::iterator it = coeffs.begin();
      it != coeffs.end(); it++)
    {
      if(it->second != 0)
	{
	  Value *phiIndVar = iVarMap[it->first];
	  
	  if(phiIndVar==iVar)
	    {
	      Value *llvmUnrollAmt = ConstantInt::get(Type::getInt32Ty(getGlobalContext()),amt);			 
	      phiIndVar = llvmBuilder->CreateAdd(phiIndVar, llvmUnrollAmt);
	    }						   
	  
	  Value *llvmCoeff = 
	    ConstantInt::get(Type::getInt32Ty(getGlobalContext()),it->second);
	  
	  Value *llvmAddrMul =
	    llvmBuilder->CreateMul(phiIndVar, llvmCoeff);
	  
	  llvmIdx = llvmBuilder->CreateAdd(llvmIdx, llvmAddrMul);
	}
    }
  
  return llvmIdx;
  
}
std::string astSubscript::getLinearString(ForStmt *l, int amt)
{
  std::stringstream ss0;
  ss0 << offset;
  std::string s;
  s += "( " + ss0.str();;
  for(std::map<ForStmt*, int>::iterator it = coeffs.begin();
      it != coeffs.end(); it++)
    {
      std::stringstream ss1;
      ss1 << it->second;
      if(l == it->first)
	{
	  if(it->second!=0)
	    {
	      s += "+ " + ss1.str() + "*" + 
		"(" + int2string(amt) + "+" + 
		(*it->first).inductionVar.c_str() + ")";
	    }
	}
      else
	{
	  if(it->second!=0)
	    s += "+ " + ss1.str() + "*" + (*it->first).inductionVar.c_str();
	}
    }
  s += " )";
  return s;
}

bool astSubscript::ivarPresent(ForStmt *l)
{
  if(coeffs.find(l) != coeffs.end())
    {
      /* coeffs of zero show up sometimes */
      return (coeffs[l] != 0);
    }
  return false;
}

/* compute stride for subscripted variable l */
int astSubscript::computeStride(ForStmt *l)
{
  return coeffs[l];
}

std::string astSubscript::getString()
{
  std::stringstream ss0;
  ss0 << offset;
  std::string s = ss0.str();
  for(std::map<ForStmt*, int>::iterator it = coeffs.begin();
      it != coeffs.end(); it++)
    {
      std::stringstream ss1;
      ss1 << it->second;
      s += "+ " + ss1.str() + "*" + (*it->first).inductionVar.c_str();
    }
  return s;
}

void astSubscript::print()
{
  if(isIndirect)
    {
      indirectRef->print(0);
    }
  else
    {
      dbgPrintf("%d ", offset);
      for(std::map<ForStmt*, int>::iterator it = coeffs.begin();
	  it != coeffs.end(); it++)
	{
	  dbgPrintf("+ %d *%s ", 
		 (it->second),
		 (*it->first).inductionVar.c_str());
	}
    }
}

int astSubscript::checkIfExclusiveIvar(ForStmt *f)
{
  int nonZeroCoeffs=0;
  int found = -1;
  for(std::map<ForStmt*, int>::iterator it = coeffs.begin();
      it != coeffs.end(); it++)
    {
      if(it->first == f)
	found = it->second;

      if(it->second != 0)
	nonZeroCoeffs++;
    }
  if(nonZeroCoeffs > 1) 
    return -1;
  else 
    return found;
}

void Var::setDim(int dim) 
{
  this->dim=dim;
  for(int i = 0; i < dim; i++)
    {
      std::string v = varName + "_dim_" + int2string(i);
      dimNames.push_back(v);
    }
}  

Node::Node() 
{
  llvmInit = false;
  Id = getId();
  parent=0;
}

double Expr::cost(ForStmt **loops) 
{
  return 0.0;
}

int Expr::computeStride(ForStmt *l)
{
  return 0;
}

std::string Expr::emitTempVars(tempVar_t t, ForStmt *l,int vecLen)
{
  return std::string();
}


Value *Expr::emitTempLLVMVars(tempVar_t t, IRBuilder<> *llvmBuilder,
			      std::map<ForStmt*, PHINode*> &iVarMap,
			      PHINode *iVar, ForStmt *l, int vecLen)
{
  dbgPrintf("%s:%d this shouldn't happen\n",__FILE__, __LINE__);
  return ConstantInt::get(Type::getInt32Ty(getGlobalContext()),0);
}



void Expr::collectMemRefs(std::list<MemRef*> &mList)
{

}

void AssignExpr::print(int depth)
{
  //dbgPrintf("\n");
  for(int i=0;i<depth;i++) 
    dbgPrintf("  ");
  if(left) left->print(depth);
  dbgPrintf("=");
  if(right) right->print(depth);
  dbgPrintf("\n");
}

void ForStmt::print(int depth)
{
  for(int i=0;i<depth;i++) 
    dbgPrintf("  ");
  dbgPrintf("for(int %s=%d; %s < %d; %s += %d)\n",
	 inductionVar.c_str(), lowerBound,
	 inductionVar.c_str(), upperBound-1,
	 inductionVar.c_str(), step);

  for(int i=0;i<depth;i++) 
    dbgPrintf("  ");
  dbgPrintf("{\n");
  for(std::list<Expr*>::iterator it = body.begin();
      it != body.end(); it++)
    {
      (*it)->print(depth+1);
    }
  for(int i=0;i<depth;i++) 
    dbgPrintf("  ");
  dbgPrintf("}\n");
}

void ForStmt::getAllStmts(std::list<AssignExpr*> &toTest)
{
 for(std::list<Expr*>::iterator it0 = body.begin();
      it0 != body.end(); it0++)
   {
     AssignExpr *ae = dynamic_cast<AssignExpr*>(*it0);
     ForStmt *fs = dynamic_cast<ForStmt*>(*it0);

     if(ae) toTest.push_back(ae);
     else if(fs) fs->getAllStmts(toTest);

     assert( ae || fs);
   }
}


void ForStmt::getDepGraph(std::list<depGraph*> &graph)
{
 for(std::list<Expr*>::iterator it0 = body.begin();
      it0 != body.end(); it0++)
   {
     AssignExpr *ae = dynamic_cast<AssignExpr*>(*it0);
     ForStmt *fs = dynamic_cast<ForStmt*>(*it0);
     
     if(ae) graph.push_back(ae->dG);
     else if(fs) fs->getDepGraph(graph);

     assert( ae || fs);
   }
  
}

int ForStmt::populateLoops(AssignExpr *ae)
{
  if(parent==NULL) {
      ae->inLoops[1] = this;
      /* dbgPrintf("inductionVar = %s @ %d\n",
	 inductionVar.c_str(), 1); */
      return 1;
    }
  else
    {
      ForStmt *fs = dynamic_cast<ForStmt*>(parent);
      if(fs)
       { 
	 int p =1+fs->populateLoops(ae);
	 ae->inLoops[p] = this;
	 /* dbgPrintf("inductionVar = %s @ %d\n",
	    inductionVar.c_str(), p); */
	 return p;
       }
   }
  
 
 assert(true==false);
 return -1;
}

void CompoundExpr::print(int depth)
  {
    dbgPrintf("(");
    if(left) left->print(depth);
    dbgPrintf(")");
    dbgPrintf("%s", scalarOpNames[op].c_str());
    dbgPrintf("(");
    if(right) right->print(depth);
    dbgPrintf(")");
  }  

void CompoundExpr::checkIfSameVar(Ref *x, std::list<Ref*> &sameVarList)
  {
    /* What happens if same var is on left twice same
     * level of loop nest ?? */
    Ref *r = dynamic_cast<Ref*>(right);
    Ref *l = dynamic_cast<Ref*>(left);
    //dbgPrintf("r=%p,l=%p\n", r,l);
    if(l==0)
      {
	CompoundExpr *ll = dynamic_cast<CompoundExpr*>(left);
	if(ll)
	  ll->checkIfSameVar(x ,sameVarList);
      }
    else
      {
	if(l->variable == x->variable)
	  {
	    sameVarList.push_back(l);
	  }
      }

    if(r==0)
      {
	CompoundExpr *rr = dynamic_cast<CompoundExpr*>(right);
	if(rr)
	  rr->checkIfSameVar(x,sameVarList);
      }
    else
      {
	if(r->variable == x->variable)
	  {
	    sameVarList.push_back(r);
	    //dbgPrintf("same var right = %s\n", r->getName().c_str());
	  }
      }

  }
void SelectExpr::checkIfSameVar(Ref *x, std::list<Ref*> &sameVarList)
  {
    //exit(-1);
    /* What happens if same var is on left twice same
     * level of loop nest ?? */
    Ref *r = dynamic_cast<Ref*>(right);
    Ref *s = dynamic_cast<Ref*>(sel);
    Ref *l = dynamic_cast<Ref*>(left);
    dbgPrintf("r=%p,l=%p,s = %p\n", r,l,s);
    if(l==0)
      {
	CompoundExpr *ll = dynamic_cast<CompoundExpr*>(left);
	if(ll)
	  ll->checkIfSameVar(x ,sameVarList);
	else
	  {
	    dbgPrintf("aborting @ %d\n", __LINE__);  
	    exit(-1);
	  }
      }
    else
      {
	if(l->variable == x->variable)
	  {
	    sameVarList.push_back(l);
	  }
      }
    if(s==0)
      {
	CompoundExpr *ss = dynamic_cast<CompoundExpr*>(sel);
	if(ss)
	  ss->checkIfSameVar(x,sameVarList);
	else
	  {
	    dbgPrintf("aborting @ %d\n", __LINE__);
	    exit(-1);
	  }
      }
    else
      {
	if(s->variable == x->variable)
	  {
	    sameVarList.push_back(s);
	  }
      }

    if(r==0)
      {
	CompoundExpr *rr = dynamic_cast<CompoundExpr*>(right);
	if(rr)
	  rr->checkIfSameVar(x,sameVarList);
	/*
	else
	  {
	    dbgPrintf("aborting @ %d\n", __LINE__);
	    exit(-1);
	  }
	*/
      }
    else
      {
	if(r->variable == x->variable)
	  {
	    sameVarList.push_back(r);
	    //dbgPrintf("same var right = %s\n", r->getName().c_str());
	  }
      }

  }

void UnaryIntrin::checkIfSameVar(Ref *x, std::list<Ref*> &sameVarList)
  {
    //exit(-1);
    /* What happens if same var is on left twice same
     * level of loop nest ?? */
    Ref *l = dynamic_cast<Ref*>(left);
    if(l==0)
      {
	CompoundExpr *ll = dynamic_cast<CompoundExpr*>(left);
	if(ll)
	  ll->checkIfSameVar(x ,sameVarList);
	else
	  {
	    dbgPrintf("aborting @ %d\n", __LINE__);  
	    exit(-1);
	  }
      }
    else
      {
	if(l->variable == x->variable)
	  {
	    sameVarList.push_back(l);
	  }
      }
  }

int CompoundExpr::computeStride(ForStmt *l)
{
  int lrc=-1,rrc=-1;
  if(left)
    lrc = left->computeStride(l);
  if(right)
    rrc = right->computeStride(l);
  
  bool leftValid = (lrc==0)||(lrc==1);
  bool rightValid = (rrc==0)||(rrc==1);

  return (leftValid && rightValid) ? 1 : -1;
}


double CompoundExpr::cost(ForStmt **c)
{
  assert(left); assert(right);
  return left->cost(c) + right->cost(c);
}


std::string CompoundExpr::emit_unrolled_c(ForStmt *l, int amt)
{
  std::string asCpp;
  asCpp += "(";
  if(left) asCpp += left->emit_unrolled_c(l,amt);
  asCpp += ")";
  asCpp += scalarOpNames[op];
  asCpp += "(";
  if(right) asCpp += right->emit_unrolled_c(l,amt);
  asCpp += ")";
  return asCpp;
}


std::string CompoundExpr::emit_vectorized_c(ForStmt *l)
{
  std::string asCpp;
 
  asCpp += vecIntrinsics[op] + "(";
  asCpp += "(";
  if(left) asCpp += left->emit_vectorized_c(l);
  asCpp += ")";
  asCpp += ",";
  asCpp += "(";
  if(right) asCpp += right->emit_vectorized_c(l);
  asCpp += ")";
  asCpp += ")";
  return asCpp;
}

std::string SelectExpr::emit_c()
{
  assert(left); assert(right); assert(sel);
  std::string asCpp;
  asCpp += "( (";
  asCpp += sel->emit_c() + " ) ? ";
  asCpp += "(";
  asCpp += left->emit_c();
  asCpp += ")";
  asCpp += " : ";
  asCpp += "(";
  asCpp += right->emit_c();
  asCpp += ")";
  asCpp += " )";
  return asCpp;
}

std::string UnaryIntrin::emit_c()
{ 
  std::string asCpp;
  assert(left);
  asCpp += "(";
  asCpp += intrinString;
  asCpp += "(";
  asCpp += left->emit_c();
  asCpp += ")";
  asCpp += ")";
  return asCpp;
}

Value *SelectExpr::emit_llvm(llvm::IRBuilder<> *llvmBuilder,
			     std::map<ForStmt*, llvm::PHINode*> &iVarMap)
{  
  Value *myValue;
  
  std::list<MemRef*> intCheckList;
  if(left) left->collectMemRefs(intCheckList);
  if(right) right->collectMemRefs(intCheckList);
  if(sel) sel->collectMemRefs(intCheckList);

  isIntegerOp = foundIntegerOps(intCheckList);

  assert(left); assert(right); assert(sel);
  selValue = sel->emit_llvm(llvmBuilder,iVarMap);
  leftValue = left->emit_llvm(llvmBuilder,iVarMap);
  rightValue = right->emit_llvm(llvmBuilder,iVarMap);

  const Type *leftType = leftValue->getType();
  const Type *rightType = rightValue->getType();
  
 
  if(leftType != rightType)
    {
      dbgPrintf("vector type mismatch!\n");
      exit(-1);
    }
  myValue = llvmBuilder->CreateSelect(selValue, leftValue, rightValue);
    
  llvmValues.push_back(myValue);
  return myValue;
  
}

Value *UnaryIntrin::emit_llvm(llvm::IRBuilder<> *llvmBuilder,
			     std::map<ForStmt*, llvm::PHINode*> &iVarMap)
{  
  Value *myValue;
  
  std::list<MemRef*> intCheckList;
  if(left) left->collectMemRefs(intCheckList);
  isIntegerOp = foundIntegerOps(intCheckList);

  assert(left); 
  leftValue = left->emit_llvm(llvmBuilder,iVarMap);

  const Type *leftType = leftValue->getType();
   
  myValue = leftValue;
  //myValue = llvmBuilder->CreateSelect(selValue, leftValue, rightValue);
    
  llvmValues.push_back(myValue);
  return myValue;
  
}


Value *SelectExpr::emit_unrolled_llvm(llvm::IRBuilder<> *llvmBuilder,
				      std::map<ForStmt*, llvm::PHINode*> &iVarMap,
				      PHINode *iVar,
				      int amt)
{
 Value *myValue;

  std::list<MemRef*> intCheckList;
  assert(left); assert(right);

  if(left) left->collectMemRefs(intCheckList);
  if(right) right->collectMemRefs(intCheckList);
  if(sel) sel->collectMemRefs(intCheckList);

  isIntegerOp = foundIntegerOps(intCheckList);

  selValue = sel->emit_unrolled_llvm(llvmBuilder,iVarMap,iVar,amt);
  leftValue = left->emit_unrolled_llvm(llvmBuilder,iVarMap,iVar,amt);
  rightValue = right->emit_unrolled_llvm(llvmBuilder,iVarMap,iVar,amt); 

  myValue = llvmBuilder->CreateSelect(selValue, leftValue, rightValue);
  llvmValues.push_back(myValue);
  return myValue;
}

Value *UnaryIntrin::emit_unrolled_llvm(llvm::IRBuilder<> *llvmBuilder,
				       std::map<ForStmt*, llvm::PHINode*> &iVarMap,
				       PHINode *iVar,
				       int amt)
{
 Value *myValue;

  std::list<MemRef*> intCheckList;
  assert(left); 
  if(left) left->collectMemRefs(intCheckList);
  isIntegerOp = foundIntegerOps(intCheckList);

  leftValue = left->emit_unrolled_llvm(llvmBuilder,iVarMap,iVar,amt);
  //  myValue = llvmBuilder->CreateSelect(selValue, leftValue, rightValue);
  myValue = leftValue;
  llvmValues.push_back(myValue);
  return myValue;
}

Value *SelectExpr::emit_vectorized_llvm(llvm::IRBuilder<> *llvmBuilder,
					std::map<ForStmt*, llvm::PHINode*> &iVarMap,
					PHINode *iVar,
					bool doAligned,
					int vecLen)
{
  dbgPrintf("not implemented @ %s:%d\n", __FILE__,
	 __LINE__);
  return NULL;
}

Value *UnaryIntrin::emit_vectorized_llvm(llvm::IRBuilder<> *llvmBuilder,
					std::map<ForStmt*, llvm::PHINode*> &iVarMap,
					PHINode *iVar,
					bool doAligned,
					int vecLen)
{
  leftValue = left->emit_vectorized_llvm(llvmBuilder,iVarMap,iVar, doAligned, vecLen);
  llvmValues.push_back(leftValue);
  return leftValue;
  /*
  dbgPrintf("not implemented @ %s:%d\n", __FILE__,
	 __LINE__);
  return NULL;
  */
}

int SelectExpr::computeStride(ForStmt *l)
{
  int src=-1,lrc=-1,rrc=-1;
  if(sel)
    src = sel->computeStride(l);
  if(left)
    lrc = left->computeStride(l);
  if(right)
    rrc = right->computeStride(l);
  
  bool leftValid = (lrc==0)||(lrc==1);
  bool rightValid = (rrc==0)||(rrc==1);
  bool selValid = (src==0) || (src==1);

  return (leftValid && rightValid && selValid) ? 1 : -1;
}

int UnaryIntrin::computeStride(ForStmt *l)
{
  int lrc=-1;
  if(left)
    lrc = left->computeStride(l);
  bool leftValid = (lrc==0)||(lrc==1);
  return (leftValid) ? 1 : -1;
}

std::string SelectExpr::emit_unrolled_c(ForStmt *l, int amt)
{
  assert(left); assert(right); assert(sel);
  std::string asCpp;
  asCpp += "(";
  asCpp += sel->emit_unrolled_c(l,amt) + " ? ";
  asCpp += "(";
  asCpp += left->emit_unrolled_c(l,amt);
  asCpp += ")";
  asCpp += " : ";
  asCpp += "(";
  asCpp += right->emit_unrolled_c(l,amt);
  asCpp += ")";
  asCpp += ")";
  return asCpp;
}

std::string UnaryIntrin::emit_unrolled_c(ForStmt *l, int amt)
{ 
  std::string asCpp;
  assert(left);
  asCpp += "(";
  asCpp += intrinString;
  asCpp += "(";
  asCpp += left->emit_unrolled_c(l,amt);
  asCpp += ")";
  asCpp += ")";
  return asCpp;
}




std::string SelectExpr::emit_vectorized_c(ForStmt *l)
{
  std::string asCpp;

  asCpp += vecIntrinsics[op] + "(" ;


  if(sel) asCpp += sel->emit_vectorized_c(l);
  asCpp += " , /* true cond */ ";

  
  if(left) asCpp += left->emit_vectorized_c(l);
  asCpp += " , /* false cond */ ";
  
  if(right) asCpp += right->emit_vectorized_c(l);

  asCpp += ")";
  return asCpp;
}

std::string UnaryIntrin::emit_vectorized_c(ForStmt *l)
{
  std::string asCpp;
  if(left) asCpp += left->emit_vectorized_c(l);
  return asCpp;
}


std::string SelectExpr::emitTempVars(tempVar_t t, ForStmt *l,int vecLen)
{
  assert(sel); assert(left); assert(right);
  std::string asCpp;
 
  if(vecIntrinsics.size()==0)
    {
      for(int i = 0; i < NUM_VEC_OPS; i++)
	{
	  vecIntrinsics.push_back(vecOpNames[i] + int2string(vecLen));
	}
    }
  
  if(sel) asCpp += sel->emitTempVars(t,l,vecLen);
  if(left) asCpp += left->emitTempVars(t,l,vecLen);
  if(right) asCpp += right->emitTempVars(t,l,vecLen);
  return asCpp;
}

std::string UnaryIntrin::emitTempVars(tempVar_t t, ForStmt *l,int vecLen)
{
  assert(left);
  std::string asCpp;
 
  if(vecIntrinsics.size()==0)
    {
      for(int i = 0; i < NUM_VEC_OPS; i++)
	{
	  vecIntrinsics.push_back(vecOpNames[i] + int2string(vecLen));
	}
    }
  
  if(left) asCpp += left->emitTempVars(t,l,vecLen);
  return asCpp;
}


Value *SelectExpr::emitTempLLVMVars(tempVar_t t, llvm::IRBuilder<> *llvmBuilder,
				    std::map<ForStmt*, llvm::PHINode*> &iVarMap,
				    llvm::PHINode *iVar, ForStmt *l, int vecLen)

{
  if(sel) sel->emitTempLLVMVars(t,llvmBuilder, iVarMap, iVar, l, vecLen);
  if(left) left->emitTempLLVMVars(t,llvmBuilder, iVarMap, iVar, l, vecLen);
  if(right) right->emitTempLLVMVars(t,llvmBuilder, iVarMap, iVar, l, vecLen);
  return NULL;
}

Value *UnaryIntrin::emitTempLLVMVars(tempVar_t t, llvm::IRBuilder<> *llvmBuilder,
				    std::map<ForStmt*, llvm::PHINode*> &iVarMap,
				    llvm::PHINode *iVar, ForStmt *l, int vecLen)

{
  if(left) left->emitTempLLVMVars(t,llvmBuilder, iVarMap, iVar, l, vecLen);
  return NULL;
}

void SelectExpr::collectMemRefs(std::list<MemRef*> &mList)
{
  assert(sel); assert(left); assert(right);
  sel->collectMemRefs(mList);
  left->collectMemRefs(mList);
  right->collectMemRefs(mList);
}

void UnaryIntrin::collectMemRefs(std::list<MemRef*> &mList)
{
  assert(left);
  left->collectMemRefs(mList);
}

void SelectExpr::print(int depth)
{
  assert(left); assert(right); assert(sel);
  dbgPrintf("( (");
  sel->print(depth);
  dbgPrintf(")");
  dbgPrintf(" ? ");
  dbgPrintf("(");
  left->print(depth);
  dbgPrintf(")");
  dbgPrintf(" : ");
  dbgPrintf("(");
  right->print(depth);
  dbgPrintf(") )");
}

void UnaryIntrin::print(int depth)
{
  assert(left);
  dbgPrintf("(");
  dbgPrintf("%s(",intrinString.c_str());
  left->print(depth);
  dbgPrintf("))");
}

bool CompoundExpr::foundIntegerOps(std::list<MemRef*> &ml)
{
  int numFP=0,numInt=0;

  for(std::list<MemRef*>::iterator it = ml.begin();
      it != ml.end(); it++)
    {
      MemRef *r = *it;
      if(r->isInteger())
	numInt++;
      else
	numFP++;
    }


  if((numInt>0) && (numFP>0))
    dbgPrintf("warning, integer and FP operations in same assignment\n");

  return (numInt>0);
}


std::string CompoundExpr::emit_c()
{
  std::string asCpp;
  asCpp += "(";
  if(left) asCpp += left->emit_c();
  asCpp += ")";
  asCpp += scalarOpNames[op];
  asCpp += "(";
  if(right) asCpp += right->emit_c();
  asCpp += ")";
  return asCpp;
}

Value *CompoundExpr::emit_llvm(llvm::IRBuilder<> *llvmBuilder,
			       std::map<ForStmt*, llvm::PHINode*> &iVarMap)  
{
  Value *myValue;
  
  std::list<MemRef*> intCheckList;
  if(left) left->collectMemRefs(intCheckList);
  if(right) right->collectMemRefs(intCheckList);

  isIntegerOp = foundIntegerOps(intCheckList);

  assert(left); assert(right);
  leftValue = left->emit_llvm(llvmBuilder,iVarMap);
  rightValue = right->emit_llvm(llvmBuilder,iVarMap);

  const Type *leftType = leftValue->getType();
  const Type *rightType = rightValue->getType();
 
  if(leftType != rightType)
    {
      dbgPrintf("vector type mismatch!\n");
      exit(-1);
    }

  switch(this->op)
    {
    case(ADD):
      myValue = isIntegerOp ? llvmBuilder->CreateAdd(leftValue,rightValue) :
      llvmBuilder->CreateFAdd(leftValue,rightValue);
      break;
    case(SUB):
      myValue = isIntegerOp ? llvmBuilder->CreateSub(leftValue,rightValue) :
	llvmBuilder->CreateFSub(leftValue,rightValue);
      break;
    case(MUL):
      myValue = isIntegerOp ? llvmBuilder->CreateMul(leftValue,rightValue) :
	llvmBuilder->CreateFMul(leftValue,rightValue);
      break;
    case(DIV):
      if(isIntegerOp) {
	myValue = llvmBuilder->CreateSDiv(leftValue,rightValue);
	//dbgPrintf("%s:%d: integer divide not implemented\n",__FILE__,__LINE__);
	//exit(-1);
      }
      else {
	myValue = llvmBuilder->CreateFDiv(leftValue,rightValue);
      }
      break;
    case(LT):
      myValue = isIntegerOp ? 
	llvmBuilder->CreateICmpSLT(leftValue,rightValue) :
      llvmBuilder->CreateFCmpULT(leftValue,rightValue);
      break;
    case(LTE):
      dbgPrintf("%s:%d: not implemented\n",__FILE__,__LINE__);
      exit(-1);
      break;
    case(GT):
      dbgPrintf("%s:%d: not implemented\n",__FILE__,__LINE__);
      exit(-1);
      break;
    case(GTE):
      dbgPrintf("%s:%d: not implemented\n",__FILE__,__LINE__);
      exit(-1);
      break;
    case(EQ):
      dbgPrintf("%s:%d: not implemented\n",__FILE__,__LINE__);
      exit(-1);
      break;
    case(NEQ):
      dbgPrintf("%s:%d: not implemented\n",__FILE__,__LINE__);
      exit(-1);
      break;
    }

  llvmValues.push_back(myValue);
  return myValue;
}


Value *CompoundExpr::emit_vectorized_llvm(llvm::IRBuilder<> *llvmBuilder,
					  std::map<ForStmt*, llvm::PHINode*> &iVarMap,
					  PHINode *iVar,
					  bool doAligned,
					  int vecLen)
  
{
  Value *myValue;

  std::list<MemRef*> intCheckList;
  assert(left); assert(right);

  if(left) left->collectMemRefs(intCheckList);
  if(right) right->collectMemRefs(intCheckList);
  
  isIntegerOp = foundIntegerOps(intCheckList);
 
  leftValue = left->emit_vectorized_llvm(llvmBuilder,iVarMap,iVar, doAligned, vecLen);
  rightValue = right->emit_vectorized_llvm(llvmBuilder,iVarMap,iVar, doAligned, vecLen);

  assert(leftValue);
  assert(rightValue);

  //cout << leftValue->getStr() << "\n";
  //cout << rightValue->getStr() << "\n";
  //VectorType *T = VectorType::get(sleftValue->getType(, vecLen);
  //Value *leftValue = llvmBuilder->CreateBitCast(sleftValue, T,
  //						"vector_left_op");
  //Value *rightValue = llvmBuilder->CreateBitCast(srightValue, T,
  //						"vector_right_op");
  
  switch(this->op)
    {
    case(ADD):
      myValue = isIntegerOp ? llvmBuilder->CreateAdd(leftValue,rightValue) :
      llvmBuilder->CreateFAdd(leftValue,rightValue);
      break;
    case(SUB):
      myValue = isIntegerOp ? llvmBuilder->CreateSub(leftValue,rightValue) :
      llvmBuilder->CreateFSub(leftValue,rightValue);
      break;
    case(MUL):
      myValue = isIntegerOp ? llvmBuilder->CreateMul(leftValue,rightValue) :
      llvmBuilder->CreateFMul(leftValue,rightValue);
      break;
    case(DIV):
      if(isIntegerOp) {
	dbgPrintf("%s:%d: integer divide not implemented\n",__FILE__,__LINE__);
	exit(-1);
      } else {
	myValue = llvmBuilder->CreateFDiv(leftValue,rightValue);
      }

      break;
    case(LT):
      dbgPrintf("%s:%d: not implemented\n",__FILE__,__LINE__);
      exit(-1);
      break;
    case(LTE):
      dbgPrintf("%s:%d: not implemented\n",__FILE__,__LINE__);
      exit(-1);
      break;
    case(GT):
      dbgPrintf("%s:%d: not implemented\n",__FILE__,__LINE__);
      exit(-1);
      break;
    case(GTE):
      dbgPrintf("%s:%d: not implemented\n",__FILE__,__LINE__);
      exit(-1);
      break;
    case(EQ):
      dbgPrintf("%s:%d: not implemented\n",__FILE__,__LINE__);
      exit(-1);
      break;
    case(NEQ):
      dbgPrintf("%s:%d: not implemented\n",__FILE__,__LINE__);
      exit(-1);
      break;
    }

  llvmValues.push_back(myValue);
  return myValue;
}

Value *CompoundExpr::emit_unrolled_llvm(llvm::IRBuilder<> *llvmBuilder,
					std::map<ForStmt*, llvm::PHINode*> &iVarMap,
					PHINode *iVar,
					int amt)  
{
  Value *myValue;

  std::list<MemRef*> intCheckList;
  assert(left); assert(right);

  if(left) left->collectMemRefs(intCheckList);
  if(right) right->collectMemRefs(intCheckList);
  
  isIntegerOp = foundIntegerOps(intCheckList);
 
  leftValue = left->emit_unrolled_llvm(llvmBuilder,iVarMap,iVar,amt);
  rightValue = right->emit_unrolled_llvm(llvmBuilder,iVarMap,iVar,amt);

  switch(this->op)
    {
    case(ADD):
      myValue = isIntegerOp ? llvmBuilder->CreateAdd(leftValue,rightValue) :
      llvmBuilder->CreateFAdd(leftValue,rightValue);
      break;
    case(SUB):
      myValue = isIntegerOp ? llvmBuilder->CreateSub(leftValue,rightValue) :
      llvmBuilder->CreateFSub(leftValue,rightValue);
      break;
    case(MUL):
      myValue = isIntegerOp ? llvmBuilder->CreateMul(leftValue,rightValue) :
      llvmBuilder->CreateFMul(leftValue,rightValue);
      break;
    case(DIV):
      if(isIntegerOp) {
	dbgPrintf("%s:%d: integer divide not implemented\n",__FILE__,__LINE__);
	exit(-1);
      } else {
	myValue = llvmBuilder->CreateFDiv(leftValue,rightValue);
      }

      break;
    case(LT):
      myValue = isIntegerOp ? 
	llvmBuilder->CreateICmpULT(leftValue,rightValue) :
	llvmBuilder->CreateFCmpULT(leftValue,rightValue);
      break;
    case(LTE):
      dbgPrintf("%s:%d: not implemented\n",__FILE__,__LINE__);
      exit(-1);
      break;
    case(GT):
      dbgPrintf("%s:%d: not implemented\n",__FILE__,__LINE__);
      exit(-1);
      break;
    case(GTE):
      dbgPrintf("%s:%d: not implemented\n",__FILE__,__LINE__);
      exit(-1);
      break;
    case(EQ):
      dbgPrintf("%s:%d: not implemented\n",__FILE__,__LINE__);
      exit(-1);
      break;
    case(NEQ):
      dbgPrintf("%s:%d: not implemented\n",__FILE__,__LINE__);
      exit(-1);
      break;
    }

  llvmValues.push_back(myValue);
  return myValue;
}

std::string CompoundExpr::emitTempVars(tempVar_t t, ForStmt *l,int vecLen)
{
  std::string asCpp;

  //dbgPrintf("CALLLED %s\n", __PRETTY_FUNCTION__);

  if(vecIntrinsics.size() == 0)
    {
      for(int i = 0; i < NUM_VEC_OPS; i++)
	{
	  vecIntrinsics.push_back(vecOpNames[i] + int2string(vecLen));
	}
    }

 
  if(left) asCpp += left->emitTempVars(t,l,vecLen);
  if(right) asCpp += right->emitTempVars(t,l,vecLen);
  return asCpp;
}


Value* CompoundExpr::emitTempLLVMVars(tempVar_t t, llvm::IRBuilder<> *llvmBuilder,
				std::map<ForStmt*, llvm::PHINode*> &iVarMap,
				llvm::PHINode *iVar, ForStmt *l, int vecLen)
{
  if(left) left->emitTempLLVMVars(t,llvmBuilder, iVarMap, iVar, l, vecLen);
  if(right) right->emitTempLLVMVars(t,llvmBuilder, iVarMap, iVar, l, vecLen);
  return NULL;
}


void CompoundExpr::collectMemRefs(std::list<MemRef*> &mList)
{
  if(left) left->collectMemRefs(mList);
  if(right) right->collectMemRefs(mList);
}


void AssignExpr::printVectors()
{
  dbgPrintf("true vectors:\n");
  for(std::set<directionVector>::iterator it=trueVectors.begin();
      it != trueVectors.end(); it++)
    { dbgPrintf("\t"); directionVector v = *it; v.print(); dbgPrintf("\n"); }

  dbgPrintf("anti vectors:\n");
  for(std::set<directionVector>::iterator it=antiVectors.begin();
      it != antiVectors.end(); it++)
    { dbgPrintf("\t"); directionVector v = *it; v.print(); dbgPrintf("\n"); }
  
  dbgPrintf("output vectors:\n");
  for(std::set<directionVector>::iterator it=outputVectors.begin();
      it != outputVectors.end(); it++)
    { dbgPrintf("\t"); directionVector v = *it; v.print(); dbgPrintf("\n"); }
}


Value *AssignExpr::emitTempLLVMVars(tempVar_t t, llvm::IRBuilder<> *llvmBuilder,
				    std::map<ForStmt*, llvm::PHINode*> &iVarMap,
				    llvm::PHINode *iVar, ForStmt *l, int vecLen)

{
  //assert(memoryRefs.size() != 0);
  MemRef *leftRef = dynamic_cast<MemRef*>(left);

  if(leftRef)
    {
      leftRef->isStore=true;   
      for(std::list<MemRef*>::iterator it0=memoryRefs.begin();
	  it0 != memoryRefs.end(); it0++)
	{
	  MemRef *r0 = *it0;
	  /* same base variable exists in statement,
	   * we have to make sure they get assigned
	   * to the same "register" 
	   *
	   * this is simplified by the fact that
	   * we only accept vectorize/unroll loops with 
	   * "="
	   *
	   * by definition, we are assigning into 
	   * the same variable range 
	   */
	  if(r0->variable == leftRef->variable)
	    {
	      r0->appendOtherRef(leftRef);
	      leftRef->appendOtherRef(r0);
	    }
	}
    }


  if(left) left->emitTempLLVMVars(t,llvmBuilder, iVarMap, iVar, l, vecLen);
  if(right) right->emitTempLLVMVars(t,llvmBuilder, iVarMap, iVar, l, vecLen);
  return NULL;
}



std::string AssignExpr::emitTempVars(tempVar_t t, ForStmt *l, int vecLen)
{
  std::string asCpp;

  memoryRefs.clear();

  if(right) right->collectMemRefs(memoryRefs);

  MemRef *leftRef = dynamic_cast<MemRef*>(left);
  //dbgPrintf("%d memory references\n", (int)memoryRefs.size());
  
  if(leftRef)
    {
      /* marked as a store to prevent register reuse in
       * loop */
      leftRef->isStore=true;
      for(std::list<MemRef*>::iterator it0=memoryRefs.begin();
	  it0 != memoryRefs.end(); it0++)
	{
	  MemRef *r0 = *it0;
	  /* same base variable exists in statement,
	   * we have to make sure they get assigned
	   * to the same "register" 
	   *
	   * this is simplified by the fact that
	   * we only accept vectorize/unroll loops with 
	   * "="
	   *
	   * by definition, we are assigning into 
	   * the same variable range 
	   */
	  if(r0->variable == leftRef->variable)
	    {
	      r0->appendOtherRef(leftRef);
	      leftRef->appendOtherRef(r0);
	    }
	}

      /* give both variables the same temporary
       * name */
      //int rCount = leftRef->variable->getRefCount();
      std::string tVar = leftRef->variable->genTempVar();

      leftRef->tempVarAssigned=true;
      leftRef->tempName=tVar;

      for(std::list<MemRef*>::iterator it=leftRef->otherMemRefs.begin();
	  it != leftRef->otherMemRefs.end(); it++)
	{
	  MemRef *r0 = *it;
	  r0->tempVarAssigned=true;
	  r0->tempName=tVar;
	}
    }



  if(left) asCpp += left->emitTempVars(t,l, vecLen);
  if(right) asCpp += right->emitTempVars(t,l, vecLen);
  return asCpp;
}


void AssignExpr::printLoops()
{
  for(int i=0;i<(MAX_DEPTH+1);i++)
    {
      if(inLoops[i]!=NULL)
	{
	  dbgPrintf("%s(%d) ", inLoops[i]->inductionVar.c_str(),i);
	}
    }
  dbgPrintf("\n");
}

int AssignExpr::deepestLevel()
  {
    for(int i=1;i<(MAX_DEPTH);i++)
      {
	if(inLoops[i]==NULL)
	  return i-1;
      }
    assert(true==false);
    return -1;
  }

bool AssignExpr::SIMDizable(ForStmt *l)
{
  bool leftSIMD = false;
  bool rightSIMD = false;
  int rc;

  if(left)
    {
      rc = left->computeStride(l);
      if(rc==0 || rc==1)
	leftSIMD=true;
    }
  if(right)
    {
      rc = right->computeStride(l);
      if(rc==0 || rc==1)
	rightSIMD=true;
    }
  return (leftSIMD && rightSIMD);
}

std::string AssignExpr::emit_unrolled_c(ForStmt *l, int amt)
{
  std::string asCpp;
  for(int i = 0; i < amt; i++)
    {
      if(left) asCpp += left->emit_unrolled_c(l,i);
      asCpp += "=";
      if(right) asCpp += right->emit_unrolled_c(l,i);
      asCpp += ";\n";
    }
  return asCpp;
}


Value *AssignExpr::emit_unrolled_llvm(llvm::IRBuilder<> *llvmBuilder,
				      std::map<ForStmt*, llvm::PHINode*> &iVarMap,
				      PHINode *iVar,
				      int amt)
{
  Value *myStore = NULL;

  MemRef *ll = dynamic_cast<MemRef*>(left);
  if(ll)
    {
      ll->isStore=true;
    }
  
  for(int i =0; i < amt; i++)
    {
      leftValue = left->emit_unrolled_llvm(llvmBuilder,iVarMap, iVar, i);
      rightValue = right->emit_unrolled_llvm(llvmBuilder,iVarMap, iVar, i);
      myStore = llvmBuilder->CreateStore(rightValue, leftValue);
      llvmValues.push_back(myStore);
    }
  return myStore;
}

Value *AssignExpr::emit_vectorized_llvm(llvm::IRBuilder<> *llvmBuilder,
					std::map<ForStmt*, llvm::PHINode*> &iVarMap,
					PHINode *iVar,
					bool doAligned,
					int vecLen)
{
  Value *myStore = NULL;

  MemRef *ll = dynamic_cast<MemRef*>(left);
  if(ll)
    {
      ll->isStore=true;
    }
 
  /* left value will be gep memory instruction */
  leftValue = left->emit_vectorized_llvm(llvmBuilder,iVarMap, iVar,doAligned,vecLen);
  rightValue = right->emit_vectorized_llvm(llvmBuilder,iVarMap, iVar,doAligned,vecLen);
 
  PointerType *pointerType = dyn_cast<PointerType>(leftValue->getType());
  assert(pointerType);
  Type *scalarType = pointerType->getElementType();
  assert(scalarType);
  
  VectorType *vT = VectorType::get(scalarType, vecLen);
  Type *T = PointerType::getUnqual(vT);

  Value *VectorPtr = llvmBuilder->CreateBitCast(leftValue, T,
						    "vector_st_ptr");
  
  
  llvm::StoreInst *st = llvmBuilder->CreateStore(rightValue, VectorPtr);
  if(!doAligned)
    st->setAlignment(1);

  myStore = st;
  llvmValues.push_back(myStore);
  
  return myStore;
}

std::string AssignExpr::emit_vectorized_c(ForStmt *l)
{
  std::string asCpp;
  if(left) asCpp += left->emit_vectorized_c(l);
  if(right) asCpp += right->emit_vectorized_c(l);
  asCpp += ");\n";
  return asCpp;
}


Value *AssignExpr::emit_llvm(llvm::IRBuilder<> *llvmBuilder,
			     std::map<ForStmt*, llvm::PHINode*> &iVarMap)
{
  Value *myStore = NULL;
  std::list<MemRef*> intCheckList;
  assert(left); assert(right);

  if(left) left->collectMemRefs(intCheckList);
  if(right) right->collectMemRefs(intCheckList);

  isIntegerOp = foundIntegerOps(intCheckList);

  MemRef *ll = dynamic_cast<MemRef*>(left);
  if(ll) 
    { 
      ll->isStore=true; 
    }

  leftValue = left->emit_llvm(llvmBuilder,iVarMap);
  rightValue = right->emit_llvm(llvmBuilder,iVarMap);

  const Type *leftType = leftValue->getType();
  const Type *rightType = rightValue->getType();

  if(leftType->isVectorTy() != rightType->isVectorTy())
    {
      dbgPrintf("vector type mismatch!\n");
      exit(-1);
    }


  myStore = llvmBuilder->CreateStore(rightValue, leftValue);
  llvmValues.push_back(myStore);
  return myStore;
}


std::string AssignExpr::emit_c()
  {
    //dbgPrintf("iteration cost=%g\n", iterationCost());
    std::string asCpp;
    if(left) asCpp += left->emit_c();
    asCpp += "=";
    if(right) asCpp += right->emit_c();
    asCpp += ";\n";
    return asCpp;
  }

double AssignExpr::iterationCost()
{
  assert(left); assert(right);
  double lcost = left->cost(inLoops);
  double rcost = right->cost(inLoops);
  return (lcost+rcost);
}


void AssignExpr::shiftLoopLevels(int k, int p)
{
  std::list<directionVector> tempTrue;
  std::list<directionVector> tempAnti;
  std::list<directionVector> tempOutput;

  size_t tSize=trueVectors.size();
  size_t aSize=antiVectors.size();
  size_t oSize=outputVectors.size();

  ForStmt *tmpFor[MAX_DEPTH+1];
  for(int i=0;i<(MAX_DEPTH+1);i++)
    tmpFor[i] = inLoops[i];

  for(int i=k;i<p;i++)
    tmpFor[i+1]=inLoops[i];
  
  tmpFor[k]=inLoops[p];

  for(int i=0;i<MAX_DEPTH;i++)
    inLoops[i]=tmpFor[i];


  for(std::set<directionVector>::iterator it=trueVectors.begin();
      it != trueVectors.end(); it++)
    {
      tempTrue.push_back(*it);
    }

  for(std::set<directionVector>::iterator it=antiVectors.begin();
      it != antiVectors.end(); it++)
    {
      tempAnti.push_back(*it);
      //it->shiftLoopLevels(k,p);
    }
  for(std::set<directionVector>::iterator it=outputVectors.begin();
      it != outputVectors.end(); it++)
    {
      tempOutput.push_back(*it);
      //it->shiftLoopLevels(k,p);
    }
  
  trueVectors.clear();
  antiVectors.clear();
  outputVectors.clear();

  for(std::list<directionVector>::iterator it=tempTrue.begin();
      it != tempTrue.end(); it++)
    {
      directionVector v = *it;
      //v.print(); dbgPrintf("\n");
      v.shiftLoopLevels(k,p);
      //v.print(); dbgPrintf("\n");
      trueVectors.insert(v);
    }
  for(std::list<directionVector>::iterator it=tempAnti.begin();
      it != tempAnti.end(); it++)
    {
      directionVector v = *it;
      v.shiftLoopLevels(k,p);
      antiVectors.insert(v);
    }
  for(std::list<directionVector>::iterator it=tempOutput.begin();
      it != tempOutput.end(); it++)
    {
      directionVector v = *it;
      v.shiftLoopLevels(k,p);
      outputVectors.insert(v);
    }

  assert(tSize==trueVectors.size());
  assert(aSize==antiVectors.size());
  assert(oSize==outputVectors.size());


  bool foundNull = false;
  for(int i = 1; i < MAX_DEPTH; i++)
    {
      if(inLoops[i]!=NULL && foundNull)
	{
	  dbgPrintf("%s:%d -> something has gone horribly wrong with loop shifting\n",
		 __FILE__, __LINE__);
	  exit(-1);
	}
      if(inLoops[i]==NULL)
	{
	  foundNull=true;
	}
    }


}

void AssignExpr::checkDepth(int level)
  {
    int maxd=1;
    for(int i=1;i<(MAX_DEPTH+1);i++)
      {
	if(inLoops[i]==NULL) break;
	maxd=i;
      }
    if(level < maxd)
      {
	dbgPrintf("%d parlevels: paroutside deepest nesting\n",maxd-level);
      }

    dbgPrintf("level = %d, maxd=%d\n", level, maxd);
  }

AssignExpr *Ref::getAssignment()
{
  assert(parent);
  AssignExpr *p = 0;
  for(Node *n=parent; n!=0; n=n->parent)
    {
      p = dynamic_cast<AssignExpr*>(n);
      if(p) return p;
    }
  assert(false);
  return p;
}

double MemRef::cost(ForStmt **loops)
{
  ForStmt *deepestLoop = NULL;
  for(int i = 1; i < MAX_DEPTH+1; i++)
    {
      if(loops[i]==NULL)
	break;
      deepestLoop = loops[i];
    }
  assert(deepestLoop);
  
  int numSubscripts = (int)subscripts.size();
  assert(numSubscripts>0);

  astSubscript *innerSubscript = subscripts[numSubscripts-1];
  assert(innerSubscript);

  /* check for stride-1 access (if present at all) */
  int s = innerSubscript->checkIfExclusiveIvar(deepestLoop);
  
  if(s==1)
    {
      dbgPrintf("stride-1 access to %s\n", getName().c_str());
      return 1.0;
    }
  else
    return 0.0;
}

Value* MemRef::emit_llvm(llvm::IRBuilder<> *llvmBuilder,
			 std::map<ForStmt*, llvm::PHINode*> &iVarMap)
{
  Value *ptr=NULL,*idx=NULL,*gep=NULL;
  
  //find pointer
  ptr = variable->llvmBase;
  idx = ConstantInt::get(Type::getInt32Ty(getGlobalContext()),0);
  //generate idx
  int sss = subscripts.size();
  int c = 0;
  for(std::vector<astSubscript*>::iterator it = subscripts.begin();
      it != subscripts.end(); it++)
      {
	Value *v = 0;

	Value *iOffset;
	
	if( (*it)->isIndirect)
	  {
	    v = (*it)->indirectRef->emit_llvm(llvmBuilder, iVarMap);
	  }
	else
	  {
	    v = (*it)->getLLVM(llvmBuilder, iVarMap);
	   
	  }

	if(c == (sss-1))
	  {
	    idx = llvmBuilder->CreateAdd(idx, v);
	  }
	else
	  {
	    iOffset = variable->llvmIndices[sss-c-1];
	
	    Value *iMulAddr =
	      llvmBuilder->CreateMul(v, iOffset);

	    idx = llvmBuilder->CreateAdd(idx, iMulAddr);
	  }
	c++;

      }
  if(llvmBuilder == 0)
    {
      dbgPrintf("llvmBuilder = %p\n", llvmBuilder);
      exit(-1);
    }

  gep = llvmBuilder->CreateGEP(ptr, idx);
  if(isStore)
    {
      //llvmValues.push_back(gep);
      return gep;
    }
  else
    {
      std::string loadName= "";
      if(tempLLVMAssigned)
	{
	  loadName += "unrolledLoad";
	}
      Value *l = llvmBuilder->CreateLoad(gep,loadName); 
      //llvmValues.push_back(l);
      return l;
    }
}


std::string MemRef::emit_c()
{
  std::string s = variable->varName;
  s+=("[");   
 
 
 
  int c=0;
  int sss = subscripts.size();
  for(std::vector<astSubscript*>::iterator it = subscripts.begin();
      it != subscripts.end(); it++)
    {
      std::string t;
      if( (*it)->isIndirect)
	{
	  t =(*it)->indirectRef->emit_c();
	}
      else
	{
	  t =(*it)->getLinearString();
	}
      s += t + "*" + ((c!=(sss-1)) ? 
		      variable->dimNames[sss-c-1] : "1"); 

      if(c!=(subscripts.size()-1))
	{
	  s+= " + ";
	}
      c++;
    }	
  
  s+=("]");
  return s;
}


void MemRef::collectMemRefs(std::list<MemRef*> &mList)
{
  mList.push_back(this);
}

int MemRef::computeStride(ForStmt *l)
{
  int sss = subscripts.size();
  assert(sss > 0);
  int rc=0;
  int c=0;
  int subSize = subscripts.size();
  bool *pos = new bool[subSize];
  for(int i=0;i<subSize;i++)
    pos[i]=false;

  int idx=0;
  /* first, check if present in any subscript */
  for(std::vector<astSubscript*>::iterator it = subscripts.begin();
      it != subscripts.end(); it++)
    {
      astSubscript *as = *it;
      if(as->ivarPresent(l)) {
	c++;
	pos[idx] = true;
	}
      idx++;
    }

  /* induction variable doesn't appear in 
   * memory reference, we're golden */
  if(c==0) {
    rc=0;
  }
  /* induction variable appears once,
   * hmmm this is probably guaranteed 
   * by earlier properties..oh well */
  else if(c==1)
    {
      /* outer most subscript,
       * aka potential for stride-1 */
      if(pos[sss-1])
	{
	  /* make sure coeffient is valid
	   * aka 1 */
	  rc = subscripts[sss-1]->computeStride(l);
	}
      /* inner subscripts, stride-1 not
       * possible */
      else
	{
	  rc = 2;
	}
    }
  /* induction variable appears multiple 
   * times, all bets are off */
  else
    {
      dbgPrintf("c=%d\n", c);
      rc = -1;
    }

  std::string s = variable->varName;
  // dbgPrintf("%s is stride-%d in loop\n", s.c_str(), rc);
  
  delete [] pos;
  return rc;
}

Value *ConstantIntVar::emit_llvm(llvm::IRBuilder<> *llvmBuilder,
			   std::map<ForStmt*, llvm::PHINode*> &iVarMap)
{
  Value *myValue = ConstantInt::get(Type::getInt32Ty(getGlobalContext()),value);

  llvmValues.push_back(myValue);
  return myValue;
}

Value *ConstantIntVar::emit_vectorized_llvm(llvm::IRBuilder<> *llvmBuilder,
					    std::map<ForStmt*, llvm::PHINode*> &iVarMap,
					    llvm::PHINode *iVar,
					    bool doAligned,
					    int vecLen)
{
  dbgPrintf("unimplemented at %s:%d\n", __FILE__,__LINE__);
  exit(-1);
  return ConstantInt::get(Type::getInt32Ty(getGlobalContext()),0);
}

Value* ConstantIntVar::emitTempLLVMVars(tempVar_t t, llvm::IRBuilder<> *llvmBuilder,
					std::map<ForStmt*, llvm::PHINode*> &iVarMap,
					llvm::PHINode *iVar, ForStmt *l, int vecLen)
{
  if(t==VECTOR)
    {
      dbgPrintf("unimplemented at %s:%d\n", __FILE__,__LINE__);
    }
  return ConstantInt::get(Type::getInt32Ty(getGlobalContext()),0);
}


Value *ConstantFloatVar::emit_llvm(llvm::IRBuilder<> *llvmBuilder,
			   std::map<ForStmt*, llvm::PHINode*> &iVarMap)
{
  Value *myValue = ConstantFP::get(getGlobalContext(), APFloat(value));
  llvmValues.push_back(myValue);
  return myValue;
}

Value *ConstantFloatVar::emit_vectorized_llvm(llvm::IRBuilder<> *llvmBuilder,
					      std::map<ForStmt*, llvm::PHINode*> &iVarMap,
					      llvm::PHINode *iVar,
					      bool doAligned,
					      int vecLen)
{
  Value *myValue = ConstantFP::get(getGlobalContext(), APFloat(value));
  VectorType *T = VectorType::get(myValue->getType(), vecLen);
  Value *myVector = UndefValue::get(T);
  for (int i = 0; i < vecLen; i++)
    {
      Value * idx = (Value*)ConstantInt::get(Type::getInt32Ty(getGlobalContext()),i);
      myVector = llvmBuilder->CreateInsertElement(myVector,
						  myValue,
						  idx,
						  "__cVec_");
    }
  return myVector;
}

std::string ConstantFloatVar::emitTempVars(tempVar_t t, ForStmt *l,int vecLen)
{
  std::string asCpp;
  if(t==VECTOR)
    {
      tempVarAssigned=true;
      tempName = "fltConstant" + int2string(getConstantId());
      asCpp += "float" + int2string(vecLen) + " " + tempName + 
	+ "=" + "vecSplatter" + int2string(vecLen) + 
	"(" + emit_c() +")" + ";\n"; 
    }
 return asCpp;
}

Value* ConstantFloatVar::emitTempLLVMVars(tempVar_t t, llvm::IRBuilder<> *llvmBuilder,
					std::map<ForStmt*, llvm::PHINode*> &iVarMap,
					llvm::PHINode *iVar, ForStmt *l, int vecLen)
{
  Value *myValue = ConstantFP::get(getGlobalContext(), APFloat(value));
  if(t==VECTOR)
    {
      VectorType *T = VectorType::get(myValue->getType(), vecLen);
      Value *myVector = UndefValue::get(T); 
      for (int i = 0; i < vecLen; i++)
	{
	  Value * idx = (Value*)ConstantInt::get(Type::getInt32Ty(getGlobalContext()),i);
	  myVector = llvmBuilder->CreateInsertElement(myVector,
						      myValue,
						      idx,
						      "__kVec_");
	}
      llvmValues.push_back(myVector);
      return myVector;
    } 
  return myValue;
}

std::string ConstantFloatVar::emit_vectorized_c(ForStmt *l)
{
  assert(tempVarAssigned);
  return tempName;
}


Value* MemRef::emitTempLLVMVars(tempVar_t t, llvm::IRBuilder<> *llvmBuilder,
				std::map<ForStmt*, llvm::PHINode*> &iVarMap,
				llvm::PHINode *iVar, ForStmt *l, int vecLen)
{
  Value *myLoad = NULL;
  Value *myVector;
  VectorType *T;

  switch(t)
    {
    case UNROLL:
    case VECTOR:
      if(computeStride(l)==0 && (!isStore))
	{
	  tempLLVMAssigned=true;
	  isConstant=true;
	  unrollLoopVar=l;
	  if(t==VECTOR)
	    {
	      if(llvmValues.size()!=0)
		{
		  dbgPrintf("vectorized memref value = %d\n", 
			 (int)llvmValues.size());
		  exit(-1);
		}
	      assert(llvmValues.size()==0);
	      /* alright, load scalar value*/
	      myLoad = emit_llvm(llvmBuilder, iVarMap);
	      assert(myLoad);
	      
	      VectorType *vT = VectorType::get(myLoad->getType(), vecLen);
	      Value *myVector = UndefValue::get(vT);
	       
	      /* generate a vector operand */
	      for (int i = 0; i < vecLen; i++)
		{
		  Value * idx = (Value*)ConstantInt::get(Type::getInt32Ty(getGlobalContext()),i);
		  myVector = llvmBuilder->CreateInsertElement(myVector,
							      myLoad,
							      idx,
							      "__vec_");
		}

	      llvmValues.push_back(myVector);
	      return myVector;
	    }
	  else
	    {
	      assert(llvmValues.size()==0);
	    
	      myLoad = emit_llvm(llvmBuilder, iVarMap);
	      llvmValues.push_back(myLoad);
	    }
	}
      
    }
  return myLoad;
}

std::string MemRef::emitTempVars(tempVar_t t, ForStmt *l, int vecLen)
{
  std::string asCpp;
  switch(t)
    {
    case UNROLL:
    case VECTOR:
      vecLoadOp = "vecLoad" + int2string(vecLen);
      vecStoreOp = "vecStore" + int2string(vecLen);
      
      if(computeStride(l)==0 && (!isStore))
	{
	  if(!tempVarAssigned)
	    {
	      tempVarAssigned=true;
	      isConstant=true;
	      unrollLoopVar=l;
	      tempName = variable->genTempVar();

	      if(t==VECTOR)
		{
		  asCpp += variable->typeStr + int2string(vecLen) + " " + tempName + 
		    + "=" + "vecLoadSplatter" + int2string(vecLen) + 
		    "(&" + emit_c() +")" + ";\n"; 
		}
	      else
		{
		  asCpp += variable->typeStr + " " + tempName + 
		    + "=" + emit_c() + ";\n"; 
		}
	    }
	  else
	    {
	      if(t==VECTOR)
		{
		  asCpp += variable->typeStr + int2string(vecLen) + " " + tempName + 
		    + "=" + "vecLoadSplatter" + int2string(vecLen) + 
		    "(&" + emit_c() +")" + ";\n"; 
		}
	      else
		{
		  asCpp += variable->typeStr + " " + tempName + 
		    + "=" + emit_c() + ";\n"; 
		}
	    }
	}
      break;
    case DEFAULT:
      break;
    default :
      break;
    };
  
  return asCpp;
}







std::string MemRef::emit_unrolled_c(ForStmt *l, int amt)
{
  /* alright this works for unrolled variables
   * that are constant */
  if(isConstant && tempVarAssigned && (!isStore)) {
    assert(unrollLoopVar == l);
    return tempName;
  }

  std::string s = variable->varName;
  s+=("[");   
  int c=0;
  int sss = subscripts.size();
  for(std::vector<astSubscript*>::iterator it = subscripts.begin();
      it != subscripts.end(); it++)
    {
      std::string t;
      if( (*it)->isIndirect)
	{
	  t =(*it)->indirectRef->emit_c();
	}
      else
	{
	  t =(*it)->getLinearString(l, amt);
	}

      s += t + "*" + ((c!=(sss-1)) ? 
			variable->dimNames[sss-c-1] : "1"); 
      if(c!=(subscripts.size()-1))
	{
	  s+= " + ";
	}
      c++;
    }	
  s+=("]");
    //dbgPrintf("s=%s\n",s.c_str());
#ifdef NDARRAY
    return getName();
#else
    return s;
#endif
}

Value* MemRef::emit_vectorized_llvm(llvm::IRBuilder<> *llvmBuilder,
				    std::map<ForStmt*, llvm::PHINode*> &iVarMap,
				    llvm::PHINode *iVar,
				    bool doAligned,
				    int vecLen)
{
  //dbgPrintf("emit vectorized llvm load/store called\n");

  Value *ptr=NULL,*idx=NULL,*gep=NULL;
  

  if(isConstant && tempLLVMAssigned && (!isStore)) {
    if(llvmValues.size()!=1)
      {
	dbgPrintf("vectorized memref value = %d\n", 
	       (int)llvmValues.size());
	exit(-1);
      }
    return llvmValues[0];
  }

  //find pointer
  ptr = variable->llvmBase;
  
  idx = ConstantInt::get(Type::getInt32Ty(getGlobalContext()),0);
  //generate idx
  int sss = subscripts.size();
  int c = 0;
  for(std::vector<astSubscript*>::iterator it = subscripts.begin();
      it != subscripts.end(); it++)
      {

	Value *v = 0;
	Value *iOffset;
	if( (*it)->isIndirect)
	  {
	    v = (*it)->indirectRef->emit_llvm(llvmBuilder, iVarMap);
	  }
	else
	  {
	    v = (*it)->getLLVM(llvmBuilder, iVarMap);
	  }
	
	if(c == (sss-1))
	  {
	    idx = llvmBuilder->CreateAdd(idx, v);
	  }
	else
	  {
	    iOffset = variable->llvmIndices[sss-c-1];
	
	    Value *iMulAddr =
	      llvmBuilder->CreateMul(v, iOffset);

	    idx = llvmBuilder->CreateAdd(idx, iMulAddr);
	  }
	c++;

      }
  
  gep = llvmBuilder->CreateGEP(ptr, idx);
  if(isStore)
    {
      return gep;
    }
  else
    {
      PointerType *pointerType = dyn_cast<PointerType>(gep->getType());
      assert(pointerType);
      Type *scalarType = pointerType->getElementType();
      assert(scalarType);

      /*
      NewOperand = Builder.CreateTruncOrBitCast(NewOperand,
                                                   oldOperand->getType());
      */

      VectorType *vT = VectorType::get(scalarType, vecLen);
      Type *T = PointerType::getUnqual(vT);

      Value *VectorPtr = llvmBuilder->CreatePointerCast(gep, T,
					      "vector_ld_ptr");
  
      LoadInst *ld = llvmBuilder->CreateLoad(VectorPtr,false,"");
      if(!doAligned)
	ld->setAlignment(1);
      return ld; 
    }

}

Value* MemRef::emit_unrolled_llvm(llvm::IRBuilder<> *llvmBuilder,
			 std::map<ForStmt*, llvm::PHINode*> &iVarMap,
			 llvm::PHINode *iVar,
			 int amt)
{
  Value *ptr=NULL,*idx=NULL,*gep=NULL;
  
 

  //find pointer
  ptr = variable->llvmBase;

  idx = ConstantInt::get(Type::getInt32Ty(getGlobalContext()),0);
  //generate idx
  int sss = subscripts.size();
  int c = 0;
  for(std::vector<astSubscript*>::iterator it = subscripts.begin();
      it != subscripts.end(); it++)
      {
	Value *v = 0;
	Value *iOffset;
	if( (*it)->isIndirect)
	  {
	    v = (*it)->indirectRef->emit_llvm(llvmBuilder, iVarMap);
	  }
	else
	  {
	    //v = (*it)->getLLVM(llvmBuilder, iVarMap);
	    v = (*it)->getLLVM(llvmBuilder, iVarMap, iVar, amt);
	  }
	
	if(c == (sss-1))
	  {
	    idx = llvmBuilder->CreateAdd(idx, v);
	  }
	else
	  {
	    iOffset = variable->llvmIndices[sss-c-1];
	
	    Value *iMulAddr =
	      llvmBuilder->CreateMul(v, iOffset);

	    idx = llvmBuilder->CreateAdd(idx, iMulAddr);
	  }
	c++;

      }

  gep = llvmBuilder->CreateGEP(ptr, idx);
  if(isStore)
    {
      return gep;
    }
  else
    {
      return llvmBuilder->CreateLoad(gep);
    }
}


std::string MemRef::emit_vectorized_c(ForStmt *l)
{
  /* alright this works for unrolled variables
   * that are constant */
  if(isConstant && tempVarAssigned && (!isStore)) {
    assert(unrollLoopVar == l);
    return tempName;
  }

  std::string s;

  if(isStore)
    {
      s += vecStoreOp + "(&";
    }
  else
    {
      s += vecLoadOp + "(&";
    }

  s+= variable->varName;
  s+=("[");   
  int c=0;
  int sss = subscripts.size();
  for(std::vector<astSubscript*>::iterator it = subscripts.begin();
      it != subscripts.end(); it++)
    {
      std::string t;
      if( (*it)->isIndirect)
	{
	  t =(*it)->indirectRef->emit_c();
	}
      else
	{
	  t =(*it)->getLinearString();
	}

      s += t + "*" + ((c!=(sss-1)) ? 
			variable->dimNames[sss-c-1] : "1"); 
      if(c!=(subscripts.size()-1))
	{
	  s+= " + ";
	}
      c++;
    }	
  s+=("]");
 
  if(isStore)
    {
      s+=",";
    }
  else
    {
      s += ")";
    }

  return s;
}

int AssignExpr::commonLoopNests(AssignExpr *o)
{
  for(int i = 1; i < MAX_DEPTH; i++)
    {
      if(this->inLoops[i]==NULL)
	return (i-1);
      else if(o->inLoops[i]==NULL)
	return (i-1);
      else if(inLoops[i] != o->inLoops[i])
	return (i-1);
    }
  dbgPrintf("file %s @ line %d, be suspicious...\n",
	__FILE__, __LINE__);
  return MAX_DEPTH;
}


std::string ForStmt::getUBoundStr()
{
  std::string s;
  if(isDynamicBound)
    {
      s = dynUBound->varName;
    }
  else
    {
      s = int2string(upperBound);
    }
  return s;
}

std::string ForStmt::getLBoundStr()
{
  std::string s;
  if(isTriLowerBound)
    {
      s = "(";
      s += triLBound->inductionVar;
      s += " + " + int2string(triOffset);
      s += ")";
    }
  else
    {
      s = int2string(lowerBound);
    }
  return s;
}

void ForStmt::testLoopNest(std::list<depGraph*> &nodes)
{
  dir_t ds[MAX_DEPTH+1];
  dir_t de[MAX_DEPTH+1];

  /* walk nested loops from top
   * and build a flat list....
   * 
   * each AssignExpr keeps track of 
   * its nesting level */
  std::list<AssignExpr*> toTest;
  getAllStmts(toTest);
  getDepGraph(nodes);

  for(std::list<AssignExpr*>::iterator it = toTest.begin();
      it != toTest.end(); it++)
    {
      AssignExpr *ae = (*it);
      ForStmt *f = dynamic_cast<ForStmt*>(ae->parent);
      assert(f);
      f->populateLoops(ae);
    }
  
 
  for(std::list<AssignExpr*>::iterator it0 = toTest.begin();
      it0 != toTest.end(); it0++)
    {
      MemRef *arr_assign = dynamic_cast<MemRef*>((*it0)->left);
      ScalarRef *sca_assign = dynamic_cast<ScalarRef*>((*it0)->left);
      
      if(arr_assign)
	{
	  dd_statement s1(arr_assign);
	  
	  for(std::list<AssignExpr*>::iterator it1 = toTest.begin();
	      it1 != toTest.end(); it1++)
	     {
	       // if(it1==it0) continue;
	       
	       AssignExpr *s = *it1;
	    	    	       
	       /* first check if assignment is to the same
		* variable and build list of potential dependences */
	       std::list<Ref*> sameVarList;
	       s->checkIfSameVar(arr_assign, sameVarList);
	       dbgPrintf("sameVarList size=%d\n", (int)sameVarList.size());

	       for(std::list<Ref*>::iterator it2 = sameVarList.begin();
		   it2 != sameVarList.end(); it2++)
		 {
		   MemRef *other_mem = dynamic_cast<MemRef*>(*it2);
		   ScalarRef *other_sca = dynamic_cast<ScalarRef*>(*it2);
	   
		   /* Need this for output dependences on yourself */
		   //if(arr_assign == other_mem) {continue;}
		  
		   if(other_mem)
		     {
		       AssignExpr *ae0 = arr_assign->getAssignment();
		       AssignExpr *ae1 = other_mem->getAssignment();
		       assert(ae0!=NULL); assert(ae1!=NULL);
 
		
		       dd_statement s2(other_mem);
		       		       
		       for(int i=0; i<MAX_DEPTH+1; i++)
			 {
			   ds[i] = DIR_S;
			   de[i] = DIR_E;
			 }
		       		       
		       //int depth = min(s1.depth,s2.depth);
		       int common_nesting = ae0->commonLoopNests(ae1);
		       //dbgPrintf("COMMON_NESTING=%d\n", common_nesting);
		       
		       /* check for loop independent dependences */
		       //if(s1.depth != s2.depth)
		       if(ae0 != ae1)
			 {
			   if(banerjee(s1,s2,de,common_nesting))
			     {
			       dbgPrintf("%s(%s) to %s(%s), got %s loop indep\n",
				      ae0->assignName.c_str(),
				      arr_assign->variable->varName.c_str(),
				      ae1->assignName.c_str(),
				      other_mem->variable->varName.c_str(),
				      (s1.Id<s2.Id) ? "true" : "anti");

			       directionVector loopIndep(de);
			       /* if true dependence, the statement will occur 
				* earlier in the program and have a smaller
				* id */
			       if( (s1.Id<s2.Id) )
				 {
				   ae0->trueVectors.insert(loopIndep);
				   ae0->dG->AddEdge(ae1->dG, TRUE, 
						    LOOP_INDEP
						    );
				   
				 }
			       else
				 {
				   ae1->antiVectors.insert(loopIndep);
				   ae1->dG->AddEdge(ae0->dG, ANTI, 
						    LOOP_INDEP
						    );
				 }
			     }
			 }
		       
		       //if(ae0!=ae1)
		       if(arr_assign != other_mem)
			 {
			   if(recurseVectors(s1,s2,ae0->trueVectors,common_nesting,ds,1))
			     {
			      
			       for(std::set<directionVector>::iterator it=
				     ae0->trueVectors.begin();
				   it != ae0->trueVectors.end(); it++)
				 {
				   if(it->level!=LOOP_INDEP)
				     ae0->dG->AddEdge(ae1->dG, TRUE, it->level);
				 }
			       
			       /*
			       dbgPrintf("%s to %s: %s\n",
				      ae1->assignName.c_str(),
				      ae0->assignName.c_str(),
				      "true");
			       */
			     }
			 }
		       
		       /* flip to test for anti */
		       // if(ae0 != ae1)
		       if(arr_assign != other_mem)
			 {
			   if(recurseVectors(s2,s1,ae0->antiVectors,common_nesting,ds,1))
			     {
			      
			       for(std::set<directionVector>::iterator it=
				     ae0->antiVectors.begin();
				   it != ae0->antiVectors.end(); it++)
				 {
				   if(it->level!=LOOP_INDEP)
				     ae1->dG->AddEdge(ae0->dG, ANTI, it->level);
				 }
				 
			     }
			 }

		       /* output */
		       if(ae0 == ae1)
			 {
			   if(recurseVectors(s1,s2,ae0->outputVectors,common_nesting,de,1))
			     {
			       for(std::set<directionVector>::iterator it=
				     ae0->outputVectors.begin();
				   it != ae0->outputVectors.end(); it++)
				 {	
				   if(it->level!=LOOP_INDEP)
				     ae0->dG->AddEdge(ae1->dG, OUTPUT, it->level);
				 }
			       
			       /*
			       dbgPrintf("%s to %s: %s\n",
				      ae1->assignName.c_str(),
				      ae0->assignName.c_str(),
				      "output");
			       */ 
			     }
			 }

		     }
		   if(other_sca)
		     {
		       dbgPrintf("NOT IMPLEMENTED YET\n");
		     }
		   
		 }
	     }

	}
       else
	 {
	  
	  for(std::list<AssignExpr*>::iterator it1 = toTest.begin();
	      it1 != toTest.end(); it1++)
	     {
	       AssignExpr *s = *it1;
	       std::list<Ref*> sameVarList;
	       s->checkIfSameVar(sca_assign, sameVarList);
	     }

	  dbgPrintf("%s to %s: %s\n",
		 sca_assign->getAssignment()->assignName.c_str(),
		 sca_assign->getAssignment()->assignName.c_str(),
		 "output");
	  sca_assign->getAssignment()->dG->AddEdge(sca_assign->getAssignment()->dG, OUTPUT, 1);
	  
	 }

     }
 }

















