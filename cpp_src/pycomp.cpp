#include <cstdio>
#include <cstdlib>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <cstdio>
#include <cassert>
#include <unistd.h>
#include <pthread.h>
#include <unordered_map>
#include "llvm/DerivedTypes.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/system_error.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/Interpreter.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Support/TargetSelect.h"


/* headers from our project */
#include "codegen.h"
#include "importxml.h"
#include "helper.h"

extern "C" 
{
#include <python2.7/Python.h>
#include <python2.7/pyconfig.h>
#include <python2.7/numpy/arrayobject.h>
#include <python2.7/numpy/arrayscalars.h>
};
using namespace std;
using namespace xercesc;
using namespace llvm;

static map<string, pair<ExecutionEngine*, Function*> > functMap;
static map<string, pair<void*, void*> > functPtrMap;
static map<string, size_t> xmlHashes;

typedef struct
{
  int lb;
  int ub;
  std::string varLB;
  std::string varUB;
} parLoopBound_t;

//loops with outer loop parallelism
static map<string, parLoopBound_t> parIterMap;

typedef struct
{
  bool en_vec;
  bool en_omp;
  bool en_unroll;
  bool en_pe;
  bool en_shift;
  int vec_len;
  bool en_fusion;
  bool en_aligned;
} copt_t;


typedef union
{
  float *flt_ptr_arg;
  int *int_ptr_arg;
  int int_arg;
  float flt_arg;
} arg_t;


static inline void native_cpuid(unsigned int *eax, unsigned int *ebx,
                                unsigned int *ecx, unsigned int *edx)
{
  /* ecx is often an input as well as an output. */
  asm volatile("cpuid"
	       : "=a" (*eax),
		 "=b" (*ebx),
		 "=c" (*ecx),
		 "=d" (*edx)
	       : "0" (*eax), "2" (*ecx));
}

static inline bool check_avx()
{
  unsigned eax, ebx, ecx, edx;
  eax = 1; /* processor info and feature bits */
  native_cpuid(&eax, &ebx, &ecx, &edx);
  /* For what it's worth, bit 28 of ECX 
   * indicates whether the CPU has AVX */
  ecx = ecx >> 28;
  ecx = ecx & 0x1;
  return (ecx!=0);
}


const std::string unionStr = "typedef union\n {\n float *flt_ptr_arg;\n int *int_ptr_arg;\n int int_arg;\n float flt_arg;\n } arg_t;\n\n";
const std::string llvmStr = "typedef int i32;\n\n";

typedef void (*trampoline) (void *, arg_t *, int);

static std::string getFunctTypeStr(const Type *t)
{
  string ts;
  std::string tts;
  llvm::raw_string_ostream rso(tts);
  t->print(rso);
  ts = rso.str(); 
  //remove return type
  size_t lparens = ts.find('(');
  ts.erase(0, lparens);
  //size_t nline = ts.find('\n');
  //ts.erase(nline); 
  return ts;
}
std::string mkCTrampoline(llvm::Function *funct, std::string fName)
{
  std::string tramp = llvmStr + unionStr;
  tramp += "void tramp_" + fName + "(void *fp, arg_t *args, int n)\n";
  tramp += "{\n";
  tramp += "typedef void (*functPtr_" + fName + ")"; 
  tramp += getFunctTypeStr(funct->getFunctionType());
  tramp += ";\n";
  tramp += "functPtr_" + fName + " tFunct = (functPtr_" + fName + ")fp;\n"; 
  tramp += "tFunct(\n";
  size_t numArgs = funct->arg_size();
  size_t idx = 0;
  for (Function::arg_iterator AI = funct->arg_begin(); 
       AI != funct->arg_end(); ++AI)
    {
      tramp += "args[" + int2string(idx) + "].";
      const Type *argType = (*AI).getType();
     
      if(argType->isIntegerTy())
	{
	  tramp += "int_arg";
	}
      else if(argType->isFloatTy())
	{
	  tramp += "flt_arg";
	}
      else if(argType->isPointerTy())
	{
	  tramp += "flt_ptr_arg";
	}

      if(idx != (numArgs-1))
	{
	  tramp += ",\n";
	}
      idx++;
      
    }
  tramp += "\n);\n";
  tramp += "}\n";
  return tramp;
}

bool clangIt(string fileName)
{
  const string clang("clang ");
  const string opts(".c -emit-llvm -O3 -c");
  string cmd = clang + fileName + opts;
  int rc = system(cmd.c_str());
  return (rc==0);
}

void *generateClangTrampoline(llvm::Function *funct, std::string fName)
{  
  std::string strErr;
  std::string tempName = "tramp_" + fName;
  std::string codeStr = mkCTrampoline(funct, fName);
  std::string trampC = tempName + ".c";
  FILE *fp = fopen(trampC.c_str(), "w");
  assert(fp);
  fprintf(fp, "%s\n", codeStr.c_str());
  fclose(fp);
  
  bool rc = clangIt(tempName);
  if(rc==false)
    {
      printf("clang failed\n");
      exit(-1);
    }
  llvm::LLVMContext &Context = llvm::getGlobalContext();
  MemoryBuffer *buff;
  OwningPtr<llvm::MemoryBuffer> fileBuf;
  if(MemoryBuffer::getFile(string(tempName) + ".o", fileBuf))
    {
      printf("getFile failed :(\n");
      exit(0);
    }
  MemoryBuffer *MB = fileBuf.take();
  Module* m = ParseBitcodeFile(MB, Context, &strErr);
  printf("module = %p\n", m);
  ExecutionEngine* ee = ExecutionEngine::create(m);
  printf("executionengine = %p\n", ee);
  Function *trampFunct = ee->FindFunctionNamed(tempName.c_str());
  printf("trampFunct = %p\n", trampFunct);
  return ee->getPointerToFunction(trampFunct);
}

void *generateLLVMTrampoline(llvm::Function *funct, std::string fName)
{  
  std::string tempName = "tramp_" + fName;
  std::string modName = "tramp_" + fName + "_module";
  

  llvm::LLVMContext &Context = llvm::getGlobalContext();
   llvm::IRBuilder<> *trampBuilder = 
     new llvm::IRBuilder<>(Context);

  llvm::Module *trampModule = new llvm::Module(modName, Context);

  vector<Type*> trampArgs(3);
  vector<string> argNames(3);
  map<string, Value*> argValueMap;

  //void * fp
  trampArgs[0] = Type::getInt8PtrTy(getGlobalContext());
  argNames[0] = std::string("fp");
  //arg_t args
  /* DBS: this is a hack for unions */
  Type *bType = Type::getFloatPtrTy(getGlobalContext());
  trampArgs[1] = bType->getPointerTo();

  argNames[1] = std::string("args");
  //int n
  trampArgs[2] = Type::getInt32Ty(getGlobalContext());
  argNames[2] = std::string("n");
  //valueMap[std::string("n")] = trampArgs[2];
  
  ArrayRef<Type*> arFunctArgs(trampArgs);
  FunctionType *FT = llvm::FunctionType::get
    (Type::getVoidTy(getGlobalContext()),
     arFunctArgs,
     false);
  llvm::Function *trampFunct = 
    Function::Create(FT, llvm::Function::ExternalLinkage, 
		     tempName, trampModule);
 
  size_t Idx=0;
  for (Function::arg_iterator AI = trampFunct->arg_begin(); 
       AI != trampFunct->arg_end(); ++AI, ++Idx) 
    {
      AI->setName(argNames[Idx]);
      argValueMap[argNames[Idx]] = AI;
      
    }    
  llvm::Value *fp = argValueMap["fp"];
  llvm::Value *args =  argValueMap["args"];
 
  std::string topBBName = fName +"entry";
  BasicBlock *topBB = BasicBlock::Create(Context, topBBName, trampFunct);
  trampBuilder->SetInsertPoint(topBB);
  
  /* DBS: cast to proper function pointer */
  llvm::FunctionType *functType = funct->getFunctionType();
  //functType->dump();

  Value *fpCast  = trampBuilder->CreateBitCast(fp, functType->getPointerTo());


  size_t idx = 0;
  std::vector<Value *> fpArgs(funct->arg_size());
  for (Function::arg_iterator AI = funct->arg_begin(); 
       AI != funct->arg_end(); ++AI)
    {
      Type *argType = (*AI).getType();  
      Value *offs = ConstantInt::get(Type::getInt32Ty(getGlobalContext()),idx);
      Value *gep = trampBuilder->CreateGEP(args, offs);
      Value *l = 0;

      if(argType->isIntegerTy() || argType->isFloatTy())
	{	 
	  std::string loadName = "scalar_load_" + int2string(idx);
	  Value *s = trampBuilder->CreateLoad(gep,loadName); 
	  l = trampBuilder->CreatePointerCast(s, argType);
	  
	  //tramp += "flt_arg";
	}
      else if(argType->isPointerTy())
	{
	  PointerType *pType = dyn_cast<PointerType>(argType);
	  Type *bType = pType->getElementType();
	                       
	  std::string loadName = "load_" + int2string(idx);
	  l = trampBuilder->CreateLoad(gep,loadName); 
	  if(bType->isIntegerTy())
	    {
	      l = trampBuilder->CreatePointerCast(l, argType);
	    }
	      
	}

      fpArgs[idx] = l;
      //gep = 

      idx++;
    }
  ArrayRef<Value*> argArrRef(fpArgs);

  trampBuilder->CreateCall(fpCast, argArrRef);

  trampBuilder->CreateRetVoid();
  //trampModule->dump();
  verifyModule(*trampModule, AbortProcessAction);  

  ExecutionEngine* ee = ExecutionEngine::create(trampModule);
  return ee->getPointerToFunction(trampFunct);
  //return NULL;
}


void testFunct(void *fp, arg_t *args, int n);

void testFunct(void *fp, arg_t *args, int n)
{
  typedef void (*jit_MM) (int, int, 
			  float *, int,
			  float *, int,
			  float *, int);
  jit_MM mm = (jit_MM)fp;
  mm(args[0].int_arg,
     args[1].int_arg,
     args[2].flt_ptr_arg,
     args[3].int_arg,
     args[4].flt_ptr_arg,
     args[5].int_arg,
     args[6].flt_ptr_arg,
     args[7].int_arg);
}

typedef struct
{
  void *f;
  void *t; 
  arg_t *args;
  int n;
} worker_t;

void* launcher(void *x)
{
  worker_t *w = (worker_t*)x;
  trampoline t = (trampoline)(w->t);
  t(w->f, w->args, w->n);
}


static int runCompiler (std::string &functName, std::string &myxml, 
			copt_t *opts, size_t h);

static bool is_pow2(unsigned int n)
{
  return (((n-1)&n)==0);
}

static copt_t opts;





int getNumpyIntScalar(PyObject *cObject)
{
  PyInt32ScalarObject *pyScalar = (PyInt32ScalarObject*)cObject;
  int v = pyScalar->obval;
  return v;
}


void *getNumpyArray(PyObject *cObject, std::vector<int> &strides, 
		    size_t *len=NULL)
{
  PyArrayObject *pyArray = (PyArrayObject*)cObject;
  int tLen = pyArray->nd;
 
  for(int i = 0; i < (tLen); i++)
    {
      strides.push_back(pyArray->dimensions[tLen-i-1]);
    }

  for(int i = (tLen-1); i > 0; i--)
    {
      int x = strides[i];
      for(int j = (i-1); j >= 0; j--)
	{
	  x *= strides[j];
	}
      strides[i] = x;
    }

  if(pyArray==NULL)
    return NULL;
  if(len != NULL)
    *len = (size_t)pyArray->dimensions[0];
  return pyArray->data;
}



static PyObject* runIt(PyObject *self, PyObject *args)
{
  PyObject *obj = NULL;
  std::string functName; 
  ExecutionEngine* ee = NULL;
  llvm::Function *myFunct = NULL;  
  Py_ssize_t argc = PyTuple_Size(args);
  std::vector<PyObject*> pyArgs(argc);
  //printf("num args = %d\n", (int)argc);

  if(argc < 1)
    return Py_BuildValue("i", -1);
  
  obj = PyTuple_GetItem(args, 0);
  if(PyString_Check(obj))
    functName = std::string(PyString_AsString(obj));
    
  if(functMap.find(functName)==functMap.end())
    return Py_BuildValue("i", -1);
  
  for(int i=1; i < (int)argc; i++)
    {
      obj = PyTuple_GetItem(args, i);
      if(!obj)
	{
	  printf("argument %d is NULL\n",i);
	  exit(-1);
	}
      pyArgs[(i-1)] = obj;
      
    }

  pair<ExecutionEngine*, Function*> p =  
    functMap[functName];
  pair<void*, void*> pp = 
    functPtrMap[functName];
  
  ee = p.first;
  myFunct = p.second;
  size_t Idx=0;
  size_t pyArgIdx=0;

  int dimCnt = -1;
  int dimPos = 0;
  std::vector<int> dV;

  bool outerPar = false;
  int lb=-1,ub=-1;
  std::string varLB, varUB;
  int numArgs = myFunct->arg_size();
  arg_t *argArray = new arg_t[myFunct->arg_size()];

  if(parIterMap.find(functName)!=parIterMap.end())
    {
      outerPar = true;     
      int nprocs = sysconf(_SC_NPROCESSORS_ONLN);
      parLoopBound_t pB = parIterMap[functName];
      lb = pB.lb;
      ub = pB.ub;
      varLB = pB.varLB;
      varUB = pB.varUB;
    }

  size_t aIdx = outerPar ? 2 : 0;
  for (Function::arg_iterator AI = myFunct->arg_begin(); 
       AI != myFunct->arg_end(); ++AI, ++Idx) 
    {
      if(outerPar && (Idx < 2))
	{
	  continue;
	}
      
      GenericValue gV;
      const Type *argType = (*AI).getType();
      string ts;

      std::string tts;
      llvm::raw_string_ostream rso(tts);
      argType->print(rso);
      ts = rso.str(); 

      string n = AI->getName();
      if(ts == string("float*"))
	{
	  dimPos = 0;
	  dV.clear();
	  void *ptr = getNumpyArray(pyArgs[pyArgIdx],dV,NULL);
	  if(dV.size() > 1)
	    {
	      dimCnt = dV.size()-1;
	    }
	  pyArgIdx++;
	  argArray[aIdx++].flt_ptr_arg = static_cast<float*>(ptr);
	}
      else if(ts == string("i32*"))
	{
	  dimPos = 0;
	  dV.clear();
	  void *ptr = getNumpyArray(pyArgs[pyArgIdx],dV,NULL);
	  if(dV.size() > 1)
	    {
	      dimCnt = dV.size()-1;
	    }	
	  pyArgIdx++;
	  gV.PointerVal = static_cast<int*>(ptr);
	  argArray[aIdx++].int_ptr_arg = static_cast<int*>(ptr);
	}
      else if(ts == string("i32"))
	{
	  int val = -1;
	  if(dimCnt != -1)
	    {
	      val = dV[dimPos];
	      dimPos++;
	      if(dimPos == dimCnt)
		{
		  dimCnt = -1;
		}
	    }
	  else
	    {
	      val = getNumpyIntScalar(pyArgs[pyArgIdx]);
	      if(n == varUB)
		{
		  ub = val;
		}
	      if(n == varLB)
		{
		  lb = val;
		}
	      pyArgIdx++;
	    }
	  gV.IntVal = APInt(32, val);
	  argArray[aIdx++].int_arg = val;
	}
      else
	{
	  printf("variable %s: ts=%s is unknown\n",
		 n.c_str(), ts.c_str());
	  exit(-1);
	}
    }

  
  if(outerPar)
    {      
      int nthr = sysconf(_SC_NPROCESSORS_ONLN);
      int workLen = ub-lb;
      int slice = workLen / nthr;
      
      pthread_t *thr = new pthread_t[nthr];
      worker_t *work = new worker_t[nthr];
      arg_t **thrArgArray = new arg_t*[nthr];

      for(int i = 0; i < nthr; i++)
	{
	  thrArgArray[i] = new arg_t[numArgs];
	  memcpy(thrArgArray[i], argArray, sizeof(arg_t)*numArgs);
	}

      for(int i=0; i < nthr; i++)
	{
	  int start = lb + slice*i;
	  int stop = lb + slice*(i+1);
	  if(stop > ub)
	    stop = ub;
	  
	  thrArgArray[i][0].int_arg = start;
	  thrArgArray[i][1].int_arg = stop;

	  work[i].f = pp.second;
	  work[i].t = pp.first;
	  work[i].args = thrArgArray[i];
	  work[i].n = numArgs;
	}

      for(int i = 0; i < nthr; i++)
	{
	  pthread_create(thr+i,NULL,launcher,work+i);
	}
      for(int i = 0; i < nthr; i++)
	{
	  pthread_join(thr[i],NULL);
	}

      for(int i = 0; i < nthr; i++)
	{
	  delete [] thrArgArray[i];
	}
      delete [] thrArgArray;
      
      delete [] thr;
      delete [] work;

    }
  else
    {
      trampoline tt = (trampoline)pp.first;
      tt(pp.second, argArray, numArgs);
    }

  delete [] argArray;
  return Py_BuildValue("i", 0);
}





static PyObject* compIt(PyObject *self, PyObject *args, PyObject *kwargs)
{
  std::string nameCode[2];
  /* get the number of arguments */
  Py_ssize_t argc = PyTuple_Size(args);
  std::vector<PyObject*> vArgs(argc);
  //printf("called with %d args\n", (int)argc);
  if(argc!=2)
    Py_BuildValue("i", -1);
  
  for(int i = 0; i < 2; i++)
    {
      PyObject *obj = PyTuple_GetItem(args, i);
      if(PyString_Check(obj))
	{
	  nameCode[i] = std::string(PyString_AsString(obj));
	}
    }
  
  int rc = 0;
  /* only run compiler if we haven't seen
   * the function before */
  size_t h = 0;  
  std::hash<std::string> hasher;
  h = hasher(nameCode[1]);
  
  if(functMap.find(nameCode[0])==functMap.end())
    {
      rc = runCompiler (nameCode[0],nameCode[1], &opts, h);
    }
  else if(xmlHashes[nameCode[0]] != h)
    {
      rc = runCompiler (nameCode[0],nameCode[1], &opts, h);
    }
  return Py_BuildValue("i", rc);
}


static PyObject* setOpts(PyObject *self, PyObject *args)
{ 
  printf("setOpts called\n");
  PyObject *obj = NULL;
  Py_ssize_t argc = PyTuple_Size(args);
  int o[5] = {0};
  if(argc==6)
    {
      for(int i=0; i < 6; i++)
	{
	  obj = PyTuple_GetItem(args, i);
	  if(PyBool_Check(obj))
	    {
	      o[i] = (obj == Py_True); 
	    }
	  else if(PyInt_Check(obj))
	    {
	      o[i] = (int)PyInt_AsLong(obj);
	    }
	  else
	    {
	      printf("setOpts:unknown type\n");
	    }
	}
    }

  opts.en_vec = (bool)o[0];
  opts.en_shift = (bool)o[1];
  opts.en_fusion = (bool)o[2];
  opts.en_aligned = (bool)o[3];
  opts.en_pe = (bool)o[4];
  opts.vec_len  = o[5];

  if(opts.vec_len < 1 || !is_pow2((unsigned int)opts.vec_len))
    {
      printf("setOpts: %d is an invalid vector length\n", opts.vec_len);
      exit(-1);
    }
  return Py_BuildValue("i", 0);
}


static char pycomp_doc[] = 
"func(a, b)\n\
\n\
Return the sum of a and b.";

static PyMethodDef pycomp_methods[] = {
  {"func", (PyCFunction)compIt, METH_VARARGS | METH_KEYWORDS, pycomp_doc},
  {"run",runIt, METH_VARARGS, pycomp_doc},
  {"setOpts", setOpts, METH_VARARGS, pycomp_doc},
  {NULL, NULL}
};

PyMODINIT_FUNC
initpycomp(void)
{ 
  InitializeNativeTarget();
  InitializeAllTargetMCs(); 
  InitializeNativeTargetAsmPrinter(); 
  llvm_start_multithreaded(); 
  memset(&opts,0,sizeof(copt_t));
  opts.en_vec = true;
  opts.en_shift = true;
  opts.en_fusion = true;
  opts.vec_len = 4;
  /* disable printfs */
  setEnPrintf(false);
  Py_InitModule3("pycomp", pycomp_methods, pycomp_doc);
}

static int runCompiler (std::string &functName, std::string &myxml, 
			copt_t *opts, size_t h)
{
  int c;
  xmlProgram *p;
  
  try 
    {
      XMLPlatformUtils::Initialize();
    }
  catch (const XMLException& toCatch) 
    {
      char* message = XMLString::transcode(toCatch.getMessage());
      cout << "Error during initialization! :\n"
	   << message << "\n";
      XMLString::release(&message);
      return -1;
    }
  
  XercesDOMParser* parser = new XercesDOMParser();
  parser->setValidationScheme(XercesDOMParser::Val_Always);
  parser->setDoNamespaces(true);    // optional
  
  ErrorHandler* errHandler = (ErrorHandler*) new HandlerBase();
  parser->setErrorHandler(errHandler);
  llvm::Function *llFunct = 0;
  llvm::Module *llvmModule = 0;
  try 
    {
      MemBufInputSource myxml_buf((const XMLByte*)myxml.c_str(), 
				  myxml.size(), 
				  "myxml (in memory)");

      parser->parse(myxml_buf);
      DOMDocument *xmlDoc = parser->getDocument();

      DOMElement *elementRoot = xmlDoc->getDocumentElement();
      if(!elementRoot) {throw(std::runtime_error("empty XML doc"));}
      std::map<std::string, ForStmt*> loopHash;
      p = new xmlProgram(opts->en_vec, 
			 opts->en_omp, 
			 opts->en_unroll, 
			 opts->en_pe, 
			 opts->en_shift, 
			 256, 
			 opts->vec_len, 
			 opts->en_fusion,
			 opts->en_aligned);
      p->root = p;
      p->parseIt(elementRoot, loopHash);
      p->forStmtHash = loopHash;
      p->printProgram();
      bool perfectNest;
      llFunct = p->runCodeGen(&llvmModule,&perfectNest);
      /* if we don't have a perfect nest, we can't safely
       * run a loop nest in parallel. handle this
       * by disabling the outer-loop multithreading
       * and rerunning the code generator */
      if(opts->en_pe && !perfectNest)
	{
	  loopHash.clear();
	  p = new xmlProgram(opts->en_vec, 
			     opts->en_omp, 
			     opts->en_unroll, 
			     false,
			     opts->en_shift, 
			     256, 
			     opts->vec_len, 
			     opts->en_fusion,
			     opts->en_aligned);
	  p->root = p;
	  p->parseIt(elementRoot, loopHash);
	  p->forStmtHash = loopHash;
	  p->printProgram();    
	  llFunct = p->runCodeGen(&llvmModule,&perfectNest);
	}

      p->dumpDot(functName);


      int lb,ub;
      std::string varUB, varLB;
      int iterSpace = p->isOuterLoopPar(lb,ub,varLB,varUB);
      if(iterSpace!=-1)
	{
	  assert(lb != -1);
	  assert(ub != -1);
	  parLoopBound_t pB;
	  pB.lb = lb;
	  pB.ub = ub;
	  pB.varUB = varUB;
	  pB.varLB = varLB;
	  parIterMap[functName] = pB;
	}
    }
  catch (const XMLException& toCatch) 
    {
      char* message = XMLString::transcode(toCatch.getMessage());
      cout << "Exception message is: \n"
	   << message << "\n";
      XMLString::release(&message);
      return -1;
    }
  catch (const DOMException& toCatch) 
    {
      char* message = XMLString::transcode(toCatch.msg);
      cout << "Exception message is: \n"
	   << message << "\n";
      XMLString::release(&message);
      return -1;
    }
  catch (...) 
    {
      cout << "Unexpected Exception \n" ;
      return -1;
    }
  
  llvm::LLVMContext &Context = 
    llvm::getGlobalContext();
  std::string ErrStr;
  EngineBuilder myBuilder(llvmModule);
  myBuilder.setOptLevel(CodeGenOpt::Aggressive);
  myBuilder.setErrorStr(&ErrStr);
 
  if(check_avx())
    {
      printf("AVX ENABLED...\n");
      myBuilder.setUseMCJIT(true);
      myBuilder.setMCPU("corei7-avx");
      std::vector<std::string> attrs;
      attrs.push_back("avx");
      myBuilder.setMAttrs(attrs);
    }

  ExecutionEngine *TheExecutionEngine = myBuilder.create();
  functMap[functName]= pair<ExecutionEngine*, Function*>
    (TheExecutionEngine, llFunct);
  
  void *tptr = generateLLVMTrampoline(llFunct, functName);

  //void *tptr = generateClangTrampoline(llFunct, functName);
  void *fptr =  TheExecutionEngine->getPointerToFunction(llFunct);
  assert(tptr != 0);
  assert(fptr != 0);

  functPtrMap[functName] = pair<void*, void*>(tptr, fptr);
  xmlHashes[functName] = h;

  delete p;
  delete parser;
  delete errHandler;
  return 0;
}
        
