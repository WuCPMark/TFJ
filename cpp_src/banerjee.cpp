#include "banerjee.h"

static int p(int t){return (t>0)?t:0;}
static int m(int t){return (t<0)?(-t):0;}
  

void printVector(dir_t *dir, int common_nesting);

directionVector::directionVector(dir_t *v)
{ 
  for(int i=0;i<MAX_DEPTH+1;i++)
    this->v[i]=v[i];
  level = findLevel(this->v);

  unsigned long p =1;
  hashed = 0;
  for(int i=0;i<MAX_DEPTH+1;i++)
    {
      hashed += v[i]*p;
      p=p*4;
    }
}

void directionVector::print()
{
  printVector(v, MAX_DEPTH);
}

void directionVector::shiftLoopLevels(int k, int p)
{
  dir_t temp[MAX_DEPTH+1];
  for(int i = 0; i < MAX_DEPTH; i++)
    temp[i]=v[i];

  for(int i=k;i<p;i++)
    temp[i+1]=v[i];
  
  temp[k]=v[p];

  for(int i=0;i<MAX_DEPTH;i++)
    v[i]=temp[i];

  level = findLevel(v);
  hashed=0;
  unsigned long pwr =1;
  hashed = 0;
  for(int i=0;i<MAX_DEPTH+1;i++)
    {
      hashed += v[i]*pwr;
      pwr=pwr*4;
    }
}

bool directionVector::operator < 
(const directionVector &rhs) const
{
  /* uh..this is kinda dumb */
  return hashed < rhs.hashed;
}

bool directionVector::operator == 
(const directionVector &rhs) const
{
  for(int i=0;i<MAX_DEPTH+1;i++)
    if(v[i]!=rhs.v[i]) return false;
  assert(hashed==rhs.hashed);
  return true;
}

dd_subscript::dd_subscript()
{
  isIndirect=false;
  indirectMR = 0;
  for(int i = 0; i<(MAX_DEPTH+1); i++)
    coeffs[i].coeff=0;
}

void dd_subscript::print()
{
  for(int i=0;i<(MAX_DEPTH+1);i++)
    {
      dbgPrintf("%d ", coeffs[i].coeff);
    }
  dbgPrintf("\n");
}

void dd_subscript::setCoeff(int index, int value)
{
  assert(index < MAX_DEPTH);
  coeffs[index].coeff = value;
}


dd_statement::dd_statement(ScalarRef *SR)
{
  Id = SR->Id;
  dbgPrintf("unimplemented\n");
  exit(-1);
}

void dd_statement::print()
{
  for(std::list<dd_subscript*>::iterator it=subscripts.begin();
      it != subscripts.end(); it++)
    {
      dd_subscript *s = *it;
      s->print();
    }
}

/* convert AST reference into valid
 * for dependence testing */
dd_statement::dd_statement(MemRef *MR)
{
  this->MR = MR;
  Id = MR->Id;
  for(int i = 0; i<(MAX_DEPTH+1); i++)
    loopCounts[i]=INT_MAX;
  isIndirect = false;

  /* this is a complete hack to 
   * determine depth */
  std::list<Node*> nodeList;
  MR->getNesting(nodeList);
  depth=0;
  bool foundFor = false;
  assert(nodeList.size() > 0);
  
  for(std::list<Node *>::iterator it = nodeList.begin();
      it != nodeList.end(); it++)
    {
      ForStmt *f = dynamic_cast<ForStmt*>(*it);
      if(f)
	{
	  //assert(f->lowerBound == 0);
	  assert(f->step == 1);
	  loopCounts[depth] = f->upperBound;
	  	    
	  //dbgPrintf("loopCounts[%d]=%d\n", depth, loopCounts[depth]);
	  depth++;
	  foundFor=true;
	}
    }
  assert(foundFor);
  

  
  if(MR->isIndirect)
    {
      isIndirect = true;
    }
  
  /* convert subscripts into other format */
  for(std::vector<astSubscript*>::iterator it0 = MR->subscripts.begin();
      it0 != MR->subscripts.end(); it0++)
    {
      astSubscript *ast_ss = (*it0);
      dd_subscript *dd_ss = new dd_subscript();
      
      /* copy offset to position 0 */
      dd_ss->coeffs[0].coeff=ast_ss->offset;
      if(ast_ss->isIndirect)
	{
	  dd_ss->isIndirect = true;
	  dd_ss->indirectMR = ast_ss->indirectRef;
	}

      /* outer loop will be visited first */
      int d=0;
      for(std::list<Node *>::iterator it1 = nodeList.begin();
	  it1 != nodeList.end(); it1++)
	{
	  ForStmt *f = dynamic_cast<ForStmt*>(*it1);
	  if(f)
	    {
	      dd_ss->coeffs[1+d].coeff = ast_ss->coeffs[f];
	      d++;
	    }
	}
      assert(d==depth);
      this->subscripts.push_back(dd_ss);
    }
}

dd_statement::dd_statement()
{
  depth=0;
  MR = 0;
  Id = INT_MIN;
  assert(true==false);
  for(int i = 0; i<(MAX_DEPTH+1); i++)
    loopCounts[i]=INT_MAX;
}

void dd_statement::addSubscript(dd_subscript* ss)
{
  subscripts.push_back(ss);
}


/* Banerjee true */
static void bj_true(int ai, int bi, int Ui, int Li, int *LB, int *UB)
{
  *LB += -(p(m(ai)+bi)*(Ui-1)) + ( m(m(ai) + bi) + p(ai) )*Li - bi;
  *UB +=  p(p(ai)-bi)*(Ui-1) - ( m(p(ai) - bi) + m(ai) )*Li - bi;
}

/* Banerjee false */
static void bj_false(int ai, int bi, int Ui, int Li, int *LB, int *UB)
{
  *LB += -m(ai-p(bi))*(Ui-1) + (p(ai-p(bi)) + m(bi))*Li + ai;
  *UB +=  p(ai-m(bi))*(Ui-1) + (m(ai-m(bi)) + p(bi))*Li + ai;
}
/* Banerjee equal */
static void bj_eq(int ai, int bi, int Ui, int Li, int *LB, int *UB)
{
  *LB += -(m(ai-bi)*Ui) + p(ai-bi)*Li;
  *UB += p(ai-bi)*Ui - m(ai-bi)*Li;
}

/* Banerjee star */
static void bj_any(int ai, int bi, int Ui, int Li, int *LB, int *UB)
{
  *LB += -Ui*(m(ai)+p(bi)) + Li*(p(ai)+m(bi));
  *UB += Ui*(p(ai)+m(bi)) - (m(ai)+p(bi))*Li;
}


bool banerjee(dd_statement &s1, dd_statement &s2, 
	      dir_t *dirVec, 
	      int common_nesting)
{
  int i, independent;
  int u;
    
  bool ret_val=true;
  int a[MAX_DEPTH+1];
  int b[MAX_DEPTH+1];
  
  int dep[MAX_DEPTH+1];
  for(int j=0;j<MAX_DEPTH+1;j++)
    {
      a[j] = b[j] = 0;
      dep[j]=1;
    }
  int num_s1 = (int)s1.subscripts.size();
  int num_s2 = (int)s2.subscripts.size();

  if(num_s1 != num_s2)  
    return true;

  /*
    if(s1.isIndirect || s2.isIndirect)
    {
      dbgPrintf("indirect in banerjee\n");
      return true;
      }
  */

  i = 0;
  independent = 0; 

  std::list<dd_subscript*>::iterator s1_it =
    s1.subscripts.begin();

  std::list<dd_subscript*>::iterator s2_it =
    s2.subscripts.begin();

  for(int i = 0; i < num_s1; i++)
    {
      if( (*s1_it)->isIndirect || (*s2_it)->isIndirect )
	{
	  
	  if((*s1_it)->indirectMR != (*s2_it)->indirectMR)
	    {
	      return true;
	    }

	  //(*s1_it)->print();	
	  //(*s2_it)->print();
	  
	}

      for(int j=0;j<MAX_DEPTH+1;j++) 
	{
	  a[j] = (*s1_it)->coeffs[j].coeff;
	  b[j] = (*s2_it)->coeffs[j].coeff;
	}
      int LB = 0, UB = 0;
      int delta = b[0]-a[0];

      /* iterate over induction variables */
      for(int k=1; k<LOOP_INDEP;k++)
	{
	  int tLB=0,tUB=0;
	  u = s1.loopCounts[k-1];
	 
	  if(dirVec[k]==DIR_L)
	    {	       
	      bj_true(a[k], b[k], u, 1, &tLB, &tUB);
	    }
	  else if(dirVec[k]==DIR_S)
	    {
	      bj_any(a[k], b[k], u, 1, &tLB, &tUB); 
	    }   
	  else if(dirVec[k]==DIR_E)
	    {
	      bj_eq(a[k], b[k], u, 1, &tLB, &tUB);
	    }
	  else
	    {
	      bj_false(a[k], b[k], u, 1, &tLB, &tUB);
	    }
	  LB += tLB;
	  UB += tUB;
	}

      //dbgPrintf("LB=%d, UB=%d\n", LB, UB);
      /* check if Banerjee's Inequality has been 
       * violated. We could return here but I'm recording
       * a vector of values because it seemed like
	* a good idea for correctness checking */
      if(!(LB <= delta && delta <= UB))
	{
	  return false;
	}
      
      s1_it++;
      s2_it++;
    }
  
  return true;
  
  return ret_val;
}

int findLevel(dir_t *dirVec)
{

  for(int i=1;i<MAX_DEPTH+1;i++)
    {
      if(dirVec[i]==DIR_L)
	return i;
    }
  return LOOP_INDEP;
  //return MAX_DEPTH;
}

void printVector(dir_t *dir, int common_nesting)
{
  for(int i=1;i<(common_nesting+1);i++)
    {
      switch(dir[i])
	{
	case DIR_L:
	  dbgPrintf("<");
	  break;
	case DIR_R:
	  dbgPrintf(">");
	  break;
	case DIR_E:
	  dbgPrintf("=");
	  break;
	case DIR_S:
	  dbgPrintf("*");
	  break;
	}
      if(i!=(common_nesting))
	dbgPrintf(",");
    }
}

bool isValidVector(dir_t *dir, int common_nesting)
{
  bool found_non_eq = false;
  for(int i=1;i<(common_nesting+1);i++)
    {
      switch(dir[i])
	{
	case DIR_L:
	  found_non_eq =true;
	  break;
	case DIR_R:
	  if(!found_non_eq)
	    return false;
	  break;
	case DIR_E:
	  break;
	case DIR_S:
	  found_non_eq = true;
	  break;
	default:
	  dbgPrintf("UNKNOWN @ %d\n",i);
	  exit(-1);
	}
    }

  return found_non_eq;
}

bool recurseVectors(dd_statement &s1, dd_statement &s2, 
		    std::set<directionVector> &vSet,
		    int common_nesting,
		    dir_t *dirVector, int k)
{
  static dir_t dirs[3] = {DIR_L,DIR_E,DIR_R};
  dir_t newDir[MAX_DEPTH+1];
  bool rc[3];

  bool t = banerjee(s1,s2,dirVector,common_nesting);
  if(t)
    {
      if(k < (common_nesting+1))
	{
	  for(int i=0;i<(MAX_DEPTH+1);i++) 
	    newDir[i]=dirVector[i];
	  for(int i=0;i<3;i++)
	    {
	      newDir[k]=dirs[i];
	      rc[i] = recurseVectors(s1,s2, vSet,
				     common_nesting,newDir,k+1);
	    }
	  for(int i=0;i<3;i++)
	    {
	      if(rc[i]==true) 
		return true;
	    }
	  return false;
	}
      else
	{
	  if(isValidVector(dirVector,common_nesting))
	    {
	      //printVector(dirVector,common_nesting);
	      //dbgPrintf(" level=%d \n",findLevel(dirVector));
	      directionVector v(dirVector);
	      vSet.insert(v);
	      return true;
	    }
	  else
	    {
	      return false;
	    }
	}
    }
  return false;
}


/*
int main()
{
  dd_subscript ss1;
  dd_subscript ss2;
  
  ss1.setCoeff(0,10);
  ss1.setCoeff(1,1);

  ss2.setCoeff(0,0);
  ss2.setCoeff(1,1);

  dd_statement s1;
  dd_statement s2;
  
  s1.addSubscript(&ss1);
  s2.addSubscript(&ss2);
  s1.loopCounts[1] = 10;
  s2.loopCounts[1] = 10;

  banerjee b;
  dir_t dir[] = {DIR_S, DIR_S};
  bool result = b.test(s1, s2, dir, 1);
  
  dbgPrintf("result is %s\n", result ? "true" : "false");
  
  return 0;
}
*/
