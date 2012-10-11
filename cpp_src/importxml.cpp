#include <stdexcept>

#include <iostream>
#include <cstdio>
#include <cassert>
#include <unistd.h>

/* headers from our project */
#include "codegen.h"
#include "importxml.h"
#include "helper.h"

using namespace std;
using namespace xercesc;
using namespace llvm;

static bool dbgParsePrint=false;

static bool parseTypeString(std::string &str)
{
  size_t pos = str.find("int");
  if(pos != string::npos)
    return true;
  else return false;
}


xmlNode::xmlNode() 
{
  parent=0;
  root = 0;
  astNode = 0;
  TAG_Var = XMLString::transcode("Var");
  TAG_Nil = XMLString::transcode("");

  TAG_Type = XMLString::transcode("Type");
  TAG_Dim = XMLString::transcode("Dim");
  
  TAG_ForStmt = XMLString::transcode("ForStmt");
  TAG_Induction = XMLString::transcode("Induction");
  TAG_Start = XMLString::transcode("Start");
  TAG_Stop = XMLString::transcode("Stop");
  TAG_LoopHeader = XMLString::transcode("LoopHeader");
  TAG_LoopBody = XMLString::transcode("LoopBody");
  TAG_MemRef = XMLString::transcode("MemRef");
  TAG_Name = XMLString::transcode("Name");
  TAG_Subscript = XMLString::transcode("Subscript");

  TAG_IndVar = XMLString::transcode("IndVar");
  TAG_MIVInd = XMLString::transcode("MIVInd");
  
  TAG_IndCoeff = XMLString::transcode("IndCoeff");
  TAG_Offset = XMLString::transcode("Offset");

  TAG_CompoundExpr = XMLString::transcode("CompoundExpr");
  TAG_AssignExpr = XMLString::transcode("AssignExpr");
  TAG_SelectExpr = XMLString::transcode("SelectExpr");
  TAG_UnaryIntrin = XMLString::transcode("UnaryIntrin");

  TAG_Op = XMLString::transcode("Op");
  TAG_Left = XMLString::transcode("Left");
  TAG_Sel = XMLString::transcode("Sel");
  TAG_Right = XMLString::transcode("Right");


  TAG_Constant = XMLString::transcode("Constant");
  TAG_IntConstant = XMLString::transcode("IntConstant");

  TAG_Value = XMLString::transcode("Value");

  TAG_Indirect = XMLString::transcode("IndirectRef");
}

xmlNode::~xmlNode()
{
  if(TAG_Var) XMLString::release(&TAG_Var);
  if(TAG_Nil) XMLString::release(&TAG_Nil);

  if(TAG_Type) XMLString::release(&TAG_Type);
  if(TAG_Dim) XMLString::release(&TAG_Dim);

  if(TAG_ForStmt) XMLString::release(&TAG_ForStmt);
  if(TAG_Induction) XMLString::release(&TAG_Induction);
  if(TAG_Start) XMLString::release(&TAG_Start);
  if(TAG_Stop) XMLString::release(&TAG_Stop);
  if(TAG_LoopHeader) XMLString::release(&TAG_LoopHeader);
  if(TAG_LoopBody) XMLString::release(&TAG_LoopBody);
  
  if(TAG_MemRef) XMLString::release(&TAG_MemRef);
  if(TAG_Name) XMLString::release(&TAG_Name);
  if(TAG_Subscript) XMLString::release(&TAG_Subscript);
  if(TAG_IndVar) XMLString::release(&TAG_IndVar);
  if(TAG_MIVInd) XMLString::release(&TAG_MIVInd);

  if(TAG_IndCoeff) XMLString::release(&TAG_IndCoeff);
  if(TAG_Offset) XMLString::release(&TAG_Offset);

  if(TAG_CompoundExpr) XMLString::release(&TAG_CompoundExpr);
  if(TAG_AssignExpr) XMLString::release(&TAG_AssignExpr);
  if(TAG_UnaryIntrin) XMLString::release(&TAG_UnaryIntrin);

  if(TAG_SelectExpr) XMLString::release(&TAG_SelectExpr);
  if(TAG_Op) XMLString::release(&TAG_Op);
  if(TAG_Left) XMLString::release(&TAG_Left);
  if(TAG_Sel) XMLString::release(&TAG_Sel);
  if(TAG_Right) XMLString::release(&TAG_Right);

  if(TAG_Constant) XMLString::release(&TAG_Constant);
  if(TAG_IntConstant) XMLString::release(&TAG_IntConstant);

  if(TAG_Value) XMLString::release(&TAG_Value);
  if(TAG_Indirect) XMLString::release(&TAG_Indirect);
}

Node *xmlNode::getASTNode()
{
  return astNode;
}

xmlNode *xmlNode::parseIt(xercesc::DOMNode *n, std::map<std::string, ForStmt*> &loopHash) 
{
  dbgPrintf("parse called on baseclass\n");
  return NULL;
}

xmlNode *xmlNode::findNode(std::string nodeName)
{
  dbgPrintf("find node made it to baseclass\n");
  return NULL;
}

bool xmlNode::getValue(DOMElement* e, XMLCh *s, std::string &str)
{
  if( XMLString::equals(e->getTagName(), s))
    {
      DOMNode *varChild = e->getFirstChild();
      if(varChild) 
	{
	  char *cstr = 0;
	  cstr = XMLString::transcode(varChild->getNodeValue());
	  
	  if(cstr){
	    str += std::string(cstr);
	    return true;
	  }
	}
    }
  return false;
}

int xmlProgram::isOuterLoopPar(int &lb, int &ub, 
			       std::string &varLB,
			       std::string &varUB)
{

  if((outerForDepFree!=0) && en_pe)
    {
      ub = outerForDepFree->upperBound;
      lb = outerForDepFree->lowerBound;
      
      if(outerForDepFree->dynUBound)
	{
	  varUB = outerForDepFree->dynUBound->varName;
	}
      return (outerForDepFree->upperBound)-
	(outerForDepFree->lowerBound);
    }
  else
    {
      return -1;
    }
}


xmlNode* xmlProgram::findNode(std::string nodeName)
{
  for(std::list<xmlVar*>::iterator it = varList.begin();
      it != varList.end(); it++)
    {
      xmlVar *vVar = *it;
      /*
      dbgPrintf("%s,%s\n",
	     vVar->name.c_str(),
	     nodeName.c_str());
      */
      if(vVar->name.compare(nodeName)==0)
	{
	  //dbgPrintf("foudn\n");
	  return vVar;
	}
    }
  dbgPrintf("unable to find %s\n", nodeName.c_str());
  exit(-1);
  return NULL;
}
void xmlProgram::printProgram()
{
  ForStmt *topLevelFor = dynamic_cast<ForStmt*>(astNode);
  assert(topLevelFor);
  dbgPrintf("\n\nSource code:\n");
  topLevelFor->print(0);
  dbgPrintf("\n\n");

}

void xmlProgram::dumpDot(std::string filename)
{
  std::string dotName = filename+".dot";
  FILE *fp = fopen(dotName.c_str(), "w");
  fprintf(fp, "%s\n", graphDump.c_str());
  fclose(fp);
}

void xmlProgram::printTestbench(std::string filename)
{
  FILE * fp = fopen(filename.c_str(), "w");
  std::string testStr = "";
  std::list<std::string> paramStr;
  testStr += "#include <cstdio>\n";
  testStr += "#include <cstdlib>\n";
  testStr += "#include \"timestamp.h\"\n";
  // Prototype for emitted function
  testStr += emitFunctionDecl() + ";\n\n";
  testStr += "int main()\n{\n";

  // Load in input data
  int varnum = 0;
  for(std::list<xmlVar*>::iterator it=varList.begin();
      it != varList.end(); it++, varnum++)
    {
      
      xmlVar *v = *it;
      if(v->isInteger) {
	continue;
      }

      paramStr.push_back("input_" + int2string(varnum));
      int nelements = 1;
      for(std::list<int>::reverse_iterator dim = v->dims.rbegin() ;
          dim != v->dims.rend() ; dim++) 
      {
	if(dim != v->dims.rbegin()) {
	  paramStr.push_back(int2string(nelements));
	}
        nelements *= *dim;
      }

      testStr += v->typeStr + " * " + "input_" + int2string(varnum) + " = new " + v->typeStr + "[" + 
      		int2string(nelements) + "];\n";
      testStr += v->typeStr + " * " + "output_" + int2string(varnum) + " = new " + v->typeStr + "[" + 
      		int2string(nelements) + "];\n";
	
      // Load data
      testStr += "FILE * fp_in" + int2string(varnum) + " = fopen(\"input_" + int2string(varnum) + 
              ".bin\", \"r\");\n";
      testStr += "FILE * fp_out" + int2string(varnum) + " = fopen(\"output_" + int2string(varnum) + 
              ".bin\", \"r\");\n";
      testStr += "fread(input_" + int2string(varnum) + ", sizeof(" + v->typeStr + "), " + 
              int2string(nelements) + 
              ", fp_in" + int2string(varnum) + ");\n";
      testStr += "fread(output_" + int2string(varnum) + ", sizeof(" + v->typeStr + "), " 
              + int2string(nelements) + 
              ", fp_out" + int2string(varnum) + ");\n";

      testStr += "fclose(fp_in" + int2string(varnum) + ");\n";
      testStr += "fclose(fp_out" + int2string(varnum) + ");\n";
    }
 
  // Run test
  testStr += "double ts = timestamp();\n";
  testStr += name + "(";
  for(std::list<std::string>::iterator it=paramStr.begin();
      it != paramStr.end() ; ) {
    testStr += *it;
    if(++it != paramStr.end()) {
      testStr += ", ";
    }
  }
  testStr += ");\n"; 
  testStr += "printf(\"Time elapsed: %f\\n\", timestamp()-ts);\n";

  // Check output data
  varnum = 0;
  for(std::list<xmlVar*>::iterator it=varList.begin();
      it != varList.end(); it++, varnum++)
    {
      xmlVar *v = *it;
      if(v->isInteger) {
	continue;
      }
      
      int nelements = 1;
      for(std::list<int>::iterator dim = v->dims.begin() ;
          dim != v->dims.end() ; dim++) 
      {
        nelements *= *dim;
      }

      testStr += "for(int i = 0 ; i < " + int2string(nelements) + "; i++)\n{\n";
      testStr += "\t" + v->typeStr + " d = input_" + int2string(varnum) + "[i] - " +
                 "output_" + int2string(varnum) + "[i];\n";
      testStr += "\td = (d > 0) ? d : -d;\n";
      testStr += "\tif(d > 0.05)\n\t{\n";
      testStr += "\t\tprintf(\"" + v->name + " mismatch at \%d\\n\", i);\n";
      testStr += "\t\tprintf(\"gold: \%f, jack: \%f\\n\", input_" + 
                 int2string(varnum) + "[i], " +
		 "output_" + int2string(varnum) + "[i]);\n";
      testStr += "\t\tbreak;\n\t}\n}\n";
      
      testStr += "delete [] input_" + int2string(varnum) + ";\n";
      testStr += "delete [] output_" + int2string(varnum) + ";\n";
    }

  testStr += "}\n";
  fprintf(fp,"%s\n", testStr.c_str());
  fclose(fp);

}
std::string xmlProgram::emitOCLDecl()
{
  std::string asCpp;
  if(en_vec)
    {
      asCpp += "#include \"vect4.h\" \n\n";
    }
  asCpp += "__kernel void " + name + " (";

  size_t numVars = varList.size();
  size_t c=0;
  for(std::list<xmlVar*>::iterator it=varList.begin();
      it != varList.end(); it++)
    {
      xmlVar *v = *it;
      
      if(v->dims.size() > 0)
	{
	  asCpp += "__global " + v->typeStr + " * " + v->name;
     	}
      else
	{
	  /* just a scalar argument */
	  string sDecl = v->typeStr + " " + v->name; 
	  asCpp += sDecl;
	}
      
      if(v->dims.size() > 1) 
	{
	  asCpp += ",";
	  Var *vVar = dynamic_cast<Var*>(v->astNode);
	  assert(vVar);
	  
	  int s = vVar->dimNames.size();
	  for(int i = 0; i < s;i++)
	    {
	      std::string sOff = "int " + vVar->dimNames[i];
	      
	      if(i==(s-1))
		{
		  asCpp += sOff;
		}
	      else if(i>0)
		{
		  asCpp += sOff + ",";
		}
	    }
	}
      
      if((c+1)!=numVars)
	asCpp += ",";
	
      c++;
    }
  asCpp += ")\n";
  return asCpp;
}

std::string xmlProgram::emitFunctionDecl()
{
  std::string asCpp;

  if(en_vec || en_pe)
    {
      asCpp += "#include \"vectorIntrinsics.h\" \n\n";
    }

  asCpp += "void " + name + " (";

  size_t numVars = varList.size();
  size_t c=0;
  for(std::list<xmlVar*>::iterator it=varList.begin();
      it != varList.end(); it++)
    {
      xmlVar *v = *it;
      
      if(v->dims.size() > 0)
	{
	  asCpp += v->typeStr + "*__restrict__ " + v->name;
	}
      else
	{
	  /* just a scalar argument */
	  string sDecl = v->typeStr + " " + v->name; 
	  asCpp += sDecl;
	}
      
      if(v->dims.size() > 1) 
	{
	  asCpp += ",";
	  Var *vVar = dynamic_cast<Var*>(v->astNode);
	  assert(vVar);
	  
	  int s = vVar->dimNames.size();
	  for(int i = 0; i < s;i++)
	    {
	      std::string sOff = "int " + vVar->dimNames[i];
	      
	      if(i==(s-1))
		{
		  asCpp += sOff;
		}
	      else if(i>0)
		{
		  asCpp += sOff + ",";
		}
	    }
	}
      
      if((c+1)!=numVars)
	asCpp += ",";
	
      c++;
    }
  asCpp += ")\n";
  return asCpp;
}

std::string xmlProgram::emitLLVMFunctionDecl()
{
  std::string asCpp;

  if(en_vec || en_pe)
    {
      asCpp += "#include \"vectorIntrinsics.h\" \n\n";
    }

  asCpp += "void " + name + " (";

  size_t numVars = varList.size();
  size_t c=0;
  
#ifdef USE_LLVM_30
  vector<Type*> llvmFunctArgs;
#else
  vector<const Type*> llvmFunctArgs;
#endif

  vector<string> llvmArgNames;

  map<string, Value*> llvmValueMap;
  
  if(outerForDepFree != NULL && en_pe)
    {
      asCpp += "int " + outerForDepFree->sliceStart + " , "; 
      asCpp += "int " + outerForDepFree->sliceStop +  " , "; 
      llvmFunctArgs.push_back(Type::getInt32Ty(getGlobalContext()));
      llvmFunctArgs.push_back(Type::getInt32Ty(getGlobalContext()));
      llvmArgNames.push_back(outerForDepFree->sliceStart);
      llvmArgNames.push_back(outerForDepFree->sliceStop);
    }

  for(std::list<xmlVar*>::iterator it=varList.begin();
      it != varList.end(); it++)
    {
      xmlVar *v = *it;
      
      if(v->dims.size() == 0)
	{
	  llvmFunctArgs.push_back(Type::getInt32Ty(getGlobalContext()));
	  asCpp += v->typeStr + " " + v->name; 
	}
      else
	{
	  if(v->isInteger) {
	    llvmFunctArgs.push_back(Type::getInt32PtrTy(getGlobalContext()));
	  } else {
	    llvmFunctArgs.push_back(Type::getFloatPtrTy(getGlobalContext()));
	  }

	  if(v->dims.size() > 0)
	    {
	      asCpp += v->typeStr + "*__restrict__ " + v->name;
	    }
	}

      llvmArgNames.push_back(v->name);
      assert(v->astNode);
      
      if(v->dims.size() > 1) 
	{
	  asCpp += ",";
	  Var *vVar = dynamic_cast<Var*>(v->astNode);
	  assert(vVar);
	  
	  int s = vVar->dimNames.size();
	  for(int i = 0; i < s;i++)
	    {
	      std::string sOff = "int " + vVar->dimNames[i];
	      
	      if(i>0)
		{
		  llvmFunctArgs.push_back(Type::getInt32Ty(getGlobalContext()));
		  llvmArgNames.push_back(vVar->dimNames[i]);
		}

	      if(i==(s-1))
		{
		  asCpp += sOff;
		}
	      else if(i>0)
		{
		  asCpp += sOff + ",";
		}
	      
	    }
	}
      
      if((c+1)!=numVars)
	asCpp += ",";
	
      c++;
    }
 
  asCpp += ")\n";
  
 
#ifdef USE_LLVM_30
  ArrayRef<Type*> arFunctArgs(llvmFunctArgs);

  FunctionType *FT = llvm::FunctionType::get
    (Type::getVoidTy(getGlobalContext()),
     arFunctArgs,
     false);

  llvmFunction = 
    Function::Create(FT, llvm::Function::ExternalLinkage, 
			   name, llvmModule);
#else

  FunctionType *FT = llvm::FunctionType::get
    (Type::getVoidTy(getGlobalContext()),
     llvmFunctArgs,false);

  llvmFunction = 
    Function::Create(FT, llvm::Function::ExternalLinkage, 
			   name, llvmModule);
#endif

  size_t Idx=0;
  for (Function::arg_iterator AI = llvmFunction->arg_begin(); Idx != llvmArgNames.size();
       ++AI, ++Idx) 
    {
      AI->setName(llvmArgNames[Idx]);
      llvmValueMap[llvmArgNames[Idx]] = AI;
      
      const Type *argType = (*AI).getType();
   
      
      /* This will always be true for the
       * type of code we generate in
       * 3 fingered jack */
      if(argType->isPointerTy() == true)
	{
	  //llvm::errs() << *argType << "\n"; 
	  /* First parameter starts @ 1 instead of 0
	   * (Zero used for return) */
	  llvmFunction->setDoesNotAlias(Idx+1);
	}
    }
  
  if(outerForDepFree != NULL && en_pe)
    {
      outerForDepFree->lsliceStart = 
	llvmValueMap[outerForDepFree->sliceStart];
      outerForDepFree->lsliceStop = 
	llvmValueMap[outerForDepFree->sliceStop];
    }


  //add hash map here
 for(std::list<xmlVar*>::iterator it=varList.begin();
      it != varList.end(); it++)
    {
      xmlVar *v = *it;
      Var *vVar = dynamic_cast<Var*>(v->astNode);
      assert(vVar);

      vVar->llvmBase = llvmValueMap[v->name];
      
      if(v->dims.size() > 1) 
	{
	  int s = vVar->dimNames.size();
	  for(int i = 0; i < s;i++)
	    {
	      vVar->llvmIndices.push_back(llvmValueMap[vVar->dimNames[i]]);
	    }
	}
    }
 
 return asCpp;
}

llvm::Function* xmlProgram::runCodeGen(llvm::Module **m=NULL, bool *perfectNest=NULL)
{
  ForStmt *topLevelFor = dynamic_cast<ForStmt*>(astNode);
  scope *s = NULL;
  scope *s0, *s1;

  topLevelFor->testLoopNest(nodes);
  graphDump = dumpGraph(nodes);
  
  this->en_ocl = true;
  this->en_omp = 
    this->en_ocl ? false : this->en_omp;

  s0 = new scope();
 
  bool xchgSafe = true;
  for(std::map<std::string, ForStmt*>::iterator mit = forStmtHash.begin();
      mit != forStmtHash.end(); mit++)
    {
      ForStmt *f = mit->second;
      if(f->isTriLowerBound)
	{
	  xchgSafe = false;
	  break;
	}
    }
  s0->en_serial = !xchgSafe;
  
 /* setup vectorization parameters */
  s0->vec_len = this->vec_len;
  s0->en_vec = this->en_vec;
  s0->en_unroll = this->en_unroll;
  s0->en_omp = this->en_omp;
  s0->en_pe = this->en_pe;
  s0->en_aligned = this->en_aligned;
  s0->block_amt = this->block_amt;
  s0->llvmBuilder = this->llvmBuilder;
  s0->en_dbg = this->en_dbg;
  s0->en_shift = false;
  
  regionGraph r0(nodes);  
  codeGen(r0,s0,1, (int)forStmtHash.size());
  s = s0;
  
  if(!(s0->simdScope() && s0->innerMostChildDepFree()) && 
     xchgSafe ) 
    {
      dbgPrintf("s0: inner loop has dep\n\n");
      s1 = new scope();
      s1->vec_len = this->vec_len;
      s1->en_vec = this->en_vec;
      s1->en_unroll = this->en_unroll;
      s1->en_omp = this->en_omp;
      s1->en_pe = this->en_pe;
      s1->en_aligned = this->en_aligned;
      s1->block_amt = this->block_amt;
      s1->llvmBuilder = this->llvmBuilder;
      s1->en_dbg = this->en_dbg;
      s1->en_shift = true; //this->en_shift;
      
      codeGen(r0,s1,1,(int)forStmtHash.size());
      s = s1;
      if((!s1->simdScope()) && s0->simdScope())
	{
	  s = s0;
	}
    }

  /* TODO: Try loop fusion here */
  if(en_fusion)
    {
      dbgPrintf("WARNING: loop fusion is enabled\n");
      s->fusion();
    }

  dbgPrintf("\n\n\t\tAFTER CODEGEN: \n\n");
  s->print(1);
    
  outerForDepFree = 
    s->outerMostChildDepFree();

  std::string asCpp;
  
  llvm::LLVMContext &Context = 
    llvm::getGlobalContext();

  std::string llvmModuleName = name + "_module";
  llvmModule = new llvm::Module(llvmModuleName, Context);
  if(m!=NULL)
    {
      *m = llvmModule;
    }

  asCpp += emitLLVMFunctionDecl();
  asCpp += "{\n";
  asCpp += s->emit_c();
  asCpp += "}\n";
  std::string fname = name + ".cc";
  FILE *fp = fopen(fname.c_str(), "w");
  fprintf(fp, "%s", asCpp.c_str());
  fclose(fp);


  /*
  if(outerForDepFree != NULL)
    {
      printf("Can generate OpenCL :)\n");
      std::string asOCL;
      asOCL += emitOCLDecl();
      asOCL += "{\n";
      asOCL += s->emit_ocl();
      asOCL += "}\n";
      
      std::string clname = name + ".cl";
      FILE *fpCL = fopen(clname.c_str(), "w");
      fprintf(fpCL, "%s", asOCL.c_str());
      fclose(fpCL);
    }
  */
  //std::string test_fname = name + "Test.cc";
  //printTestbench(test_fname);
  
  //if(s->en_vec==false)
    {
      std::string topBBName = name +"entry";
      BasicBlock *topBB = 
	BasicBlock::Create(getGlobalContext(), topBBName, llvmFunction);
      
      llvmBuilder->SetInsertPoint(topBB);
      
      s->emit_llvm();
      if(perfectNest)
	{
	  *perfectNest = s->isPerfectNest();
	  dbgPrintf("is a perfect nest? = %s\n",
		 *perfectNest ? "yes" : "no");
	}
 
      llvmBuilder->CreateRetVoid();
      //llvmModule->dump();
  
      if(en_dbg) {
	verifyModule(*llvmModule, PrintMessageAction);
      } else {
	verifyModule(*llvmModule, AbortProcessAction);  
      }
      /* copied from LLVM tutorial:
       * http://llvm.org/docs/tutorial/LangImpl4.html
       */
      
      
      
      llvm::FunctionPassManager OurFPM(llvmModule);
      // Provide basic AliasAnalysis support for GVN.
      OurFPM.add(createBasicAliasAnalysisPass());
      // Do simple "peephole" optimizations and bit-twiddling optzns.
      OurFPM.add(createInstructionCombiningPass());
      // Reassociate expressions.
      OurFPM.add(createReassociatePass());
      // Eliminate Common SubExpressions.
      OurFPM.add(createGVNPass());
      // Simplify the control flow graph (deleting unreachable blocks, etc).
      OurFPM.add(createCFGSimplificationPass());
      
      OurFPM.add(createLICMPass());
      OurFPM.doInitialization();
      
      
      OurFPM.run(*llvmFunction);
      

      if(en_dbg) {
	dbgPrintf("\n\nLLVM dump begins now:\n");
	llvmModule->dump();
      }

      /* copied from tart */
      std::string bitname= name + ".bc"; 
      std::string errorInfo;
      
      // raw_fd_ostream bcOut(bitname.c_str(), errorInfo, true);
      raw_fd_ostream bcOut(bitname.c_str(),errorInfo, false);
      WriteBitcodeToFile(llvmModule, bcOut);
      
      bcOut.close();

      return llvmFunction;
    }
}


xmlNode *xmlConstant::parseIt(xercesc::DOMNode *n, std::map<std::string, ForStmt*> &loopHash)
 {
  DOMNodeList*      children = n->getChildNodes();
  const  XMLSize_t nodeCount = children->getLength();
  //Var *vVar = 0;
  /*parse the loop header */
  for( XMLSize_t xx = 0; xx < nodeCount; ++xx )
    {
      DOMNode* currentNode = children->item(xx);
      if( currentNode->getNodeType() &&  // true is not NULL
	  currentNode->getNodeType() == DOMNode::ELEMENT_NODE ) // is element
	{
	  DOMElement* e
	    = dynamic_cast< xercesc::DOMElement* >( currentNode );

	  std::string sVal;
	  if(getValue(e, TAG_Name, sVal))
	    {
	      this->name = sVal;
	    }
	  else if(getValue(e, TAG_Value, sVal))
	    {
	      /* Punt..generate fP */
	      val = atof(sVal.c_str());
	      //printf("value = %s\n", sVal.c_str());
	    }
	}
    }
  return NULL;
 }

xmlNode *xmlVar::parseIt(xercesc::DOMNode *n, std::map<std::string, ForStmt*> &loopHash)
{
  DOMNodeList*      children = n->getChildNodes();
  const  XMLSize_t nodeCount = children->getLength();
  Var *vVar = 0;
  /*parse the loop header */
  for( XMLSize_t xx = 0; xx < nodeCount; ++xx )
    {
      DOMNode* currentNode = children->item(xx);
      if( currentNode->getNodeType() &&  // true is not NULL
	  currentNode->getNodeType() == DOMNode::ELEMENT_NODE ) // is element
	{
	  DOMElement* e
	    = dynamic_cast< xercesc::DOMElement* >( currentNode );

	  std::string sVal;
	  if(getValue(e, TAG_Dim, sVal))
	    {
	      int d = atoi(sVal.c_str());
	      dims.push_back(d);
	    }
	  else if(getValue(e, TAG_Name, sVal))
	    {
	      this->name = sVal;
	    }
	  else if(getValue(e, TAG_Type, sVal))
	    {
	      typeStr = sVal;
	    }
	}
    }

  bool varIsInt = parseTypeString(typeStr);
  this->isInteger = varIsInt;
  vVar = new Var(this->name,varIsInt);
  setAstNode(vVar);
 
  assert(vVar);
  vVar->setDim(dims.size());
  vVar->typeStr=typeStr;
  //xVar->name=sVal;
  return NULL;
}

xmlNode* xmlProgram::parseIt(xercesc::DOMNode *n, std::map<std::string, ForStmt*> &loopHash) 
{
  DOMNodeList*      children = n->getChildNodes();
  const  XMLSize_t nodeCount = children->getLength();
  
  assert(n->hasChildNodes());
  std::string sVal;
  this->name = "func0";

 /* Need to parse all variables*/ 
  for( XMLSize_t xx = 0; xx < nodeCount; ++xx )
    {
      DOMNode* currentNode = children->item(xx);
      if( currentNode->getNodeType() &&  // true is not NULL
	  currentNode->getNodeType() == DOMNode::ELEMENT_NODE ) // is element
	{
	  DOMElement* currentElement
	    = dynamic_cast< xercesc::DOMElement* >( currentNode );
	  
	  if(getValue(currentElement, TAG_Name, sVal))
	    {
	      this->name = sVal;
	    }

	  if(XMLString::equals(currentElement->getTagName(), TAG_Var))
	    {
	      xmlVar *xVar = new xmlVar();
	      xVar->parent=this;
	      xVar->root=root;
	      varList.push_back(xVar);
	      xVar->parseIt(currentElement,loopHash);	   
	    }
	}
    }

  for( XMLSize_t xx = 0; xx < nodeCount; ++xx )
    {
      DOMNode* currentNode = children->item(xx);
      if( currentNode->getNodeType() &&  // true is not NULL
	  currentNode->getNodeType() == DOMNode::ELEMENT_NODE ) // is element
	{
	  DOMElement* e
	    = dynamic_cast< xercesc::DOMElement* >( currentNode );
	  if( XMLString::equals(e->getTagName(), TAG_ForStmt))
	    {
	      if(dbgParsePrint) dbgPrintf("got ForStmt\n");
	      xmlForStmt *xmlFor = new xmlForStmt();
	      xmlFor->parent=this;
	      xmlFor->root=root;
	      forList.push_back(xmlFor);
	      xmlFor->parseIt(e,loopHash);
	      astNode = xmlFor->getASTNode();
	    }
	}
    }
  assert(forList.size() == 1);
  assert(astNode);
  
  /*  now handle top level loop */
  return this;
}

xmlNode* xmlForStmt::findNode(std::string nodeName)
{
  assert(parent!=NULL);
  /* check induction variables */
  if(header->inductionVar.compare(nodeName)==0)
    {
      //printf("found induction variable\n");
      return this;
    }
  return parent->findNode(nodeName);
}

xmlNode* xmlForStmt::parseIt(xercesc::DOMNode *n, std::map<std::string, ForStmt*> &loopHash) 
{
  DOMNodeList*      children = n->getChildNodes();
  const  XMLSize_t nodeCount = children->getLength();
  ForStmt *thisFor=0;

  /*parse the loop header */
  for( XMLSize_t xx = 0; xx < nodeCount; ++xx )
    {
      DOMNode* currentNode = children->item(xx);
      if( currentNode->getNodeType() &&  // true is not NULL
	  currentNode->getNodeType() == DOMNode::ELEMENT_NODE ) // is element
	{
	  DOMElement* e
	    = dynamic_cast< xercesc::DOMElement* >( currentNode );
	  if( XMLString::equals(e->getTagName(), TAG_LoopHeader))
	    {
	      if(dbgParsePrint) dbgPrintf("got LoopHeader\n");
	      header = new xmlLoopHeader();
	      header->parent=this;
	      header->root = root;
	      header->parseIt(e,loopHash);
	      Var *vUB = 0;
	      if(header->dynUBound)
		{
		  vUB = dynamic_cast<Var*>(header->dynUBound->astNode);
		}
	      
	      if(header->isTriLBound)
		{
		  header->start = INT_MIN;
		}

	      thisFor = new ForStmt(header->start,
				    header->stop,
				    1, //implicit step of 1
				    header->isDynamicBound,
				    vUB,
				    header->isTriLBound, 
				    header->triLBound, /* triLBound */
				    header->triOffset,
				    header->inductionVar);
	      
	      loopHash[header->inductionVar] = thisFor;
	      astNode = thisFor;
	    }
	  else if(XMLString::equals(e->getTagName(), TAG_LoopBody))
	    {
	      if(dbgParsePrint) dbgPrintf("got LoopBody\n");
	      body = new xmlLoopBody();
	      body->parent = this;
	      body->root = root;
	      body->parseIt(e,loopHash);
	    }
	}
    }
  assert(thisFor);
  assert(body);
  
  for(std::list<Node*>::iterator it =
	body->astBodyList.begin();
      it != body->astBodyList.end();
      it++)
    {
      Expr *e = dynamic_cast<Expr*>(*it);
      assert(e);
      thisFor->addBodyStmt(e);
    }

  return this;
}

void parseTriOffset(const char *p, std::string &inVar, int &offset)
{
  int i0 = 0,i1,i2;
  std::string temp;

  while( p[i0] != '(')
    {
      i0++;
    }
  i0++;
  i1 = i0;
  while( p[i1] != ',')
    {
      i1++;
    }
  for(int i = i0; i < i1; i++)
    {
      char c[2];
      c[0] = p[i];
      c[1] = '\0';
      inVar += std::string(c);
    }
  i2 = i1+1;
  while( p[i2] != ')')
    {
      i2++;
    }
  for(int i = (i1+1); i< i2; i++)
    {
      char c[2];
      c[0] = p[i];
      c[1] = '\0';
      temp += std::string(c);
    }
  offset = atoi(temp.c_str());
} 


xmlNode *xmlLoopHeader::parseIt(xercesc::DOMNode *n, std::map<std::string, ForStmt*> &loopHash)
{
  DOMNodeList*      children = n->getChildNodes();
  const  XMLSize_t nodeCount = children->getLength();
  
  this->isDynamicBound = false;
  this->dynUBound = 0;
  this->isTriLBound = false;
  this->triLBound = 0;
  this->start = -1;
  for( XMLSize_t xx = 0; xx < nodeCount; ++xx )
    {
      DOMNode* currentNode = children->item(xx);
      if( currentNode->getNodeType() &&  // true is not NULL
	  currentNode->getNodeType() == DOMNode::ELEMENT_NODE ) // is element
	{
	  DOMElement* e
	    = dynamic_cast< xercesc::DOMElement* >( currentNode );

	  std::string sVal;
	  if(getValue(e, TAG_Induction, sVal))
	    {
	      inductionVar = sVal;
	      if(dbgParsePrint)
		{
		  dbgPrintf("found inductionVar=%s\n", 
			 inductionVar.c_str());
		}
	    }
	  else if(getValue(e, TAG_Start, sVal))
	    {
	      const char *czStart = sVal.c_str();
	      bool gotSUB = (strncmp("SUB",czStart,3)==0);
	      bool gotADD = (strncmp("ADD",czStart,3)==0);
	      
	      if(gotSUB || gotADD)
		{
		  std::string otherLoop; 
		  int otherOffset;
		  parseTriOffset(czStart, otherLoop, otherOffset);
		  this->isTriLBound = true;
		  this->triLBound = loopHash[otherLoop];
		  this->triOffset = gotSUB ? -otherOffset : otherOffset;
		  if(this->triLBound==0)
		    {
		      dbgPrintf("couldn't find loop...\n");
		    }
		}
	      else
		{
		  start = atoi(sVal.c_str());
		}
	      dbgPrintf("start loop @ %d\n", start);
	    }
	  else if(getValue(e, TAG_Stop, sVal))
	    {
	      //integer bound
	      if(isInteger(sVal.c_str()))
		{
		  stop = atoi(sVal.c_str());
		  if(dbgParsePrint) dbgPrintf("stop loop @ %d\n", stop);
		}
	      else
		{
		  isDynamicBound = true;
		  stop = INT_MAX;
		  xmlNode *nn = parent->findNode(sVal.c_str());
		  assert(nn!=NULL);
		  xmlVar *b = dynamic_cast<xmlVar*>(nn);
		  assert(b != NULL);
		  dynUBound = b;
		  /*
		  dbgPrintf("name = %s\n", b->name.c_str());
		  dbgPrintf("type=%s\n",
			 b->typeStr.c_str());
		  exit(-1);
		  */
		}
	      
	    }
	}
    }

  return this;
}

xmlNode *xmlLoopBody::findNode(std::string nodeName)
{
  /* TODO: Make sure scoping is correct */
  for(std::list<xmlNode*>::iterator it=bodyList.begin();
      it != bodyList.end(); it++)
    {
      xmlNode *n = *it;
      if(n->name.compare(nodeName)==0)
	{
	  if(dbgParsePrint)
	    dbgPrintf("found %s\n", n->name.c_str());
	  return n;
	}
    }
  assert(parent!=NULL);
  return parent->findNode(nodeName);
}

xmlNode *xmlLoopBody::parseIt(xercesc::DOMNode *n, std::map<std::string, ForStmt*> &loopHash)
{
  /* TODO: THIS ASSUMES PERFECTLY NESTED LOOPS */
  DOMNodeList*      children = n->getChildNodes();
  const  XMLSize_t nodeCount = children->getLength();

  for( XMLSize_t xx = 0; xx < nodeCount; ++xx )
    {
      DOMNode* currentNode = children->item(xx);
      if( currentNode->getNodeType() &&  // true is not NULL
	  currentNode->getNodeType() == DOMNode::ELEMENT_NODE ) // is element
	{
	  DOMElement* e
	    = dynamic_cast< xercesc::DOMElement* >( currentNode );
	  /* Another nested loop */
	  if( XMLString::equals(e->getTagName(), TAG_ForStmt))
	    {
	      if(dbgParsePrint) dbgPrintf("got nested loop\n");
	      xmlForStmt *nestedFor = new xmlForStmt();
	      nestedFor->parent=this;
	      nestedFor->root = root;
	      nestedFor->parseIt(e,loopHash);
	      Node *nestedASTFor = 
		nestedFor->getASTNode();
	      assert(nestedASTFor);
	      astBodyList.push_back(nestedASTFor);
	      bodyList.push_back(nestedFor);
	    }
	  else if(XMLString::equals(e->getTagName(), TAG_Constant) ||
		  XMLString::equals(e->getTagName(), TAG_IntConstant) )
	    {
	   
	      xmlConstant *myConstant = new xmlConstant();
	      myConstant->parent = this;
	      myConstant->root = root;
	      myConstant->parseIt(e,loopHash);
	      if(XMLString::equals(e->getTagName(), TAG_Constant)) {
		ConstantFloatVar *cVar = new ConstantFloatVar(myConstant->val);
		myConstant->astNode = cVar;
	      } else {
		ConstantIntVar *cVar = new ConstantIntVar((int)myConstant->val);
		myConstant->astNode = cVar;
	      }
	      bodyList.push_back(myConstant);
	    }
	  else if( XMLString::equals(e->getTagName(), TAG_MemRef))
	    {
	      if(dbgParsePrint) dbgPrintf("got a MemRef\n");
	      xmlMemRef *memReference = new xmlMemRef();
	      memReference->parent = this;
	      memReference->root = root;
	      memReference->parseIt(e,loopHash);
	      Node *nestedMemRef =
		memReference->getASTNode();
	      assert(nestedMemRef);
	      //astBodyList.push_back(nestedMemRef);
	      bodyList.push_back(memReference);
	    }
	  else if(XMLString::equals(e->getTagName(), TAG_CompoundExpr))
	    {
	      if(dbgParsePrint) dbgPrintf("got CompoundExpr\n");
	      xmlCompoundExpr *aCompoundExpr = new xmlCompoundExpr();
	      aCompoundExpr->parent = this;
	      aCompoundExpr->root = root;
	      aCompoundExpr->parseIt(e,loopHash);
	      Node *nestedCompound =
		aCompoundExpr->getASTNode();
	      assert(nestedCompound);
	      //astBodyList.push_back(nestedCompound);
	      bodyList.push_back(aCompoundExpr);
	    }
	  else if(XMLString::equals(e->getTagName(), TAG_SelectExpr))
	    {
	      if(dbgParsePrint) dbgPrintf("got SelectExpr\n");
	      xmlSelectExpr* aSelectExpr = new xmlSelectExpr();
	      aSelectExpr->parent = this;
	      aSelectExpr->root = root;
	      aSelectExpr->parseIt(e,loopHash);
	      Node *nestedSelect = 
		aSelectExpr->getASTNode();
	      assert(nestedSelect);
	      bodyList.push_back(aSelectExpr);
	    }
	  else if(XMLString::equals(e->getTagName(), TAG_AssignExpr))
	    {
	      if(dbgParsePrint) dbgPrintf("got AssignExpr\n");
	      xmlAssignExpr *anAssignExpr = new xmlAssignExpr();
	      anAssignExpr->parent = this;
	      anAssignExpr->root = root;
	      anAssignExpr->parseIt(e,loopHash);
	      Node *nestedAssign =
		anAssignExpr->getASTNode();
	      assert(nestedAssign);
	      astBodyList.push_back(nestedAssign);
	      bodyList.push_back(anAssignExpr);
	    }
	  else if(XMLString::equals(e->getTagName(), TAG_UnaryIntrin))
	    {
	      xmlUnaryIntrin* aUnaryIntrin = new xmlUnaryIntrin();
	      aUnaryIntrin->parent = this;
	      aUnaryIntrin->root = root;
	      aUnaryIntrin->parseIt(e,loopHash);
	      Node *nestedSelect = 
		aUnaryIntrin->getASTNode();
	      assert(nestedSelect);
	      bodyList.push_back(aUnaryIntrin);
	    }
	}
    }
  return this;
}

xmlNode *xmlMemRef::findNode(std::string nodeName)
{
  assert(parent!=NULL);
  return parent->findNode(nodeName);
}

xmlNode *xmlMemRef::parseIt(xercesc::DOMNode *n, std::map<std::string, ForStmt*> &loopHash)
{
  DOMNodeList*      children = n->getChildNodes();
  const  XMLSize_t nodeCount = children->getLength();
  xmlNode *bVar=0;

  for( XMLSize_t xx = 0; xx < nodeCount; ++xx )
    {
     DOMNode* currentNode = children->item(xx);
     if( currentNode->getNodeType() &&  // true is not NULL
	  currentNode->getNodeType() == DOMNode::ELEMENT_NODE ) // is element
	{
	  DOMElement* e
	    = dynamic_cast< xercesc::DOMElement* >( currentNode );
	  std::string nstr;
	  
	  if(getValue(e, TAG_Name, nstr))
	    {
	      this->name = nstr;
	      if(dbgParsePrint) dbgPrintf("memRef name = %s\n", name.c_str());
	    }
	  else if(getValue(e, TAG_Var, nstr))
	    {
	      this->baseVar = nstr;
	      bVar = findNode(this->baseVar);
	      if(bVar==0)
		{
		  dbgPrintf("couldn't find %s\n", baseVar.c_str());
		  exit(-1);
		}
	      if(dbgParsePrint) 
		dbgPrintf("baseVar name = %s\n", baseVar.c_str());
	    }
	  else if( XMLString::equals(e->getTagName(), TAG_Subscript))
	    {
	      xmlSubscript *aSubscript = new xmlSubscript();
	      aSubscript->parent = this;
	      aSubscript->root = root;
	      if(dbgParsePrint) dbgPrintf("got []\n");
	      aSubscript->parseIt(e,loopHash);
	      subList.push_back(aSubscript);
	    }
	}
    }
  if(bVar==0)
    {
      dbgPrintf("bVar equals NULL!\n");
      exit(-1);
    }
  Var *vASTVar = dynamic_cast<Var*>(bVar->getASTNode());
  assert(vASTVar);
  assert(root);
  MemRef *mR = new MemRef(vASTVar);
  root->MemRefMap[this->name] = mR;
  astNode = mR;

  for(std::list<xmlSubscript*>::iterator it=subList.begin();
      it != subList.end(); it++)
    {
      astSubscript *s = (*it)->astSubs;
      if(s->isIndirect) {
	  mR->isIndirect = true;
	}
      assert(s);
      mR->addSubscript(s);
    }
  

  return this;
}

xmlNode *xmlSubscript::findNode(std::string nodeName)
{
  assert(parent);
  return parent->findNode(nodeName);
}

xmlNode *xmlSubscript::parseIt(xercesc::DOMNode *n, std::map<std::string, ForStmt*> &loopHash)
{
  DOMNodeList*      children = n->getChildNodes();
  const  XMLSize_t nodeCount = children->getLength();
 
  bool gotIndirectRef = false;
  MemRef *mR = 0;

  bool gotVar=false;
  bool gotCoeff=false;
  std::string tIndVar;
  int tIndCoeff=INT_MIN;

  /*TODO: expand representation to support 
   * constructs like X[I+J] */

  for( XMLSize_t xx = 0; xx < nodeCount; ++xx )
    {
     DOMNode* currentNode = children->item(xx);
     if( currentNode->getNodeType() &&  // true is not NULL
	 currentNode->getNodeType() == DOMNode::ELEMENT_NODE ) // is element
       {
	 DOMElement* e
	    = dynamic_cast< xercesc::DOMElement* >( currentNode );
	 std::string nstr;

	 /* parse subscript with multiple induction variables */
	 if( XMLString::equals(e->getTagName(), TAG_MIVInd))
	    {
	      DOMNodeList* MIVs = currentNode->getChildNodes();
	      const XMLSize_t MIVcount = MIVs->getLength();

	      for(XMLSize_t xxx=0; xxx < MIVcount; ++xxx)
		{
		  DOMNode *mivNode = MIVs->item(xxx);
		  if( mivNode->getNodeType() &&  
		      mivNode->getNodeType() == DOMNode::ELEMENT_NODE )
		    {
		      DOMElement* ee
			= dynamic_cast< xercesc::DOMElement* >(mivNode );
		      std::string nnstr;
		      
		      if(getValue(ee, TAG_IndVar, nnstr))
			{
			  tIndVar = nnstr;
			  gotVar=true;
			  
			  if(gotVar && gotCoeff)
			    {
			      gotVar=false;
			      gotCoeff=false;
			      std::pair<xmlNode*, int> p(findNode(tIndVar), tIndCoeff);
			      indNodes.push_back(p);
			    }
			}
		      else if(getValue(ee, TAG_IndCoeff, nnstr))
			{
			  tIndCoeff = atoi(nnstr.c_str());
			  gotCoeff = true;
			  
			  if(gotVar && gotCoeff)
			    {
			      gotVar=false;
			      gotCoeff=false;
			      std::pair<xmlNode*, int> p(findNode(tIndVar), tIndCoeff);
			      indNodes.push_back(p);
			    }
			}
		    }
		}
	    }
	 else if(getValue(e, TAG_IndVar, nstr))
	   {
	     tIndVar = nstr;
	     gotVar=true;

	     if(gotVar && gotCoeff)
	       {
		 gotVar=false;
		 gotCoeff=false;
		 std::pair<xmlNode*, int> p(findNode(tIndVar), tIndCoeff);
		 indNodes.push_back(p);
	       }
	     //if(dbgParsePrint) dbgPrintf("Ind=%s\n", IndVar.c_str());
	     //indNodes.push_back(findNode(IndVar));
	   }
	 else if(getValue(e, TAG_IndCoeff, nstr))
	   {
	     tIndCoeff = atoi(nstr.c_str());
	     gotCoeff = true;

	     if(gotVar && gotCoeff)
	       {
		 gotVar=false;
		 gotCoeff=false;
		 std::pair<xmlNode*, int> p(findNode(tIndVar), tIndCoeff);
		 indNodes.push_back(p);
	       }
	     //IndCoeffs.push_back(atoi(nstr.c_str());
	     //if(dbgParsePrint) dbgPrintf("Coeff=%d\n", IndCoeff);
	   }
	 else if(getValue(e, TAG_Offset, nstr))
	   {
	     Offset = atoi(nstr.c_str());
	     if(dbgParsePrint) dbgPrintf("Ind=%d\n",Offset);
	   }

	 else if(getValue(e, TAG_Indirect, nstr))
	   {
	     mR = root->MemRefMap[nstr];
	     //dbgPrintf("MemRef = %s\n", nstr.c_str());
	     gotIndirectRef = true;
	   }
	}
    }
  
  if(gotIndirectRef)
    {
      assert(mR);
      astSubs = new astSubscript(mR);
      /* okay, we need to get the induction variable
       * iterating the indirect access, we're going t
       * to make an assumption of a transitive relationship
       */
      //iterate over all subscripts \in indirect statement
      for(vector<astSubscript*>::iterator it = mR->subscripts.begin();
	  it != mR->subscripts.end(); it++)
	{
	  for(map<ForStmt*, int>::iterator mit = (*it)->coeffs.begin(); 
	      mit != (*it)->coeffs.end(); mit++)
	    {
	      astSubs->addTerm(mit->first, mit->second);
	    }
	}

      //dbgPrintf("got indirect mem ref, bombing out\n");
      //exit(-1);
    }
  else
    {
      astSubs = new astSubscript(Offset);
      for(std::list<std::pair<xmlNode*, int> >::iterator it = indNodes.begin();
	  it != indNodes.end(); it++)
	{
	  std::pair<xmlNode*, int> p = *it;
	  xmlNode *indNode = p.first;
	  if(indNode) {
	    ForStmt *indForLoop = dynamic_cast<ForStmt*>(indNode->getASTNode());;
	    assert(indForLoop);
	    astSubs->addTerm(indForLoop, p.second);
	  }
	}
    }

  return this;
}

xmlNode *xmlAssignExpr::findNode(std::string nodeName)
{
  assert(parent!=NULL);
  return parent->findNode(nodeName);
}

xmlNode *xmlUnaryIntrin::findNode(std::string nodeName)
{
  assert(parent!=NULL);
  return parent->findNode(nodeName);
}

xmlNode *xmlCompoundExpr::findNode(std::string nodeName)
{
  assert(parent!=NULL);
  return parent->findNode(nodeName);
}

xmlNode *xmlSelectExpr::findNode(std::string nodeName)
{
  assert(parent!=NULL);
  return parent->findNode(nodeName);
}


xmlNode *xmlCompoundExpr::parseIt(xercesc::DOMNode *n, std::map<std::string, ForStmt*> &loopHash)
{
  DOMNodeList*      children = n->getChildNodes();
  const  XMLSize_t nodeCount = children->getLength();
 
  for( XMLSize_t xx = 0; xx < nodeCount; ++xx )
    {
      DOMNode* currentNode = children->item(xx);
      if( currentNode->getNodeType() &&  // true is not NULL
	  currentNode->getNodeType() == DOMNode::ELEMENT_NODE ) // is element
       {
	 DOMElement* e
	   = dynamic_cast< xercesc::DOMElement* >( currentNode );
	 std::string nstr;
	 if(getValue(e, TAG_Name, nstr))
	   {
	     name=nstr;
	     if(dbgParsePrint) dbgPrintf("compound name=%s\n", name.c_str());
	   }
	 else if(getValue(e, TAG_Op, nstr))
	   {
	     if(dbgParsePrint) dbgPrintf("op name=%s\n", nstr.c_str());
	    
	     if(nstr.compare("ADD")==0)
	       op=ADD;
	     else if(nstr.compare("SUB")==0)
	       op=SUB;
	     else if(nstr.compare("MUL")==0)
	       op=MUL;
	     else if(nstr.compare("DIV")==0)
	       op=DIV;
	     else if(nstr.compare("LT")==0)
	       op=LT;
	     else if(nstr.compare("LTE")==0)
	       op=LTE;
	     else if(nstr.compare("GT")==0)
	       op=GT;
	     else if(nstr.compare("GTE")==0)
	       op=GTE;
	     else if(nstr.compare("EQ")==0)
	       op=EQ;
	     else if(nstr.compare("NEQ")==0)
	       op=NEQ;
	     else
	       {
		 dbgPrintf("UNKNOWN OP TYPE\n");
		 exit(-1);
	       }
	   }
	 else if(getValue(e, TAG_Left, nstr))
	   {
	     left = findNode(nstr);
	   }
	 else if(getValue(e, TAG_Right, nstr))
	   {
	     right = findNode(nstr);
	   }
       }
    }

  assert(left); assert(right);
  Expr *leftAST = dynamic_cast<Expr*>(left->getASTNode());
  Expr *rightAST = dynamic_cast<Expr*>(right->getASTNode());
  assert(leftAST); assert(rightAST);
  CompoundExpr *compExpr = new CompoundExpr(op, leftAST, rightAST);
  astNode=compExpr;
  return this;
}

xmlNode *xmlSelectExpr::parseIt(xercesc::DOMNode *n, std::map<std::string, ForStmt*> &loopHash)
{
  DOMNodeList*      children = n->getChildNodes();
  const  XMLSize_t nodeCount = children->getLength();
 
  for( XMLSize_t xx = 0; xx < nodeCount; ++xx )
    {
      DOMNode* currentNode = children->item(xx);
      if( currentNode->getNodeType() &&  // true is not NULL
	  currentNode->getNodeType() == DOMNode::ELEMENT_NODE ) // is element
       {
	 DOMElement* e
	   = dynamic_cast< xercesc::DOMElement* >( currentNode );
	 std::string nstr;
	 if(getValue(e, TAG_Name, nstr))
	   {
	     name=nstr;
	     if(dbgParsePrint) dbgPrintf("select name=%s\n", name.c_str());
	   }
	 else if(getValue(e, TAG_Left, nstr))
	   {
	     left = findNode(nstr);
	   }
	 else if(getValue(e, TAG_Right, nstr))
	   {
	     right = findNode(nstr);
	   }
	 else if(getValue(e, TAG_Sel, nstr))
	   {
	     sel = findNode(nstr);
	   }
       }
    }

  assert(left); assert(right); assert(sel);
  Expr *leftAST = dynamic_cast<Expr*>(left->getASTNode());
  Expr *rightAST = dynamic_cast<Expr*>(right->getASTNode());
  Expr *selAST = dynamic_cast<Expr*>(sel->getASTNode());
  assert(leftAST); assert(rightAST); assert(selAST);
  SelectExpr *selExpr = new SelectExpr(selAST, leftAST, rightAST);
  astNode=selExpr;
  return this;
}


xmlNode *xmlAssignExpr::parseIt(xercesc::DOMNode *n, std::map<std::string, ForStmt*> &loopHash)
{
  DOMNodeList*      children = n->getChildNodes();
  const  XMLSize_t nodeCount = children->getLength();
 
  for( XMLSize_t xx = 0; xx < nodeCount; ++xx )
    {
      DOMNode* currentNode = children->item(xx);
      if( currentNode->getNodeType() &&  // true is not NULL
	  currentNode->getNodeType() == DOMNode::ELEMENT_NODE ) // is element
       {
	 DOMElement* e
	   = dynamic_cast< xercesc::DOMElement* >( currentNode );
	 std::string nstr;
	 if(getValue(e, TAG_Name, nstr))
	   {
	     name=nstr;
	     if(dbgParsePrint) dbgPrintf("assign name=%s\n", name.c_str());
	   }
	 else if(getValue(e, TAG_Left, nstr))
	   {
	     left = findNode(nstr);
	   }
	 else if(getValue(e, TAG_Right, nstr))
	   {
	     right = findNode(nstr);
	   }
       }
    }
  assert(left); assert(right);
  Expr *leftAST = dynamic_cast<Expr*>(left->getASTNode());
  Expr *rightAST = dynamic_cast<Expr*>(right->getASTNode());
  assert(leftAST); assert(rightAST);
  AssignExpr *aExpr= new AssignExpr(name, leftAST, rightAST);
  astNode=aExpr;
  return this;
}


xmlNode *xmlUnaryIntrin::parseIt(xercesc::DOMNode *n, std::map<std::string, ForStmt*> &loopHash)
{
  DOMNodeList*      children = n->getChildNodes();
  const  XMLSize_t nodeCount = children->getLength();
 
  for( XMLSize_t xx = 0; xx < nodeCount; ++xx )
    {
      DOMNode* currentNode = children->item(xx);
      if( currentNode->getNodeType() &&  // true is not NULL
	  currentNode->getNodeType() == DOMNode::ELEMENT_NODE ) // is element
       {
	 DOMElement* e
	   = dynamic_cast< xercesc::DOMElement* >( currentNode );
	 std::string nstr;
	 if(getValue(e, TAG_Name, nstr))
	   {
	     name=nstr;
	     if(dbgParsePrint) dbgPrintf("assign name=%s\n", name.c_str());
	   }	
	 else if(getValue(e, TAG_Left, nstr))
	   {
	     arg = findNode(nstr);
	   }
	 else if(getValue(e, TAG_Op, nstr))
	   {
	     intrinStr = nstr;
	   }
       }
    }

  Expr *leftAST = dynamic_cast<Expr*>(arg->getASTNode());
  assert(leftAST);
  UnaryIntrin *uniIntrin = new UnaryIntrin(leftAST, intrinStr);
  astNode=uniIntrin;
  return this;
}
