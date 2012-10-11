#include "scc.h"
#include "graph.h"
#include "ast.h"

#include <list>
#include <map>
#include <string>
#include <cstdio>

#include "llvm/DerivedTypes.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Support/IRBuilder.h"



#ifndef __CODEGEN__
#define __CODEGEN__


class scope
{
 public:
  scope();
  bool en_vec;
  bool en_omp;
  bool en_unroll;
  bool en_pe;
  bool en_dbg;
  bool en_shift;
  bool en_ocl;
  bool en_serial;
  bool en_aligned;
  int block_amt;
  int vec_len;
  scope* parent;
  ForStmt *myLoop;
  /* */
  bool carriesNoDep;
  int depLevel;
  
  llvm::PHINode *myIPHI;

  llvm::IRBuilder<> *llvmBuilder;
  std::list<scope*> children;
  bool initBuilder;
  std::map<ForStmt*,llvm::PHINode*> iVarMap;

  virtual void setParent(scope* p) 
  {
    myIPHI = 0;
    llvmBuilder = p->llvmBuilder;
    initBuilder=true;
    vec_len = p->vec_len;
    en_vec = p->en_vec;
    en_omp = p->en_omp;
    en_unroll = p->en_unroll;
    en_pe = p->en_pe;
    en_shift = p->en_shift;
    en_serial = p->en_serial;
    en_aligned = p->en_aligned;
    parent=p;
  }
  void setFor(ForStmt **n, int d); 
  virtual void print(int d);

  virtual std::string emit_c();
  std::string emit_ocl();

  virtual llvm::Value* emit_llvm();

  std::string makeLoopHeader(std::string &ivar, std::string lb, std::string ub, int s, bool block);
  std::string makeBlockedHeader(std::string &ivar, int lb, int ub, int s);
  void addChild(scope *s); 

  virtual bool innerMostChildDepFree();
  virtual ForStmt *outerMostChildDepFree();
  std::string loopFusion_emit_c(ForStmt *unrollLoop);
  llvm::Value *loopFusion_emit_llvm(ForStmt *unrollLoop);
  virtual bool isStatement();
  virtual bool isPerfectNest();
  virtual void setDepLevel(int num);
  void buildIVarMap(scope *p);
  
  /* loop fusion */
  void fusion();
  virtual scope* fusable(scope *s,scope *masterScope);
  bool scopeFused;
  virtual bool checkVectorizable(ForStmt *vectLoop);
  virtual std::string emitTempVars(tempVar_t t, ForStmt *l,int vecLen);

  virtual llvm::Value *emitTempVars(tempVar_t t, ForStmt *l, 	 llvm::IRBuilder<> *llBuilder,
				    std::map<ForStmt*,llvm::PHINode*> &iMap,
				    int vecLen);

  virtual std::string generateCode(bool doVec, ForStmt *l, int amt);
  virtual llvm::Value *generateLLVM(bool doVec, llvm::PHINode *oIPHI, int amt,
				    bool doAligned,
				    std::map<ForStmt*,llvm::PHINode*> &iMap);

  virtual bool simdScope();
};

class assign : public scope
{
 public:
  bool initBuilder;
 assign() : scope() 
    {
      parent=NULL;
      initBuilder=false;
      scopeFused = false;  
      myIPHI = 0;
    }
  int level;
  void setLevel(int level) {this->level = level;}
  virtual bool isStatement();
  virtual bool isPerfectNest();
  virtual void print(int d);
  virtual void setParent(scope *p)
  { parent = p; }
  AssignExpr *ae;
  virtual std::string emit_c();
  virtual llvm::Value *emit_llvm();
   
  virtual std::string traverse_par_loops(int floop, int lloop, int c);
  llvm::Value *traverse_par_loops_llvm(int floop, int lloop, int c);
  std::string traverse_par_loops_block(int floop, int lloop, int c);

  virtual ForStmt *outerMostChildDepFree();
  virtual bool innerMostChildDepFree();
  virtual void setDepLevel(int num) 
  { return; }
  virtual scope* fusable(assign *s, scope *masterScope);
  virtual scope* fusable(scope *s, scope *masterScope );
  scope *scopeFusionExpand(scope *f, int d, scope *masterScope );
  virtual bool checkVectorizable(ForStmt *vectLoop);
  virtual std::string emitTempVars(tempVar_t t, ForStmt *l,int vecLen);

  virtual llvm::Value *emitTempVars(tempVar_t t, ForStmt *l,llvm::IRBuilder<> *llBuilder,
				    std::map<ForStmt*,llvm::PHINode*> &iMap,
				    int vecLen);

  virtual std::string generateCode(bool doVec, ForStmt *l, int amt);

  virtual llvm::Value *generateLLVM(bool doVec, llvm::PHINode *oIPHI, int amt,
				    bool doAligned,
				    std::map<ForStmt*,llvm::PHINode*> &iMap);
  virtual bool simdScope();
};

void codeGen(regionGraph &rg, scope* s, int depth, int maxDepth);
llvm::IRBuilder<> *findBuilder(scope *s);
llvm::IRBuilder<> *findBuilder(assign *s);


#endif
