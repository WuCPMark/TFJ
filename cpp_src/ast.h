#ifndef __AST__
#define __AST__

#include <cstdio>
#include <cstdlib>
#include <list>
#include <vector>
#include <map>
#include <set>
#include <string>

#include "llvm/DerivedTypes.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Support/IRBuilder.h"


/* these data structures are largely 
 * influeued by cs143 @ stanford */

enum op_t {ADD=0, SUB, MUL, DIV, LT, LTE, GT, GTE, EQ, NEQ, SEL, ASSIGN, INTRIN}; 

enum tempVar_t {UNROLL=0, VECTOR, DEFAULT};

class ForStmt;
class Ref;
class ScalarRef;
class MemRef;
class AssignExpr;

#include "banerjee.h"
#include "helper.h"
#include "graph.h"

int getConstantId();
int getId();

class astSubscript
{
 public:
  bool isIndirect;
  MemRef *indirectRef;
  int offset;
  astSubscript();
  astSubscript(int offset);
  astSubscript(MemRef *indirectRef);

  /* map induction variables to coeffs */
  std::map<ForStmt*, int> coeffs;
  int checkIfExclusiveIvar(ForStmt *f);
  void addTerm(ForStmt *f, int c)
  {
    coeffs[f] = c;
  }
  llvm::Value *getLLVM(llvm::IRBuilder<> *llvmBuilder,
		       std::map<ForStmt*, llvm::PHINode*> &iVarMap);
  /* unrolled version */
  llvm::Value *getLLVM(llvm::IRBuilder<> *llvmBuilder,
		       std::map<ForStmt*, llvm::PHINode*> &iVarMap,
		       llvm::PHINode *iVar,
		       int amt);
  std::string getString();
  std::string getLinearString();
  std::string getLinearString(ForStmt *l, int amt);
  bool ivarPresent(ForStmt *l);
  int computeStride(ForStmt *l);
  void print();

};


class Node
{
 public:
  int Id;
  Node *parent;
  std::vector<llvm::Value *> llvmValues;
  bool llvmInit;
  Node(); 
  void getNesting(std::list<Node*> &nodeList)
  {
    if(parent!=0)
      {
	parent->getNesting(nodeList);
      }
    nodeList.push_back(this);   
  }
  virtual void print(int depth)
  {
    dbgPrintf("calling virtual print\n");
  }
  virtual std::string emit_c()
  {
    std::string asCpp;
    return asCpp;
  }
  virtual llvm::Value *emit_llvm(llvm::IRBuilder<> *llvmBuilder,
				 std::map<ForStmt*, llvm::PHINode*> &iVarMap)
    {
      dbgPrintf("%s@%d: calling virtual llvm emit\n", __FILE__, __LINE__);
      return NULL;
    }

  virtual std::string emit_unrolled_c(ForStmt *l, int amt)
    {
      dbgPrintf("base unroller called\n");
      std::string asCpp;
      return asCpp;
    }
  
  virtual llvm::Value *emit_unrolled_llvm(llvm::IRBuilder<> *llvmBuilder,
					  std::map<ForStmt*, llvm::PHINode*> &iVarMap,
					  llvm::PHINode *iVar,
					  int amt)
    {
      dbgPrintf("llvm unroller called\n");
      exit(-1);
      return NULL;
    }

  virtual llvm::Value *emit_vectorized_llvm(llvm::IRBuilder<> *llvmBuilder,
					    std::map<ForStmt*, llvm::PHINode*> &iVarMap,
					    llvm::PHINode *iVar,
					    bool doAligned,
					    int vecLen)
    {
      dbgPrintf("llvm vector called\n");
      exit(-1);
      return NULL;
    }

  virtual std::string emit_vectorized_c(ForStmt *l)
    {
      return emit_unrolled_c(l,0);
    }
};

class Stmt : public Node
{
  public:
     Stmt() : Node() {}
};

class Expr : public Stmt
{
  public:
    Expr() : Stmt() {}
  virtual double cost(ForStmt **loops); 
  virtual int computeStride(ForStmt *l);
  virtual std::string emitTempVars(tempVar_t t, ForStmt *l,int vecLen);
  virtual llvm::Value *emitTempLLVMVars(tempVar_t t, llvm::IRBuilder<> *llvmBuilder,
					std::map<ForStmt*, llvm::PHINode*> &iVarMap,
					llvm::PHINode *iVar, ForStmt *l, int vecLen);
 
  virtual void collectMemRefs(std::list<MemRef*> &mList);
};

class Var : public Expr
{
 public:
  std::string varName;
  std::string typeStr;

  int dim;
  int refCount;
  bool isInteger;
  std::vector <std::string> dimNames;
  
  llvm::Value *llvmBase;
  std::vector <llvm::Value *> llvmIndices;
 
  void setDim(int dim);  
  Var(std::string varName, bool isInteger)
    {
      llvmBase = 0;
      this->isInteger = isInteger;
      dim = 0;
      refCount=0;
      this->varName=varName;
    }
  /* used for creating unique temporary
   * variable names */
  int getRefCount()
  {
    int rc = refCount;
    refCount++;
    return rc;
  }
  std::string genTempVar()
    {
      return std::string("t") + varName + "_" + int2string(getRefCount());
    }

};

class ConstantFloatVar : public Var
{
 public:
  std::string tempName;
  bool tempVarAssigned;
  float value;
  virtual void print(int depth) {dbgPrintf("%f", value);}
  virtual std::string emit_c() {return flt2string(value); }

 ConstantFloatVar(float value) : Var("floatConstant",false)
    {
      tempVarAssigned=false;
      this->value=value;
    }

  virtual std::string emit_unrolled_c(ForStmt *l, int amt)
    {
      return emit_c();
    }
  virtual std::string emitTempVars(tempVar_t t, ForStmt *l,int vecLen);
  virtual std::string emit_vectorized_c(ForStmt *l);

  virtual llvm::Value *emitTempLLVMVars(tempVar_t t, llvm::IRBuilder<> *llvmBuilder,
					std::map<ForStmt*, llvm::PHINode*> &iVarMap,
					llvm::PHINode *iVar, ForStmt *l, int vecLen);
  
  
  virtual llvm::Value* emit_llvm(llvm::IRBuilder<> *llvmBuilder,
				 std::map<ForStmt*, llvm::PHINode*> &iVarMap);

  virtual llvm::Value *emit_vectorized_llvm(llvm::IRBuilder<> *llvmBuilder,
					    std::map<ForStmt*, llvm::PHINode*> &iVarMap,
					    llvm::PHINode *iVar,
					    bool doAligned,
					    int vecLen);

  virtual llvm::Value *emit_unrolled_llvm(llvm::IRBuilder<> *llvmBuilder,
					  std::map<ForStmt*, llvm::PHINode*> &iVarMap,
					  llvm::PHINode *iVar,
					  int amt)
    {
      return emit_llvm(llvmBuilder, iVarMap);
    }
};

class ConstantDoubleVar : public Var
{
 public:
  double value;
  virtual void print(int depth) {dbgPrintf("%g", value);}
  virtual std::string emit_c() {return dbl2string(value); }
 ConstantDoubleVar(double value) : Var("doubleConstant",false)
    {this->value=value;}
};

class ConstantIntVar : public Var
{
 public:
  int value;
  virtual void print(int depth) {dbgPrintf("%d", value);}
  virtual std::string emit_c() {return int2string(value); }
 ConstantIntVar(int value) : Var("intConstant",true)
    {this->value=value;}
  
  virtual llvm::Value *emitTempLLVMVars(tempVar_t t, llvm::IRBuilder<> *llvmBuilder,
					std::map<ForStmt*, llvm::PHINode*> &iVarMap,
					llvm::PHINode *iVar, ForStmt *l, int vecLen);
  virtual llvm::Value* emit_llvm(llvm::IRBuilder<> *llvmBuilder,
				 std::map<ForStmt*, llvm::PHINode*> &iVarMap);

  virtual llvm::Value *emit_vectorized_llvm(llvm::IRBuilder<> *llvmBuilder,
					    std::map<ForStmt*, llvm::PHINode*> &iVarMap,
					    llvm::PHINode *iVar,
					    bool doAligned,
					    int vecLen);

  virtual llvm::Value *emit_unrolled_llvm(llvm::IRBuilder<> *llvmBuilder,
					  std::map<ForStmt*, llvm::PHINode*> &iVarMap,
					  llvm::PHINode *iVar,
					  int amt)
    {
      return emit_llvm(llvmBuilder, iVarMap);
    }


};


class Ref : public Expr
{
 public:
  Var *variable;
  Ref(Var *variable) 
   {this->variable = variable;}
 
  AssignExpr *getAssignment();
  
  
  virtual std::string getName()
    {
      return variable->varName;
    }

};

class ScalarRef : public Ref
{
 public:
 ScalarRef(Var *variable): Ref(variable) {}
  
  virtual std::string getName()
    {
      return variable->varName;
    }
 
  virtual std::string emit_c() 
    {return variable->varName; }
  virtual void print(int depth)
  {
    dbgPrintf("%s", variable->varName.c_str());
  }
};

class MemRef : public Ref
{
 public:
  bool tempVarAssigned;
  std::string tempName;
  bool isConstant;
  bool tempLLVMAssigned;
  ForStmt *unrollLoopVar;
  bool isStore;
  
  /* for "y[idx[i]]":
   * idx[i] is not indirect
   * y[] is indirect */
  bool isIndirect;

  bool isInteger()
  {
    return variable->isInteger;
  }

  std::vector<astSubscript*> subscripts;
  std::string vecLoadOp;
  std::string vecStoreOp;

  /* keep track of other memory refs
   * that assign to the same base variable */
  std::list<MemRef *> otherMemRefs;

  void appendOtherRef(MemRef *r)
  {
    otherMemRefs.push_back(r);
  }
  

 MemRef(Var *variable) : Ref(variable) 
  {
    tempLLVMAssigned = false;
    tempVarAssigned = false;
    isConstant = false;
    isStore=false;
    isIndirect = false;
    unrollLoopVar = NULL;
  }
  
  virtual std::string getName()
    {
      std::string s = variable->varName;
      for(std::vector<astSubscript*>::iterator it = subscripts.begin();
	  it != subscripts.end(); it++)
	{
	  s+=("[");
	  s+=(*it)->getString();
	  s+=("]");
      }	
      return s;
    }

  virtual void print(int depth)
  {
    dbgPrintf("%s", variable->varName.c_str());
    for(std::vector<astSubscript*>::iterator it = subscripts.begin();
	it != subscripts.end(); it++)
      {
	dbgPrintf("[");
	(*it)->print();
	dbgPrintf("]");
      }
  }

  virtual std::string emit_c();
  virtual std::string emit_unrolled_c(ForStmt *l, int amt);
  virtual std::string emit_vectorized_c(ForStmt *l);
  virtual llvm::Value *emit_llvm(llvm::IRBuilder<> *llvmBuilder,
				 std::map<ForStmt*, llvm::PHINode*> &iVarMap);
  virtual llvm::Value *emit_vectorized_llvm(llvm::IRBuilder<> *llvmBuilder,
					    std::map<ForStmt*, llvm::PHINode*> &iVarMap,
					    llvm::PHINode *iVar,
					    bool doAligned,
					    int vecLen);
  virtual llvm::Value *emit_unrolled_llvm(llvm::IRBuilder<> *llvmBuilder,
					  std::map<ForStmt*, llvm::PHINode*> &iVarMap,
					  llvm::PHINode *iVar,
					  int amt);
  virtual double cost(ForStmt **loops);
  virtual int computeStride(ForStmt *l);
  virtual std::string emitTempVars(tempVar_t t, ForStmt *l, int vecLen);
  llvm::Value *emitTempLLVMVars(tempVar_t t, llvm::IRBuilder<> *llvmBuilder,
				std::map<ForStmt*, llvm::PHINode*> &iVarMap,
				llvm::PHINode *iVar, ForStmt *l, int vecLen);

  virtual void collectMemRefs(std::list<MemRef*> &mList);
  void addSubscript(astSubscript* s)
  {
    subscripts.push_back(s);
  }
  

};

class CompoundExpr: public Expr
{
 public:
  op_t op;
  Expr *left, *right;
  bool isIntegerOp;
  llvm::Value *leftValue;
  llvm::Value *rightValue;

  std::vector<std::string> vecIntrinsics;
  CompoundExpr(op_t op, Expr *left, Expr *right)
    {
      isIntegerOp = false;
     
      if(left)
	left->parent = this;
      if(right)
	right->parent = this;

      this->op = op;
      this->left = left; this->right=right;
    }
  virtual double cost(ForStmt **c);
  virtual void checkIfSameVar(Ref *x, std::list<Ref*> &sameVarList);
  virtual void print(int depth);
 
  bool foundIntegerOps(std::list<MemRef*> &ml);
  
  virtual std::string emit_c();
  virtual llvm::Value *emit_llvm(llvm::IRBuilder<> *llvmBuilder,
				 std::map<ForStmt*, llvm::PHINode*> &iVarMap);
  virtual llvm::Value *emit_vectorized_llvm(llvm::IRBuilder<> *llvmBuilder,
					    std::map<ForStmt*, llvm::PHINode*> &iVarMap,
					    llvm::PHINode *iVar,
					    bool doAligned,
					    int vecLen);
  virtual llvm::Value *emit_unrolled_llvm(llvm::IRBuilder<> *llvmBuilder,
					  std::map<ForStmt*, llvm::PHINode*> &iVarMap,
					  llvm::PHINode *iVar,
					  int amt);
  llvm::Value *emitTempLLVMVars(tempVar_t t, llvm::IRBuilder<> *llvmBuilder,
				std::map<ForStmt*, llvm::PHINode*> &iVarMap,
				llvm::PHINode *iVar, ForStmt *l, int vecLen);
  virtual int computeStride(ForStmt *l);
  virtual std::string emit_unrolled_c(ForStmt *l, int amt);
  virtual std::string emit_vectorized_c(ForStmt *l);
  virtual std::string emitTempVars(tempVar_t t, ForStmt *l,int vecLen);
  virtual void collectMemRefs(std::list<MemRef*> &mList);
};

class SelectExpr: public CompoundExpr
{
 public:
  Expr *sel;
  llvm::Value *selValue;
  virtual void checkIfSameVar(Ref *x, std::list<Ref*> &sameVarList);
 SelectExpr(Expr *sel, Expr *left, Expr *right) : 
  CompoundExpr(SEL, left, right)
    {
      sel->parent = this;
      op = SEL;
      this->sel = sel;
    }
  virtual std::string emit_c();
  virtual int computeStride(ForStmt *l);
  virtual std::string emit_unrolled_c(ForStmt *l, int amt);
  virtual std::string emit_vectorized_c(ForStmt *l);
  virtual llvm::Value *emit_vectorized_llvm(llvm::IRBuilder<> *llvmBuilder,
					    std::map<ForStmt*, llvm::PHINode*> &iVarMap,
					    llvm::PHINode *iVar,
					    bool doAligned,
					    int vecLen);
  virtual llvm::Value *emit_unrolled_llvm(llvm::IRBuilder<> *llvmBuilder,
					  std::map<ForStmt*, llvm::PHINode*> &iVarMap,
					  llvm::PHINode *iVar,
					  int amt);
  virtual std::string emitTempVars(tempVar_t t, ForStmt *l,int vecLen);
  llvm::Value *emitTempLLVMVars(tempVar_t t, llvm::IRBuilder<> *llvmBuilder,
				std::map<ForStmt*, llvm::PHINode*> &iVarMap,
				llvm::PHINode *iVar, ForStmt *l, int vecLen);
  virtual void collectMemRefs(std::list<MemRef*> &mList);
  virtual void print(int depth);
  virtual llvm::Value *emit_llvm(llvm::IRBuilder<> *llvmBuilder,
				 std::map<ForStmt*, llvm::PHINode*> &iVarMap);
};

class UnaryIntrin: public CompoundExpr
{
 public:
  std::string intrinString;
  virtual void checkIfSameVar(Ref *x, std::list<Ref*> &sameVarList);
 UnaryIntrin(Expr *left, std::string intrinString) : 
  CompoundExpr(INTRIN, left, NULL)
    {
      left->parent = this;
      op = INTRIN;
      this->intrinString = intrinString;
    }
  virtual std::string emit_c();
  virtual int computeStride(ForStmt *l);
  virtual std::string emit_unrolled_c(ForStmt *l, int amt);
  virtual std::string emit_vectorized_c(ForStmt *l);
  virtual llvm::Value *emit_vectorized_llvm(llvm::IRBuilder<> *llvmBuilder,
					    std::map<ForStmt*, llvm::PHINode*> &iVarMap,
					    llvm::PHINode *iVar,
					    bool doAligned,
					    int vecLen);
  virtual llvm::Value *emit_unrolled_llvm(llvm::IRBuilder<> *llvmBuilder,
					  std::map<ForStmt*, llvm::PHINode*> &iVarMap,
					  llvm::PHINode *iVar,
					  int amt);
  virtual std::string emitTempVars(tempVar_t t, ForStmt *l,int vecLen);
  llvm::Value *emitTempLLVMVars(tempVar_t t, llvm::IRBuilder<> *llvmBuilder,
				std::map<ForStmt*, llvm::PHINode*> &iVarMap,
				llvm::PHINode *iVar, ForStmt *l, int vecLen);
  virtual void collectMemRefs(std::list<MemRef*> &mList);
  virtual void print(int depth);
  virtual llvm::Value *emit_llvm(llvm::IRBuilder<> *llvmBuilder,
				 std::map<ForStmt*, llvm::PHINode*> &iVarMap);
};

class AssignExpr: public CompoundExpr
{
 public:
  std::string assignName;
  depGraph *dG;
  ForStmt* inLoops[MAX_DEPTH+1];
  void shiftLoopLevels(int k, int p);
  void findBestLoopPermutation(int first, int last);
  bool permuteIt(int first, int last,
		 std::vector<int> &assigned,
		 std::vector<int> &unassigned);

  std::list<MemRef*> memoryRefs;

  virtual llvm::Value *emit_llvm(llvm::IRBuilder<> *llvmBuilder,
				 std::map<ForStmt*, llvm::PHINode*> &iVarMap);
  virtual llvm::Value *emit_vectorized_llvm(llvm::IRBuilder<> *llvmBuilder,
					    std::map<ForStmt*, llvm::PHINode*> &iVarMap,
					    llvm::PHINode *iVar,
					    bool doAligned,
					    int vecLen);
  virtual llvm::Value *emit_unrolled_llvm(llvm::IRBuilder<> *llvmBuilder,
					  std::map<ForStmt*, llvm::PHINode*> &iVarMap,
					  llvm::PHINode *iVar,
					  int amt);
  llvm::Value *emitTempLLVMVars(tempVar_t t, llvm::IRBuilder<> *llvmBuilder,
				std::map<ForStmt*, llvm::PHINode*> &iVarMap,
				llvm::PHINode *iVar, ForStmt *l, int vecLen);
  std::set<directionVector> trueVectors;
  std::set<directionVector> antiVectors;
  std::set<directionVector> outputVectors;
  void printVectors();
  void printLoops();
  int deepestLevel();
  double iterationCost();
  /* constant and stride-1 memory access */
  bool SIMDizable(ForStmt *l);
  virtual std::string emitTempVars(tempVar_t t, ForStmt *l,int vecLen);
  
 AssignExpr(std::string assignName, Expr *left, Expr *right) : CompoundExpr(ASSIGN, left, right) 
    {
      for(int i=0;i<(MAX_DEPTH+1);i++)
	inLoops[i] = NULL;

      dG = new depGraph(assignName);
      dG->ae = this;
      this->assignName = assignName;
    }

  virtual void print(int depth);
 
  virtual std::string emit_c();
  void checkDepth(int level);
  virtual std::string emit_unrolled_c(ForStmt *l, int amt);
  virtual std::string emit_vectorized_c(ForStmt *l);

  int commonLoopNests(AssignExpr *o);
};


class ForStmt: public Expr
{
 public:
  std::string inductionVar;
  int lowerBound;
  int upperBound;
  int step;  
  bool isDynamicBound;
  Var *dynUBound;

  bool isTopLoop;

  bool isTriLowerBound;
  ForStmt *triLBound;
  int triOffset;

  std::string sliceStart;
  std::string sliceStop;

  llvm::Value *lsliceStart;
  llvm::Value *lsliceStop;

  std::list<Expr*> body;


  ForStmt(int lb, int ub, int s, bool isDynamicBound, Var *dynUBound,
	  bool isTriLowerBound, ForStmt *triLBound, int triOffset,
	  std::string ivar)
    {
      isTopLoop = false;
      lowerBound = lb; 
      upperBound = ub; 
      step = s;
      inductionVar = ivar;
      this->dynUBound = dynUBound;
      this->isDynamicBound = isDynamicBound;
      this->isTriLowerBound = isTriLowerBound;
      this->triLBound = triLBound;
      this->triOffset = triOffset;
    }
  
  void addBodyStmt(Expr *s)
  {
    s->parent = this;
    body.push_back(s);
  }

  void setTopLoop()
  {
    sliceStart = inductionVar + "Start";
    sliceStop = inductionVar + "Stop";
    isTopLoop = true;
  }

  void getAllStmts(std::list<AssignExpr*> &toTest);
  void getDepGraph(std::list<depGraph*> &graph);

  void testLoopNest(std::list<depGraph*> &nodes);
  int populateLoops(AssignExpr *ae);

  bool isDynUBound()
  {
    if(isDynamicBound)
      {
	if(dynUBound==0)
	  {
	    dbgPrintf("dynamic ubound set but variable null\n");
	    exit(-1);
	  }
      }
    return isDynamicBound;
  }

  std::string getUBoundStr();
  std::string getLBoundStr();

  virtual void print(int depth);
};

#endif
