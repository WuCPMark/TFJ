#ifndef __IMPORTXML__
#define __IMPORTXML__
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <cstdio>
#include <map>

#include "llvm/DerivedTypes.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Support/IRBuilder.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/PassManager.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Transforms/Scalar.h"

#include <string>
#include <fstream>

#include "ast.h"

class xmlNode;
class xmlProgram;
class xmlForStmt;
class xmlLoopHeader;
class xmlLoopBody;
class xmlMemRef;
class xmlSubscript;
class xmlCompoundExpr;
class xmlAssignExpr;
class xmlVar;

class xmlNode
{
 public:
  std::string name;
  xmlNode *parent;
  xmlProgram *root;
  Node *astNode;

  xmlNode();
  ~xmlNode();
  XMLCh* TAG_Var;
  XMLCh* TAG_Nil;
  XMLCh* TAG_ForStmt;

  XMLCh* TAG_Type;
  XMLCh* TAG_Dim;

  XMLCh* TAG_Induction;
  XMLCh* TAG_Start;
  XMLCh* TAG_Stop;
  XMLCh* TAG_LoopHeader;
  XMLCh* TAG_LoopBody;

  XMLCh* TAG_MemRef;
  XMLCh* TAG_Name;
  XMLCh* TAG_Subscript;
  XMLCh* TAG_MIVInd;
  XMLCh* TAG_IndVar;
  XMLCh* TAG_IndCoeff;
  XMLCh* TAG_Offset;

  XMLCh* TAG_CompoundExpr;
  XMLCh* TAG_AssignExpr;
  XMLCh* TAG_SelectExpr;
  XMLCh* TAG_UnaryIntrin;
  XMLCh* TAG_Op;
  
  XMLCh* TAG_Sel;
  XMLCh* TAG_Left;
  XMLCh* TAG_Right;
  
  XMLCh* TAG_IntConstant;
  XMLCh* TAG_Constant;
  XMLCh* TAG_Value;

  XMLCh* TAG_Indirect;

  virtual void print() {printf("xmlNode\n");}
  virtual xmlNode *parseIt(xercesc::DOMNode *n, std::map<std::string, ForStmt*> &loopHash);
  bool getValue(xercesc::DOMElement* e, XMLCh *s, std::string &str);
  virtual xmlNode *findNode(std::string nodeName);
  Node *getASTNode();
};

class xmlProgram : public xmlNode
{
 public:
  llvm::Module *llvmModule;
  llvm::Function *llvmFunction;
  llvm::IRBuilder<> *llvmBuilder;

  std::list<xmlVar*> varList;
  std::list<xmlForStmt*> forList;
  std::list<depGraph*> nodes;

  std::map<std::string, MemRef*> 
    MemRefMap;

  /* keep a copy */
  std::map<std::string, ForStmt*> forStmtHash;

  std::string graphDump;
  bool en_vec;
  bool en_omp;
  bool en_ocl;
  bool en_unroll;
  bool en_pe;
  bool en_aligned;
  bool en_shift;
  int vec_len;
  int block_amt;
  bool en_dbg;
  bool en_fusion;
  ForStmt *outerForDepFree;
  
  int isOuterLoopPar(int &lb, int &ub, 
		     std::string &varLB,
		     std::string &varUB
		     );


 xmlProgram(bool en_vec, bool en_omp, bool en_unroll, bool en_pe, 
	    bool en_shift, int block_amt, int vec_len, 
	    bool en_fusion, bool en_aligned) : xmlNode() 
    {
      llvmBuilder = new llvm::IRBuilder<>(llvm::getGlobalContext());
      this->en_vec = en_vec;
      this->en_omp = en_omp;
      this->en_unroll = en_unroll;
      this->en_pe = en_pe;
      this->vec_len = vec_len;
      this->en_aligned = en_aligned;
      this->block_amt = block_amt;
      this->en_shift = en_shift;
      this->en_dbg = false;
      this->en_fusion = en_fusion;
      outerForDepFree = NULL;
    }
 xmlProgram() : xmlNode() 
    {
      en_unroll = en_omp = en_vec = false;
      vec_len = 4;
    }


  void printProgram();
  void printTestbench(std::string filename);
  llvm::Function *runCodeGen(llvm::Module **m, bool *perfectNest);
  void dumpDot(std::string filename);
  virtual xmlNode *parseIt(xercesc::DOMNode *n, std::map<std::string, ForStmt*> &loopHash);
  virtual xmlNode *findNode(std::string nodeName);
  std::string emitLLVMFunctionDecl();
  std::string emitFunctionDecl();
  std::string emitOCLDecl();
};

class xmlVar : public xmlNode
{
 public:
  virtual xmlNode *parseIt(xercesc::DOMNode *n, std::map<std::string, ForStmt*> &loopHash);
  std::list<int> dims;
  std::string typeStr;
  bool isInteger;
 xmlVar(): xmlNode() 
    { isInteger = false; }
  void setAstNode(Var *vVar) {astNode=vVar;} 
};


class xmlLoopHeader : public xmlNode
{
 public:
  std::string inductionVar;
  int start;
  int stop;
  /* not known at compile time */
  bool isDynamicBound;
  xmlVar *dynUBound;
  bool isTriLBound;
  ForStmt *triLBound;
  int triOffset;
 xmlLoopHeader() : xmlNode() {}
  virtual xmlNode *parseIt(xercesc::DOMNode *n, std::map<std::string, ForStmt*> &loopHash); 
};

class xmlLoopBody : public xmlNode
{
 public:
  std::list<xmlNode*> bodyList;
  std::list<Node*> astBodyList;
 xmlLoopBody() : xmlNode() {}
  virtual xmlNode *parseIt(xercesc::DOMNode *n, std::map<std::string, ForStmt*> &loopHash); 
  virtual xmlNode *findNode(std::string nodeName);
};

class xmlForStmt : public xmlNode
{
 public:
  xmlLoopHeader *header;
  xmlLoopBody *body;
 xmlForStmt() : xmlNode() 
    {
      header=0; body=0;
    } 

  virtual xmlNode *parseIt(xercesc::DOMNode *n, 
			   std::map<std::string, ForStmt*> &loopHash); 
  virtual xmlNode *findNode(std::string nodeName); 
};

class xmlMemRef : public xmlNode
{
 public:
  std::string baseVar;
  std::list<xmlSubscript *> subList;
 xmlMemRef() : xmlNode() {}
  virtual xmlNode *parseIt(xercesc::DOMNode *n, std::map<std::string, ForStmt*> &loopHash); 
  virtual xmlNode *findNode(std::string nodeName); 
};

class xmlConstant : public xmlNode
{
 public:
 xmlConstant() : xmlNode() {}
 virtual xmlNode *parseIt(xercesc::DOMNode *n, std::map<std::string, ForStmt*> &loopHash); 
 double val;
};

class xmlSubscript : public xmlNode
{
 public:
  //std::string IndVar;
  std::list<std::pair<xmlNode*, int> > indNodes;

  int Offset;
  astSubscript *astSubs;
 xmlSubscript() : xmlNode() 
    {Offset=0; astSubs=0;}
  virtual xmlNode *parseIt(xercesc::DOMNode *n, std::map<std::string, ForStmt*> &loopHash); 
  virtual xmlNode *findNode(std::string nodeName); 
};


class xmlCompoundExpr : public xmlNode
{
 public:
  op_t op;
  xmlNode *left;
  xmlNode *right;
 xmlCompoundExpr() : xmlNode() 
    {
      left=0;right=0;
    }
  virtual xmlNode *parseIt(xercesc::DOMNode *n, std::map<std::string, ForStmt*> &loopHash); 
  virtual xmlNode *findNode(std::string nodeName); 
};

class xmlSelectExpr : public xmlNode
{
 public:
  op_t op;
  xmlNode *sel;
  xmlNode *left;
  xmlNode *right;
 xmlSelectExpr() : xmlNode()
    {
      op = SEL;
      left=0; right=0; sel=0;
    }
  virtual xmlNode *parseIt(xercesc::DOMNode *n, std::map<std::string, ForStmt*> &loopHash); 
  virtual xmlNode *findNode(std::string nodeName); 
};

class xmlUnaryIntrin : public xmlNode
{
 public:
  std::string intrinStr;
  xmlNode *arg;
  op_t op;
 xmlUnaryIntrin() : xmlNode()
    {
      op = INTRIN;
      arg = 0;
    }
  virtual xmlNode *parseIt(xercesc::DOMNode *n, std::map<std::string, ForStmt*> &loopHash); 
  virtual xmlNode *findNode(std::string nodeName); 
};

class xmlAssignExpr : public xmlNode
{
 public:
  op_t op;
  xmlNode *left;
  xmlNode *right;
 xmlAssignExpr() : xmlNode() 
    {
      op = ASSIGN;
      left=0;right=0;
    }
  virtual xmlNode *parseIt(xercesc::DOMNode *n, std::map<std::string, ForStmt*> &loopHash); 
  virtual xmlNode *findNode(std::string nodeName); 
};

#endif
