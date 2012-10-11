#include <cstdlib>
#include <cstdio>
#include <list>
#include <algorithm>
#include <cassert>
#include <cstring>

#include "codegen.h"
#include "toposort.h"
#include "ast.h"

#include <iostream>

using namespace llvm;

scope::scope()
{
  en_serial = false;
  initBuilder=false;
  parent=0;
  carriesNoDep = false;
  depLevel = -1;
  myLoop = 0;
  scopeFused = false;
}


bool scope::simdScope()
{
  if(parent==NULL)
    {
      /* if any child is simdizable... */
      for(std::list<scope*>::iterator it=children.begin();
	  it != children.end(); it++)
	{
	  scope *ss = *it;
	  if(ss->simdScope()==true)
	    return true;
	}
      return false;
    }
  else
    {
      for(std::list<scope*>::iterator it=children.begin();
	  it != children.end(); it++)
	{
	  scope *ss = *it;
	  if(ss->simdScope()==false)
	    return false;
	}
      return true;
    }
}

bool assign::simdScope()
{
  ForStmt *vectLoop = ae->inLoops[ae->deepestLevel()];
  if(vectLoop==0)
    {
      dbgPrintf("NULL unroll loop...this is bad\n");
      exit(-1);
    }
  //dbgPrintf("deepest loop ivar = %s\n", 
  //vectLoop->inductionVar.c_str()); 
  return ae->SIMDizable(vectLoop);
}

void scope::fusion()
{
  scope *scopePtrs[2] = {0,0};
  bool outerScope = (myLoop == NULL);
  if(outerScope)
    {
      if(children.size()==2)
	{
	  int c = 0;
	  std::list<scope*>::iterator cit;
	  for(std::list<scope*>::iterator it=children.begin();
	      it != children.end(); it++)
	    {
	      scope *s = *it;
	      scopePtrs[c] = s;
	      if(c==1)
		cit = it;
	      c++;
	    }
	  scope *fuseScope = scopePtrs[0]->fusable(scopePtrs[1], this); 
	  bool validFuse = (fuseScope != 0);
	  //dbgPrintf("fused loops = %s\n", validFuse ? "true" : "false");
	  if(validFuse)
	    {
	      //dbgPrintf("children size = %lu\n", children.size());
	      children.clear();
	      fuseScope->setParent(this);
	      children.push_back(fuseScope);
	      //dbgPrintf("children size = %lu\n", children.size());
	    }
	}
    }
}

scope* scope::fusable(scope *s,scope *masterScope)
{
  if(children.size() > 1)
    { 
      return 0;
    }
  else
    {
      scope *ss = children.front();
      assign *aa = dynamic_cast<assign*>(ss);
      if(aa)
	return aa->fusable(s,masterScope);
      else
	return ss->fusable(s,masterScope);

    }
}

scope* assign::fusable(assign *s, scope *masterScope)
{
  int numParLoopsA = ae->deepestLevel()-level;
  int numParLoopsB = s->ae->deepestLevel()-s->level;
  
  if(numParLoopsA != numParLoopsB)
    return 0;

  /* check that both assignments are in the same 
   * loop nest */
  if(numParLoopsA > 0)
    {
      for(int i=level+1;i<(ae->deepestLevel()+1);i++)
	{
	  if(ae->inLoops[i] != s->ae->inLoops[i])
	    return 0;
	}
    }

  /* we will never fuse loops have anything
   * but loop-independent dependences */
  if(ae->dG->checkForFusion(s->ae->dG)==false)
    return 0;

  if(s->ae->dG->checkForFusion(ae->dG)==false)
    return 0;

  
  // parent->children.push_back(s);
  //s->parent = parent;
  scope *f = new scope();
  this->level = ae->deepestLevel();
  s->level = this->level;
  f->addChild(this);
  f->addChild(s);
  return scopeFusionExpand(f,0,masterScope);
}

scope *assign::scopeFusionExpand(scope *f, int d, scope *masterScope)
 {
   if(d == (ae->deepestLevel()+1))
     {
       f->scopeFused = true;
       return f;
     }
   else
     {
       scope *cs = scopeFusionExpand(f, d+1, masterScope);
       scope *ns = new scope();
       ns->setFor(ae->inLoops, d);

       ns->myIPHI = 0;
       ns->llvmBuilder = masterScope->llvmBuilder;
       ns->initBuilder=true;
       ns->block_amt = block_amt;
       ns->vec_len = masterScope->vec_len;
       ns->en_vec = masterScope->en_vec;
       ns->en_omp = masterScope->en_omp;
       ns->en_unroll = masterScope->en_unroll;
       ns->en_pe = masterScope->en_pe;
       ns->en_shift = masterScope->en_shift;
       ns->en_aligned = masterScope->en_aligned;
       ns->scopeFused = true;
       ns->carriesNoDep = true;
       ns->addChild(cs);
       cs->setParent(ns);
       return ns;
     }
 }

scope* assign::fusable(scope *s, scope *masterScope)
{
  if(s->children.size() == 0)
    {
      assign *aa = dynamic_cast<assign*>(s);
      if(aa)
	return fusable(aa,masterScope);
      else
	return 0;
    }
  else if(s->children.size() > 1)
    {
      return 0;
    }
  else
    {
      scope *ss = s->children.front();
      assign *aa = dynamic_cast<assign*>(ss);
      if(aa)
	return fusable(aa,masterScope);
      else
	return fusable(ss,masterScope);
    }
}



void scope::setDepLevel(int num)
{
  int next_num = num;
  if(myLoop)
    next_num++;
  
  if(carriesNoDep)
    depLevel = next_num;

  for(std::list<scope*>::iterator lit = children.begin();
      lit != children.end(); lit++)
    {
      (*lit)->setDepLevel(next_num);
    }
}

bool scope::innerMostChildDepFree()
{
  if(parent==NULL)
    {
      for(std::list<scope*>::iterator it = children.begin();
	  it != children.end(); it++)
	{      
	  scope *s = *it;
	  if(s->innerMostChildDepFree()==true)
	    return true;
	}
      return false;
    }
  else
    {
      if(children.size() > 0)
	{
	  for(std::list<scope*>::iterator it = children.begin();
	      it != children.end(); it++)
	    {
	      scope *s = *it;
	      if(s->innerMostChildDepFree()==false)
		return false;
	    }
	  return true;
	}
      else
	{
	  return false;
	}
    }
}

ForStmt *scope::outerMostChildDepFree()
{
  if(myLoop == NULL)
    {
      scope *s = children.front();
      return s->outerMostChildDepFree();
    }
  else
    {
      //dbgPrintf("myLoop->var=%s,carriesNoDep=%d\n",
      //myLoop->inductionVar.c_str(),
      //carriesNoDep);
      if(carriesNoDep)
	{
	  myLoop->setTopLoop();
	  return myLoop;
	}
      else
	{
	  return NULL;
	}
    }
}

bool assign::innerMostChildDepFree()
{
  if(ae == NULL)
    {
      dbgPrintf("something is fucked in %s\n",
	    __PRETTY_FUNCTION__);
      exit(-1);
    }
  int numParLoops = ae->deepestLevel()-level;

  if(numParLoops > 0)
    return true;
  else 
    return false;
}

ForStmt* assign::outerMostChildDepFree()
{
  //dbgPrintf("should never see this, NEED TO ABORT\n");
  //exit(-1);
  return NULL;
}


void scope::setFor(ForStmt **n, int d)
{
  if(d==0) return;

  assert(d > 0);
  if(myLoop==NULL)
    {
      myLoop=n[d];
    }
  assert(myLoop==n[d]);

  if(parent != NULL)
    {
      parent->setFor(n, d-1);
      if((d-1)==0 && parent->carriesNoDep && en_pe)
	{
	  //dbgPrintf("WARNING OUT LOOP DEP FREE SET\n");
	  carriesNoDep = true;
	}
    }
}
void scope::addChild(scope *s)
{
  s->parent = this;
  s->vec_len = vec_len;
  s->en_vec = en_vec;
  s->en_unroll = en_unroll;
  s->en_omp = en_omp;
  s->en_pe = en_pe;
  s->block_amt = block_amt;
  s->en_aligned = en_aligned;
  children.push_back(s);
}

bool scope::isPerfectNest()
{
  int numScope = 0;
  int numAssign = 0;

  for(std::list<scope*>::iterator lit = children.begin();
      lit != children.end(); lit++)
    {
      scope *s = *lit;
      assign *a = dynamic_cast<assign*>(s);
      if(a)
	{
	  //dbgPrintf("got an assign,this=%p\n",this);
	  numAssign++;
	}
      else
	{
	  //dbgPrintf("got a scope,this=%p\n",this);
	  numScope++;
	}
    }
  
  //dbgPrintf("myLoop=%p, numAssign=%d, numScope=%d\n",
  //	 myLoop, numAssign, numScope);
   
  if(myLoop == 0)
    {
      if(!scopeFused && (numAssign > 1 || numScope > 1))
	return false;
    }

  /* If we gotten to the bottom of the loop
   * nest, the children will ALL be assign
   * statements.
   * If we have a mix of statement types,
   * then we don't have a perfect nest 
   */

  if( (numScope > 1) )
    {
      //dbgPrintf("%p with %d children returning false, s=%d,a=%d\n",
      //this, children.size(),numScope,numAssign);
      return false;
    }

  /* Ensure that we only have single loop nesting */
  if(numAssign == 0 && (numScope > 1) )
    {
  
      return false;
    }

  for(std::list<scope*>::iterator lit = children.begin();
      lit != children.end(); lit++)
    {
      scope *s = *lit;
      if(s->isPerfectNest()==false)
	return false;
    }

  return true;
}


bool scope::isStatement()
{
  return false;
}

bool assign::isStatement()
{
  return true;
}

bool assign::isPerfectNest()
{
  //dbgPrintf("assign perfect nest called\n");
  return true;
}


std::string scope::makeBlockedHeader(std::string &ivar, int lb, int ub, int st)
{
  char loopHeader[256];
  std::string loopStr;
  std::string tVar = ivar + ivar;
  snprintf(loopHeader, 256, 
	   "for(int %s = %s; %s < min((%s + %d),%d); %s+=%d) {\n", 
	   ivar.c_str(), tVar.c_str(),
	   ivar.c_str(), tVar.c_str(), block_amt, ub,
	   ivar.c_str(), st); 
  loopStr = std::string(loopHeader);
  return loopStr;
}


std::string scope::makeLoopHeader(std::string &ivar, std::string lb, 
				  std::string ub, 
				  int st, bool block)
{
  char loopHeader[256];
  std::string loopStr;
  /*
  if(myLoop)
    {
      dbgPrintf("myLoop=%p,en_pe=%d,myLoop->isTopLoop=%d\n", 
	     myLoop,en_pe,myLoop->isTopLoop);
    }
  */
  if((myLoop!=NULL) && 
     en_pe && 
     myLoop->isTopLoop)
    {
      snprintf(loopHeader, 256, "for(int %s = %s; %s < %s; %s+=%d) {\n", 
	       ivar.c_str(), myLoop->sliceStart.c_str(),
	       ivar.c_str(), myLoop->sliceStop.c_str(),
	       ivar.c_str(), st);
      loopStr = std::string(loopHeader);
    }
  else
    {
      snprintf(loopHeader, 256, "for(int %s = %s; %s < %s; %s+=%d) {\n", 
	       ivar.c_str(), lb.c_str(),
	       ivar.c_str(), ub.c_str(),
	       ivar.c_str(), st);
      loopStr = std::string(loopHeader);
    }
  return loopStr;
};


Value* scope::emit_llvm()
{
  std::string ivar;

  Function *TheFunction = llvmBuilder->GetInsertBlock()->getParent();
  assert(TheFunction);

  Value *lb=0,*ub=0,*st=0;
  BasicBlock *PreheaderBB=0,*LoopBB=0;
  BasicBlock *LoopEndBB=0, *AfterBB =0;
  
  bool allAssigns = true;
  int numAssigns = 0;
  if(children.size()==1)
    {
      scope *c = children.front();
      for(std::list<scope*>::iterator it = c->children.begin();
	  it != c->children.end(); it++)
	{
	  scope *s = *it;
	  assign *a = dynamic_cast<assign*>(s);
	  if(a==NULL)
	    {
	      allAssigns = false;
	      break;
	    }
	  numAssigns++;
	}
    }

  /*
  if(myLoop)
    {
      const char *ivar = myLoop->inductionVar.c_str();
      dbgPrintf("(ivar=%s) allAssigns = %s\n", ivar,
	     allAssigns ? "true" : "false");
    }
  */

  if(scopeFused && allAssigns)
    {
      loopFusion_emit_llvm(myLoop);
    }
  else
    {
      if(myLoop!=NULL)
	{
	  ivar = myLoop->inductionVar.c_str();
	  /* make loop header */
	  
	  if(myLoop->isTopLoop && en_pe)
	    {
	      lb = myLoop->lsliceStart;
	      ub = myLoop->lsliceStop;
	    }
	  else
	    {
	      if(myLoop->isTriLowerBound)
		{
		  buildIVarMap(parent);
		  Value *triLB = iVarMap[myLoop->triLBound];
		  Value *triOffs = ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 
						    (uint64_t)myLoop->triOffset, false);
		  if(triLB==0)
		    {
		      dbgPrintf("couldnt find shit\n");
		      exit(-1);
		    }
		  lb = llvmBuilder->CreateAdd(triLB, triOffs);
		}
	      else
		{
		  lb = ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 
					(uint64_t)myLoop->lowerBound, false);
		}

	      if(myLoop->isDynUBound()==false)
		{
		  ub = ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 
					(uint64_t)(myLoop->upperBound),
					false);
		}
	      else
		{
		  ub = myLoop->dynUBound->llvmBase;
		}
	    }
	  
	  st = ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 
				(uint64_t)myLoop->step, false);
	  
	  PreheaderBB = llvmBuilder->GetInsertBlock();
	  std::string loopName = "loop" + ivar;
	  LoopBB = BasicBlock::Create(getGlobalContext(), 
				      loopName, TheFunction);


	  std::string afterloopName = "afterloop" + ivar;
	  AfterBB = BasicBlock::Create(getGlobalContext(), 
				       afterloopName, TheFunction);

	  std::string shouldCondName = "shouldCondTest" + ivar;
	  Value *shouldIterateCond = llvmBuilder->CreateICmpSLT(lb, ub, shouldCondName);
	  
	  llvmBuilder->CreateCondBr(shouldIterateCond,LoopBB,AfterBB);
	  llvmBuilder->SetInsertPoint(LoopBB);
      
#ifdef USE_LLVM_30
	  myIPHI = llvmBuilder->CreatePHI(Type::getInt32Ty(getGlobalContext()),0,
					  ivar);
#else
	  myIPHI = llvmBuilder->CreatePHI(Type::getInt32Ty(getGlobalContext()),
					  ivar);
#endif
	  /*
	  dbgPrintf("created myIPHI (var=%s) with address %p\n",
		 ivar.c_str(), myIPHI);
	  */
	  myIPHI->addIncoming(lb, PreheaderBB);

	  //dbgPrintf("generated PHI node\n");
	  //PHINode *Variable = Builder.CreatePHI(Type::getDoubleTy(getGlobalContext()), 2, VarName.c_str());
	  //Variable->addIncoming(StartVal, PreheaderBB);

	}
 
      /* call on children */
      for(std::list<scope*>::iterator it=children.begin();
	  it != children.end(); it++)
	{
	  scope *s = *it;
	  assert(s);
	  s->emit_llvm();
	}

      if(myLoop!=NULL)
	{
	  std::string stepName = "next" + ivar;
	  Value *IncrVar = llvmBuilder->CreateAdd(myIPHI, st, stepName);
	  /* close loop */

	  LoopEndBB = llvmBuilder->GetInsertBlock();


	  std::string condName = "condTest" + ivar;
	  Value *EndCond = llvmBuilder->CreateICmpSLT(IncrVar, ub, condName);

	  // Insert the conditional branch into the end of LoopEndBB.
	  llvmBuilder->CreateCondBr(EndCond, LoopBB, AfterBB);
      
	  // Any new code will be inserted in AfterBB.
	  llvmBuilder->SetInsertPoint(AfterBB);
      
	  // Add a new entry to the PHI node for the backedge.
	  myIPHI->addIncoming(IncrVar, LoopEndBB);
      
	}
    }

 return Constant::getNullValue(Type::getDoubleTy(getGlobalContext()));
}

std::string scope::emit_ocl()
{
  std::string asCpp;
  std::string ivar;
  bool gotNonNull = false;

  if(parent==0) {
    setDepLevel(0);
  }

  if(myLoop!=NULL)
    {
      gotNonNull=true;
      assert(carriesNoDep);
      ivar = myLoop->inductionVar;
      int lb = myLoop->lowerBound;
      std::string ub = myLoop->getUBoundStr();
      assert(myLoop->isDynUBound()==false);

      int st = myLoop->step;
      
      asCpp += "int " + ivar + " = get_global_id(0);\n";
      asCpp += "if( " + ivar + " >= " + ub + ") \n \t return; \n";
      asCpp += "{\n";
    }
 
  for(std::list<scope*>::iterator it=children.begin();
      it != children.end(); it++)
    {
      scope *s = *it;
      assert(s);
      if(gotNonNull)
	asCpp += s->emit_c();
      else
	asCpp += s->emit_ocl();
      
    }
 if(myLoop!=NULL)
    {
      asCpp += std::string("/* ending loop ")+ ivar + "*/\n";
      asCpp += "}\n";
    }
  return asCpp;
}

std::string scope::emit_c()
{
  std::string asCpp;
  std::string ivar;

  if(parent==0) {
    setDepLevel(0);
  }

  bool allAssigns = true;
  int numAssigns = 0;
  if(children.size()==1)
    {
      scope *c = children.front();
      for(std::list<scope*>::iterator it = c->children.begin();
	  it != c->children.end(); it++)
	{
	  scope *s = *it;
	  assign *a = dynamic_cast<assign*>(s);
	  if(a==NULL)
	    {
	      allAssigns = false;
	      break;
	    }
	  numAssigns++;
	}
    }
 
  /*
  if(myLoop)
    {
      const char *ivar = myLoop->inductionVar.c_str();
      dbgPrintf("EMIT_C: scope = %p, ivar=%s\n", this, ivar);
    }
  */

  if(scopeFused && allAssigns)
    {
      asCpp += loopFusion_emit_c(myLoop);
    }
  else
    {
      if(myLoop!=NULL)
	{
	  ivar = myLoop->inductionVar;
	  std::string lb = myLoop->getLBoundStr();
	  std::string ub = myLoop->getUBoundStr();
	  //assert(myLoop->isDynUBound()==false);
	  
	  int st = myLoop->step;
	  /*
	  dbgPrintf("%s (level=%d) : carriesNoDep = %d\n",
		 ivar.c_str(), depLevel, carriesNoDep);
	  */
	  asCpp += makeLoopHeader(ivar, lb, ub, st, false);
	}
      
      for(std::list<scope*>::iterator it=children.begin();
	  it != children.end(); it++)
	{
	  scope *s = *it;
	  assert(s);
	  asCpp += s->emit_c();
	  
	}
      if(myLoop!=NULL)
	{
	  asCpp += std::string("/* ending loop ")+ ivar + "*/\n";
	  asCpp += "}\n";
	}
    }
  return asCpp;
}

void assign::print(int d)
{
  int numParLoops = ae->deepestLevel()-level;
  if(numParLoops > 0)
    {
      dbgPrintf("%d parloops in statemnt:",numParLoops);
      for(int i=level+1;i<(ae->deepestLevel()+1);i++)
	dbgPrintf("%s (%p),", 
	       ae->inLoops[i]->inductionVar.c_str(),
	       ae->inLoops[i]
	       );
      dbgPrintf("\n");
    }
  ae->print(0);
}




std::string assign::traverse_par_loops_block(int floop, int lloop, int c)
{
  assert(true==false);
  return std::string("");
}

llvm::Value *assign::traverse_par_loops_llvm(int floop, int lloop, int c)
{
  assert(parent);
  assert(this->initBuilder);
  //dbgPrintf("llvmBuilder=%p\n", llvmBuilder);
 
  Function *TheFunction = llvmBuilder->GetInsertBlock()->getParent();
  std::string ivar = ae->inLoops[c]->inductionVar;
  

  assert(TheFunction);

  Value *lb=0,*ub=0,*st=0;
  BasicBlock *PreheaderBB=0,*LoopBB=0;
  BasicBlock *LoopEndBB=0, *AfterBB =0;


  if((c==lloop) && (en_unroll || en_vec) )
    {
      Value *smLower=0,*smUpper=0,*smStep=0;
      BasicBlock *smPreheaderBB=0,*smLoopBB=0;
      BasicBlock *smLoopEndBB=0, *smAfterBB =0;
      int unrollAmt = vec_len;
      int upperAmt = ae->inLoops[c]->upperBound;
      bool justUnitStride = ae->SIMDizable(ae->inLoops[c]);
      bool doVectorForNest = justUnitStride && en_vec;
      
      /* 
      if(upperAmt < vec_len) {
	doVectorForNest = false;
	unrollAmt = upperAmt;
      }
      */
      
      ae->emitTempLLVMVars(doVectorForNest ? VECTOR : UNROLL, 
			   llvmBuilder, iVarMap,
			   NULL, ae->inLoops[c], vec_len);
      
      /* generate outer loop */
      if(ae->inLoops[c]->isTriLowerBound)
	{
	  buildIVarMap(parent);
	  Value *triLB = iVarMap[ae->inLoops[c]->triLBound];
	  Value *triOffs = ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 
					    (uint64_t)ae->inLoops[c]->triOffset, false);
	  if(triLB==0)
	    {
	      dbgPrintf("couldnt find shit\n");
	      exit(-1);
	    }
	  lb = llvmBuilder->CreateAdd(triLB, triOffs);
	}
      else
	{
	  lb = ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 
				(uint64_t)ae->inLoops[c]->lowerBound, false);
	}

      if(ae->inLoops[c]->isDynUBound()==false)
	{
	  ub = ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 
				(uint64_t)(ae->inLoops[c]->upperBound),
				false);
	}
      else
	{
	  ub = ae->inLoops[c]->dynUBound->llvmBase;
	}
           
      
      Value *ust = ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 
			    (uint64_t)(unrollAmt),
			    false);
      
      st = ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 
			    (uint64_t)(ae->inLoops[c]->step),
			    false);

      PreheaderBB = llvmBuilder->GetInsertBlock();
      Value *iterLen = llvmBuilder->CreateSub(ub,lb);
      Value *smUpperBound = llvmBuilder->CreateSub(ub,llvmBuilder->CreateURem(iterLen,ust));
      //Value *smUpperBound = llvmBuilder->CreateSub(ub,llvmBuilder->CreateURem(ub,ust));
      Value *llvmZero = 
	ConstantInt::get(Type::getInt32Ty(getGlobalContext()),0);

      Value *ZeroCond = llvmBuilder->CreateICmpEQ(llvmZero, smUpperBound, 
						  "smZeroTest" + ivar);

      std::string loopName = "smLoop" + ivar;
      LoopBB = BasicBlock::Create(getGlobalContext(), 
				  loopName, TheFunction);
      assert(LoopBB);
      BasicBlock *bbSecondTerm = 
	BasicBlock::Create(getGlobalContext(), 
			   "secondTerm" + ivar, 
			   TheFunction);
      

      /* Handle the case where the loop iterates fewer times
       * than the vector / strip-mine length */
      BasicBlock *bbThirdTerm = 
	BasicBlock::Create(getGlobalContext(), 
			   "thirdTerm" + ivar, 
			   TheFunction);

      llvmBuilder->CreateCondBr(ZeroCond, bbThirdTerm, LoopBB);
      
      llvmBuilder->SetInsertPoint(bbThirdTerm);
     
#ifdef USE_LLVM_30
      PHINode *jIPHI = llvmBuilder->CreatePHI(Type::getInt32Ty(getGlobalContext()),0,
					      ivar);
#else
      PHINode *jIPHI = llvmBuilder->CreatePHI(Type::getInt32Ty(getGlobalContext()),
					      ivar);
#endif

      jIPHI->addIncoming(lb, PreheaderBB);
      iVarMap[ae->inLoops[c]] = jIPHI;
      ae->emit_llvm(llvmBuilder,iVarMap);
      iVarMap.erase(ae->inLoops[c]);
      
      
      Value* jIVar = llvmBuilder->CreateAdd(jIPHI, st);
      jIPHI->addIncoming(jIVar, bbThirdTerm);
      Value *jCond = llvmBuilder->CreateICmpSLT(jIVar, ub);
      
      llvmBuilder->CreateCondBr(jCond,bbThirdTerm,bbSecondTerm);
    

      /* Handle nice long strip mined blocks */

      llvmBuilder->SetInsertPoint(LoopBB);
#ifdef USE_LLVM_30
      PHINode *oIPHI = llvmBuilder->CreatePHI(Type::getInt32Ty(getGlobalContext()),0,
					      ivar);
#else
      PHINode *oIPHI = llvmBuilder->CreatePHI(Type::getInt32Ty(getGlobalContext()),
					      ivar);
#endif

      oIPHI->addIncoming(lb, PreheaderBB);

      iVarMap[ae->inLoops[c]] = oIPHI;
      /* generate code here */

      /* check that we can actually emit
       * code for this loop */
      if(doVectorForNest)
	{
	  ae->emit_vectorized_llvm(llvmBuilder,iVarMap, oIPHI, en_aligned, vec_len);
	}
      else
	{
	  ae->emit_unrolled_llvm(llvmBuilder,iVarMap, oIPHI, unrollAmt);
	}
      //ae->emit_llvm(llvmBuilder,iVarMap);
      iVarMap.erase(ae->inLoops[c]);


      std::string stepName = "next" + ivar;
      Value *IncrVar = llvmBuilder->CreateAdd(oIPHI, ust, stepName);

      LoopEndBB = llvmBuilder->GetInsertBlock();

      AfterBB = BasicBlock::Create(getGlobalContext(), "firstTerm" + ivar, TheFunction);
      
      std::string condName = "condTest" + ivar;
      Value *EndCond = llvmBuilder->CreateICmpSLT(IncrVar, smUpperBound, condName);
      



      // Insert the conditional branch into the end of LoopEndBB.
      llvmBuilder->CreateCondBr(EndCond, LoopBB, AfterBB);
      
      // Any new code will be inserted in AfterBB.
      llvmBuilder->SetInsertPoint(AfterBB);
      
      // Add a new entry to the PHI node for the backedge.
      oIPHI->addIncoming(IncrVar, LoopEndBB);
      
      /* AND NOW CLEANUP */

      /* generate clean up */
      PreheaderBB = llvmBuilder->GetInsertBlock();

      condName = "preCondTest";
      Value *PreCond = llvmBuilder->CreateICmpEQ(IncrVar, ub, condName);

      loopName = "smCleanUp" + ivar;
      LoopBB = BasicBlock::Create(getGlobalContext(), 
				  loopName, TheFunction);
      assert(LoopBB);

      /*
      BasicBlock *bbSecondTerm = 
	BasicBlock::Create(getGlobalContext(), 
			   "secondTerm" + ivar, 
			   TheFunction);
      */
      AfterBB = bbSecondTerm;
      llvmBuilder->CreateCondBr(PreCond,AfterBB,LoopBB);

      llvmBuilder->SetInsertPoint(LoopBB);
      
#ifdef USE_LLVM_30
      PHINode *cIPHI = llvmBuilder->CreatePHI(Type::getInt32Ty(getGlobalContext()),0,
					      ivar);
#else
      PHINode *cIPHI = llvmBuilder->CreatePHI(Type::getInt32Ty(getGlobalContext()),
					      ivar);
#endif

      cIPHI->addIncoming(smUpperBound, PreheaderBB);

      iVarMap[ae->inLoops[c]] = cIPHI;

      /* generate code here */
      ae->emit_llvm(llvmBuilder,iVarMap);

      stepName = "cNext" + ivar;
      IncrVar = llvmBuilder->CreateAdd(cIPHI, st, stepName);

      LoopEndBB = llvmBuilder->GetInsertBlock();
                 
      condName = "condTest" + ivar;
      EndCond = llvmBuilder->CreateICmpSLT(IncrVar, ub, condName);
      
      // Insert the conditional branch into the end of LoopEndBB.
      llvmBuilder->CreateCondBr(EndCond, LoopBB, AfterBB);
      
      // Any new code will be inserted in AfterBB.
      llvmBuilder->SetInsertPoint(AfterBB);
      
      // Add a new entry to the PHI node for the backedge.
      cIPHI->addIncoming(IncrVar, LoopEndBB);
    }
  else 
    {
      lb = ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 
			    (uint64_t)(ae->inLoops[c]->lowerBound),
			    false);
      if(ae->inLoops[c]->isDynUBound()==false)
	{
	  ub = ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 
				(uint64_t)(ae->inLoops[c]->upperBound),
				false);
	}
      else
	{
	  ub = ae->inLoops[c]->dynUBound->llvmBase;
	}
            
      st = ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 
			    (uint64_t)(ae->inLoops[c]->step),
			    false);
      PreheaderBB = llvmBuilder->GetInsertBlock();
      
      std::string loopName = "parLoop" + ivar;
      LoopBB = BasicBlock::Create(getGlobalContext(), 
				  loopName, TheFunction);
      assert(LoopBB);
      
      
      llvmBuilder->CreateBr(LoopBB);
      llvmBuilder->SetInsertPoint(LoopBB);
      
#ifdef USE_LLVM_30
      PHINode *tIPHI = llvmBuilder->CreatePHI(Type::getInt32Ty(getGlobalContext()),0,
					      ivar);
#else
      PHINode *tIPHI = llvmBuilder->CreatePHI(Type::getInt32Ty(getGlobalContext()),
					      ivar);
#endif
      
      tIPHI->addIncoming(lb, PreheaderBB);
      iVarMap[ae->inLoops[c]] = tIPHI;
      
      //dbgPrintf("generated TPHI node\n");
      
      if(c < lloop)
	traverse_par_loops_llvm(floop, lloop, c+1);
      else
	{
	  ae->emit_llvm(llvmBuilder,iVarMap);
	}
      
      std::string stepName = "next" + ivar;
      Value *IncrVar = llvmBuilder->CreateAdd(tIPHI, st, stepName);
      /* close loop */
      
      LoopEndBB = llvmBuilder->GetInsertBlock();
      std::string afterloopName = "afterloop" + ivar;
      AfterBB = BasicBlock::Create(getGlobalContext(), afterloopName, TheFunction);
      
      std::string condName = "condTest" + ivar;
      Value *EndCond = llvmBuilder->CreateICmpSLT(IncrVar, ub, condName);
      
      // Insert the conditional branch into the end of LoopEndBB.
      llvmBuilder->CreateCondBr(EndCond, LoopBB, AfterBB);
      
      // Any new code will be inserted in AfterBB.
      llvmBuilder->SetInsertPoint(AfterBB);
      
      // Add a new entry to the PHI node for the backedge.
      tIPHI->addIncoming(IncrVar, LoopEndBB);
    }

  return NULL;
}

llvm::Value *scope::loopFusion_emit_llvm(ForStmt *unrollLoop)
{ 
  buildIVarMap(parent);
  bool vectorizeLoop = false;
  std::string ivar = unrollLoop->inductionVar;
  Function *TheFunction = llvmBuilder->GetInsertBlock()->getParent();
  Value *lb=0,*ub=0,*st=0;
  BasicBlock *PreheaderBB=0,*LoopBB=0;
  BasicBlock *LoopEndBB=0, *AfterBB =0;
  Value *smLower=0,*smUpper=0,*smStep=0;
  BasicBlock *smPreheaderBB=0,*smLoopBB=0;
  BasicBlock *smLoopEndBB=0, *smAfterBB =0;
  int upperAmt = unrollLoop->upperBound;
  
  if(en_vec)
    vectorizeLoop = checkVectorizable(unrollLoop);
    
  if(vectorizeLoop)
    dbgPrintf("loop is SIMDizable\n");
  else
    dbgPrintf("can't do shit\n");


  /*emit temporary variables */
  emitTempVars((vectorizeLoop ? VECTOR : UNROLL), unrollLoop, llvmBuilder,
	       iVarMap, vec_len);
  
  if(unrollLoop->isTriLowerBound)
    {
      dbgPrintf("not supported\n");
      exit(-1);
    } 
  else 
    {
      lb = ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 
			    (uint64_t)(unrollLoop->lowerBound),
			    false);
    }

  if(unrollLoop->isDynUBound()==false)
    {
      ub = ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 
			    (uint64_t)(unrollLoop->upperBound),
			    false);
    }
  else
    {
      ub = unrollLoop->dynUBound->llvmBase;
    }
  
  Value *ust = ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 
				(uint64_t)(vec_len),
				false);
  
  st = ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 
			(uint64_t)(unrollLoop->step),
			false);
  
  PreheaderBB = llvmBuilder->GetInsertBlock();
  Value *iterLen = llvmBuilder->CreateSub(ub,lb);
  Value *smUpperBound = llvmBuilder->CreateSub(ub,llvmBuilder->CreateURem(iterLen,ust));
  Value *llvmZero = 
    ConstantInt::get(Type::getInt32Ty(getGlobalContext()),0);
  
  Value *ZeroCond = llvmBuilder->CreateICmpEQ(llvmZero, smUpperBound, 
					      "smZeroTest" + ivar);

  std::string loopName = "smLoop" + ivar;
  LoopBB = BasicBlock::Create(getGlobalContext(), 
			      loopName, TheFunction);
  assert(LoopBB);
  BasicBlock *bbSecondTerm = 
    BasicBlock::Create(getGlobalContext(), 
		       "secondTerm" + ivar, 
		       TheFunction);
  
  
  /* Handle the case where the loop iterates fewer times
   * than the vector / strip-mine length */
  BasicBlock *bbThirdTerm = 
    BasicBlock::Create(getGlobalContext(), 
		       "thirdTerm" + ivar, 
		       TheFunction);
  
  llvmBuilder->CreateCondBr(ZeroCond, bbThirdTerm, LoopBB);
  
  llvmBuilder->SetInsertPoint(bbThirdTerm);
  
#ifdef USE_LLVM_30
  PHINode *jIPHI = llvmBuilder->CreatePHI(Type::getInt32Ty(getGlobalContext()),0,
					  ivar);
#else
  PHINode *jIPHI = llvmBuilder->CreatePHI(Type::getInt32Ty(getGlobalContext()),
					  ivar);
#endif
  
  jIPHI->addIncoming(lb, PreheaderBB);
  iVarMap[unrollLoop] = jIPHI;
  /* generate the "good stuff" */
  generateLLVM(false, NULL, -1, false, iVarMap);
  iVarMap.erase(unrollLoop);
  
  Value* jIVar = llvmBuilder->CreateAdd(jIPHI, st);
  jIPHI->addIncoming(jIVar, bbThirdTerm);
  Value *jCond = llvmBuilder->CreateICmpSLT(jIVar, ub);
  
  llvmBuilder->CreateCondBr(jCond,bbThirdTerm,bbSecondTerm);
    
  /* Handle nice long strip mined blocks */
  llvmBuilder->SetInsertPoint(LoopBB);

#ifdef USE_LLVM_30
      PHINode *oIPHI = llvmBuilder->CreatePHI(Type::getInt32Ty(getGlobalContext()),0,
					      ivar);
#else
      PHINode *oIPHI = llvmBuilder->CreatePHI(Type::getInt32Ty(getGlobalContext()),
					      ivar);
#endif

      oIPHI->addIncoming(lb, PreheaderBB);

      iVarMap[unrollLoop] = oIPHI;
      
      /* generate code here */
    
      generateLLVM(vectorizeLoop, oIPHI,vec_len,en_aligned,iVarMap);
      iVarMap.erase(unrollLoop);


      std::string stepName = "next" + ivar;
      Value *IncrVar = llvmBuilder->CreateAdd(oIPHI, ust, stepName);

      LoopEndBB = llvmBuilder->GetInsertBlock();

      AfterBB = BasicBlock::Create(getGlobalContext(), "firstTerm" + ivar, TheFunction);
      
      std::string condName = "condTest" + ivar;
      Value *EndCond = llvmBuilder->CreateICmpSLT(IncrVar, smUpperBound, condName);

      // Insert the conditional branch into the end of LoopEndBB.
      llvmBuilder->CreateCondBr(EndCond, LoopBB, AfterBB);
      
      // Any new code will be inserted in AfterBB.
      llvmBuilder->SetInsertPoint(AfterBB);
      
      // Add a new entry to the PHI node for the backedge.
      oIPHI->addIncoming(IncrVar, LoopEndBB);
      
      /* AND NOW CLEANUP */

      /* generate clean up */
      PreheaderBB = llvmBuilder->GetInsertBlock();

      condName = "preCondTest";
      Value *PreCond = llvmBuilder->CreateICmpEQ(IncrVar, ub, condName);

      loopName = "smCleanUp" + ivar;
      LoopBB = BasicBlock::Create(getGlobalContext(), 
				  loopName, TheFunction);
      assert(LoopBB);

 
      AfterBB = bbSecondTerm;
      llvmBuilder->CreateCondBr(PreCond,AfterBB,LoopBB);

      llvmBuilder->SetInsertPoint(LoopBB);
      
#ifdef USE_LLVM_30
      PHINode *cIPHI = llvmBuilder->CreatePHI(Type::getInt32Ty(getGlobalContext()),0,
					      ivar);
#else
      PHINode *cIPHI = llvmBuilder->CreatePHI(Type::getInt32Ty(getGlobalContext()),
					      ivar);
#endif

      cIPHI->addIncoming(smUpperBound, PreheaderBB);

      iVarMap[unrollLoop] = cIPHI;

      /* TODO: emit LLVM here */
      generateLLVM(false, NULL, -1, false, iVarMap);
     

      stepName = "cNext" + ivar;
      IncrVar = llvmBuilder->CreateAdd(cIPHI, st, stepName);

      LoopEndBB = llvmBuilder->GetInsertBlock();
                 
      condName = "condTest" + ivar;
      EndCond = llvmBuilder->CreateICmpSLT(IncrVar, ub, condName);
      
      // Insert the conditional branch into the end of LoopEndBB.
      llvmBuilder->CreateCondBr(EndCond, LoopBB, AfterBB);
      
      // Any new code will be inserted in AfterBB.
      llvmBuilder->SetInsertPoint(AfterBB);
      
      // Add a new entry to the PHI node for the backedge.
      cIPHI->addIncoming(IncrVar, LoopEndBB);
      
      return 0;
}

std::string scope::loopFusion_emit_c(ForStmt *unrollLoop)
{
  std::string asCpp;
  bool vectorizeLoop = false;
  
  std::string ivar = unrollLoop->inductionVar;
  std::string lb = unrollLoop->getLBoundStr();
  std::string ub = unrollLoop->getUBoundStr();
  int st = unrollLoop->step;
  std::string ubStr = (unrollLoop->getUBoundStr());
    
  asCpp += "/* Vectorizable loop */ \n";
  asCpp += "{\n";
  
  if(en_vec)
    {
      vectorizeLoop = checkVectorizable(unrollLoop);
    }
  if(vectorizeLoop)
    {
      dbgPrintf("LF: loop is SIMDizable\n");
      asCpp += "/* LF: Vectorizable with unit stride operations */ \n";
      std::string tS = emitTempVars(VECTOR,unrollLoop,vec_len);
      asCpp += tS;
    }
  else
    {
      asCpp += "/* LF: Non-unit stride unit stride vector operations */ \n";
      asCpp += emitTempVars(UNROLL,unrollLoop,1);
    }
  
  std::string smVar = ivar + "_smlen";
  
  block_amt = vec_len;
  std::string strSMUB = "min((" + ivar+ivar + "+" + int2string(block_amt)  + "), " + ubStr  + ")";
  
  std::string strUB = "(" + ubStr +")";
  std::string strLB = "(" + lb +")";
  std::string strDiff = "(" + strUB + " - " + strLB + ")";

  /* generate strip-mine bound */
  asCpp += "int " + smVar + " = " + strUB + 
    "- (" + strDiff + " % " + int2string(vec_len) + ");\n";
  
  int unrollAmt = vec_len;
  /* generate sm'd loop */
  std::string smStart = strLB;
  asCpp += "for(int " + ivar + "=" + smStart + ";" + ivar + "<" + smVar + ";" 
    + ivar + "+=" + int2string(unrollAmt) + ") {\n";
  
  asCpp += generateCode(vectorizeLoop, unrollLoop, unrollAmt);
  
  /* INSERT VECT / UNROLLED CODE HERE */
  asCpp += "}\n";
  /* generate clean up loop */
  asCpp += "for(int " + ivar + "=" + smVar + ";" + ivar + "<" + strUB + ";" 
    + ivar + "++) {\n";
  /* INSERT CLEANUP HERE */
  asCpp += generateCode(vectorizeLoop, unrollLoop, -1);

  asCpp += "}\n";
  asCpp += std::string("/* ending parloop ")+ ivar + "*/\n";
  
asCpp += "}\n";
  return asCpp;
}

bool scope::checkVectorizable(ForStmt *vectLoop)
{ 
  for(std::list<scope*>::iterator it=children.begin();
      it != children.end(); it++)
    {
      scope *ss = *it;
      if(ss->checkVectorizable(vectLoop)==false)
	return false;
    }
  return true;
}

bool assign::checkVectorizable(ForStmt *vectLoop)
{
  return ae->SIMDizable(vectLoop);
}

std::string scope::generateCode(bool doVec, ForStmt *l, int amt)
{
  std::string asCpp;
  for(std::list<scope*>::iterator it=children.begin();
      it != children.end(); it++)
    {
      scope *ss = *it;
      asCpp += ss->generateCode(doVec, l, amt);
    }
  return asCpp;
}


std::string assign::generateCode(bool doVec, ForStmt *l, int amt)
{
  if(amt == -1)
    {
      return ae->emit_c();
    }
  else
    {
      if(doVec)
	{
	  return ae->emit_vectorized_c(l);
	}
      else
	{
	  return ae->emit_unrolled_c(l, amt);
	}
    }
}


llvm::Value *scope::emitTempVars(tempVar_t t, ForStmt *l,
				 llvm::IRBuilder<> *llBuilder,
				 std::map<ForStmt*,llvm::PHINode*> &iMap,
				 int vecLen)
{ 
  for(std::list<scope*>::iterator it=children.begin();
      it != children.end(); it++)
    {
      scope *ss = *it;
      ss->emitTempVars(t,l,llBuilder,iMap,vecLen);
    }
  return 0;
}

llvm::Value *assign::emitTempVars(tempVar_t t, ForStmt *l,
				  llvm::IRBuilder<> *llBuilder,
				 std::map<ForStmt*,llvm::PHINode*> &iMap,
				 int vecLen)
{
  llvmBuilder = llBuilder;
  if(llvmBuilder == 0)
    {
      dbgPrintf("llvmBuilder is nul\n");
      exit(-1);
    }
  return ae->emitTempLLVMVars(t, llvmBuilder, iMap, NULL, l, vecLen);
}


std::string scope::emitTempVars(tempVar_t t, ForStmt *l,int vecLen)
{ 
  std::string asCpp;
  for(std::list<scope*>::iterator it=children.begin();
      it != children.end(); it++)
    {
      scope *ss = *it;
      asCpp += ss->emitTempVars(t, l, vecLen);
    }
  return asCpp;
}

std::string assign::emitTempVars(tempVar_t t, ForStmt *l,int vecLen)
{
  return ae->emitTempVars(t,l,vecLen);
}


llvm::Value *scope::generateLLVM(bool doVec, llvm::PHINode *oIPHI, int amt,
				 bool doAligned, 
				 std::map<ForStmt*,llvm::PHINode*> &iMap)
{
  for(std::list<scope*>::iterator it=children.begin();
      it != children.end(); it++)
    {
      scope *ss = *it;
      ss->generateLLVM(doVec, oIPHI, amt, doAligned, iMap);
    }
  return 0;
}


llvm::Value *assign::generateLLVM(bool doVec, llvm::PHINode *oIPHI, int amt,
				  bool doAligned, 
				  std::map<ForStmt*,llvm::PHINode*> &iMap)
{
  if(amt==-1)
    {
      ae->emit_llvm(llvmBuilder, iMap);
    }
  else
    {
      if(doVec)
	{
	  ae->emit_vectorized_llvm(llvmBuilder,iMap, oIPHI, doAligned, amt);
	}
      else
	{
	  ae->emit_unrolled_llvm(llvmBuilder,iMap, oIPHI, amt);
	}
    }
  return 0;
}






std::string assign::traverse_par_loops(int floop, int lloop, int c)
{
  assert(parent != NULL);
  
  std::string asCpp;
  std::string ivar = ae->inLoops[c]->inductionVar;
  int lb = ae->inLoops[c]->lowerBound;

  int ub = ae->inLoops[c]->upperBound;
  std::string ubStr = (ae->inLoops[c]->getUBoundStr());
  std::string lbStr = (ae->inLoops[c]->getLBoundStr());

  int st = ae->inLoops[c]->step;
 
  //assert(ae->inLoops[c]->isDynUBound()==false);

  bool vectorizeLoop = false;
  bool emitOpenMPiLoop = false;

  asCpp += std::string("/* starting parloop ")+ ivar + "*/\n";

  //dbgPrintf("c=%d, floop=%d, lloop=%d\n", 
  //c, floop, lloop);
 
  /* bottom level parloop, try to run in vector */
  if(c == lloop && (en_vec || en_unroll))
    {
      ForStmt *unrollLoop = ae->inLoops[c];
      assert(unrollLoop);

      asCpp += "/* Vectorizable loop */ \n";
      /* create new scope for sm variables */

      /* vectors and openmp, the good life */
      asCpp += "{\n";


      /* lower bound strip mine */
      std::string lbSM = ivar + "_lsmlen";
      std::string smStart = lbStr;
#ifdef LB_SMINE
      smStart = lbSM;
      asCpp += "int " + lbSM + " = " + lbStr + "+ (" + int2string(vec_len) + " - (" + lbStr + " % " +int2string(vec_len)+ ") );\n";
    
     /* generate pre-fix clean up loop */
      asCpp += "for(int " + ivar + "=" + lbStr + ";" + ivar + "<" + lbSM + ";" 
	+ ivar + "++) {\n";
      asCpp += ae->emit_c();
      asCpp += "}\n";
#endif

      if(ae->SIMDizable(unrollLoop) && en_vec)
	vectorizeLoop = true;

      if(vectorizeLoop)
	{
	  dbgPrintf("PT: loop is SIMDizable\n");
	  asCpp += "/* PT: Vectorizable with unit stride operations */ \n";
	  std::string tS = ae->emitTempVars(VECTOR,unrollLoop,vec_len);
	  asCpp += tS;
	}
      else
	{
	  asCpp += "/* PT: Non-unit stride unit stride vector operations */ \n";
	  asCpp += ae->emitTempVars(UNROLL,unrollLoop,1);
	}

      std::string smVar = ivar + "_smlen";
      
      std::string strSMUB = "min((" + ivar+ivar + "+" + int2string(block_amt)  + "), " + ubStr  + ")";

      std::string strUB = "(" + ubStr +")";

      /* generate strip-mine bound */
      std::string strDiff = "(" + strUB + " - " + lbStr + ")";

      asCpp += "int " + smVar + " = (" + strUB + 
	")- (" + strDiff + " % " + int2string(vec_len) + ");\n";
      
      int unrollAmt = vec_len;
      /* generate sm'd loop */


 
      asCpp += "for(int " + ivar + "=" + smStart + ";" + ivar + "<" + smVar + ";" 
	+ ivar + "+=" + int2string(unrollAmt) + ") {\n";
      
      if(vectorizeLoop)
	asCpp += ae->emit_vectorized_c(unrollLoop);
      else
	asCpp += ae->emit_unrolled_c(unrollLoop, unrollAmt);
      
      asCpp += "}\n";

      /* generate clean up loop */
      asCpp += "for(int " + ivar + "=" + smVar + ";" + ivar + "<" + strUB + ";" 
	+ ivar + "++) {\n";
      asCpp += ae->emit_c();
      asCpp += "}\n";

      asCpp += std::string("/* ending parloop ")+ ivar + "*/\n";
      asCpp += "}\n";
      return asCpp;

    }
  /* top level parloop, run with thread-level parallelism */
  else if(c == floop && en_omp)
    {
      asCpp += "#pragma omp parallel for\n";
    }

  asCpp += makeLoopHeader(ivar, lbStr, ubStr, st, false);
  
  if(c < lloop)
    {
      asCpp += traverse_par_loops(floop,lloop,c+1);
    }
  else
    {
      asCpp += ae->emit_c();
      /* emit loop body here */
    }
  asCpp += "}\n";
  return asCpp;
}


llvm::IRBuilder<> *findBuilder(scope *s)
{
  return s->llvmBuilder;
}

llvm::IRBuilder<> *findBuilder(assign *a)
{

  if(a==NULL) 
    {
      dbgPrintf("line %d: s == NULL\n", __LINE__);
      exit(-1);
    }

  if(!a->initBuilder)
    {
      a->initBuilder=true;
      assert(a->parent);
      return findBuilder(a->parent);
    }
  else
    return a->llvmBuilder;
}

void scope::buildIVarMap(scope *p)
{
  if(p != NULL)
    {
      if(p->myLoop != NULL)
	{
	  dbgPrintf("(map size=%lu) adding ivar variable %s, p->myIPHI=%p\n", 
		 iVarMap.size(), p->myLoop->inductionVar.c_str(),
		 p->myIPHI);
	  if(iVarMap.find(p->myLoop)==iVarMap.end())
	    {
	      iVarMap[p->myLoop] = p->myIPHI;
	    }
	}
      buildIVarMap(p->parent);
    }
}

Value* assign::emit_llvm()
{  
  llvmBuilder = findBuilder(this);
  assert(this->initBuilder);
  buildIVarMap(parent);

  int numParLoops = ae->deepestLevel()-level;
  if(numParLoops > 0)
    {
      //ae->findBestLoopPermutation(level+1,ae->deepestLevel());
      traverse_par_loops_llvm((level+1), ae->deepestLevel(), (level+1));
    }
  else
    {
      ae->emit_llvm(llvmBuilder,iVarMap);
    }
  //dbgPrintf("assign::emit_llvm() called\n");
  return NULL;
}


std::string assign::emit_c()
{
  std::string asCpp;
  int numParLoops = ae->deepestLevel()-level;
  if(numParLoops > 0)
    {
      asCpp += traverse_par_loops((level+1), ae->deepestLevel(), (level+1));
      
      /*
      dbgPrintf("%d parloops in statemnt:",numParLoops);
      for(int i=level+1;i<(ae->deepestLevel()+1);i++)
	dbgPrintf("%s,", ae->inLoops[i]->inductionVar.c_str());
      dbgPrintf("\n");
      */
    }
  else
    {
      asCpp += ae->emit_c();
    }

  return asCpp;
}



void scope::print(int d)
{
  std::string ivar; 
  if(myLoop!=NULL)
    {
      ivar = myLoop->inductionVar;
      dbgPrintf("DO(%p, %s): carries dep=%s (%d children)\n", 
	     this,
	     ivar.c_str(),
	     carriesNoDep ? "false" : "true",
	     (int)children.size()
	     );
      //dbgPrintf("incomplete scope\n");
     }
  else
    {
      dbgPrintf("START_NULL_SCOPE (%p,%s)\n",
	     this,
	     carriesNoDep ? "false" : "true" 
	     );
    }
  for(std::list<scope*>::iterator it=children.begin();
      it != children.end(); it++)
    {
      scope *s = *it;
      assert(s);
      s->print(d+1);
      
    }
 if(myLoop!=NULL)
    {
      dbgPrintf("ENDDO\n");
    }
 else
   {
     dbgPrintf("END_NULL_SCOPE\n");
   }
}



void codeGen(regionGraph &rg, scope *s, int level, int maxDepth)
{
  if(rg.numNodes()==0) return;
  
  /*
  std::string asDot;
  asDot = rg.dumpGraph();
  std::string fname = "graph.dot" + int2string(level);
  FILE *fp = fopen(fname.c_str(), "w");
  fdbgPrintf(fp, "%s\n", asDot.c_str());
  fclose(fp);
  */

  /* compute SCCs at this level */
  std::list<std::list<depGraph*> > clusters = 
    kosaraju(rg.nodes, level);
  assert(clusters.size() <= rg.nodes.size());

  dbgPrintf("clusters = %d\n", clusters.size());

  /* generate new graph with clusters
   * for sccs */
  std::list<std::list<depGraph*> > sccs = 
    topoSort(clusters);

  dbgPrintf("level = %d, maxDepth=%d\n",
	 level, maxDepth);

  /* iterate over topologically sorted pi-blocks */
  for(std::list<std::list<depGraph*> >::iterator it0 = sccs.begin();
      it0 != sccs.end(); it0++)
    {
      int numNodesInCluster = it0->size();
      bool cyclic = isCyclic(*it0);
      bool goodGood = (!cyclic) && (maxDepth > 1 && level==1);
      goodGood = false;
      
      /* cyclic piblock */
      if(cyclic || (s->en_serial && (level <= maxDepth)) ||
	 goodGood)
	{       
	  /* remove find all nodes not part of this piblock */
	  std::list<depGraph*> allNodes;
	  
	  for(std::list<depGraph*>::iterator lit = rg.nodes.begin();
	      lit != rg.nodes.end(); lit++)
	    {
	      allNodes.push_back(*lit);
	    }


	  for(std::list<depGraph*>::iterator lit = it0->begin();
	      lit != it0->end(); lit++)
	    {
	      std::list<depGraph*>::iterator p = 
		find(allNodes.begin(),allNodes.end(), *lit);
	      assert(p != allNodes.end());
	      allNodes.erase(p);
	    }
	  assert(allNodes.size()+it0->size()==rg.nodes.size());

	  /* remove all edges outsize this piblock
	   ...this is inefficient */
	  regionGraph rgg(rg, level+1);
	  
	  for(std::list<depGraph*>::iterator lit = allNodes.begin();
	      lit != allNodes.end(); lit++)
	    {
	      depGraph *g = *lit;
	      rgg.removeNode(g);
	    }

	  /* look at current lowest level dep */
	  int outerDepLevel = rg.outerMostDepLevel();
	  dbgPrintf("outerDepLevel=%d\n", outerDepLevel);
	  
	  if(((level+1) < outerDepLevel) && (outerDepLevel > 0) ||
	     ((level==1) && (outerDepLevel==1)))
	    {
	      dbgPrintf("setting loop dep\n");
	      s->carriesNoDep = false;
	    } 
	  else 	    
	    {
	      dbgPrintf("clearing dep\n");
	      s->carriesNoDep = true;
	    }

	  //if(goodGood)
	  //s->carriesNoDep = true;

	  dbgPrintf("DO(%p) @ level %d (carries dep=%s)\n", 
		 s, level, s->carriesNoDep ? "false" : "true");

	  scope *ns = new scope();
	  ns->setParent(s);
	  s->addChild(ns);
	  
	  dbgPrintf("numNodesInCluster = %d\n", 
		 numNodesInCluster);
	  /* david..this is an ugly hack */
	  dbgPrintf("outerDepLevel = %d, level = %d\n", 
		 outerDepLevel, level);
	  if((numNodesInCluster == 1) && s->en_shift 
	     && (outerDepLevel > level) 
	     && ((outerDepLevel-level) <= 1)
	     /* && (level > 1) */)
	    { 
	      dbgPrintf("trying interchange (level=%d,outer=%d)\n",
		     level, outerDepLevel);
	      rgg.shiftLoopLevels(level, outerDepLevel);
	      
	      regionGraph rggg(rgg, level+1);
	      codeGen(rggg,ns,level+1,maxDepth); 

	    }
	  else
	    {
	      codeGen(rgg,ns,level+1,maxDepth); 
	    }
	  //dbgPrintf("ENDDO\n");
	}
      /* acyclic piblock */
      else
	{
	  /* TODO: look at dependence matrix for reordering? */
	  assert(it0->size()==1);
	  std::list<depGraph*>::iterator lit = it0->begin();
	  depGraph *g = *lit;
	  AssignExpr *ae = g->ae;
	  //ae->checkDepth(level);
	  assert(g->ae != NULL);
	  dbgPrintf("statement %s is in an acyclic piblock at level %d\n",
		 g->name.c_str(), level-1);
	  //ae->printVectors();
	  //ae->printLoops();
	  assign *an = new assign();
	  an->ae = ae;
	  an->setLevel(level-1);
	  s->addChild(an);
	  an->setParent(s);
          dbgPrintf("at level %d:", level); 

	  
	  s->setFor(ae->inLoops, level-1);
	  //exit(-1);

	  if(s->en_shift) 
	    {
	      ae->findBestLoopPermutation(level,ae->deepestLevel());
	    }
	  else
	    {
	      //DBS: weighted set-cover should go here....
	      ae->printVectors();
	    }
	  ae->printLoops();

	}
    }

}
