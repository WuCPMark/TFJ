#ifndef __BANERJEE__
#define __BANERJEE__
class directionVector;

#include "graph.h"
#include "ast.h"
#include <list>
#include <set>
#include <limits.h>
#include <assert.h>


typedef enum 
{
  DIR_L=0, /* < */
  DIR_E, /* = */
  DIR_R, /* > */
  DIR_S  /* * */
} dir_t;

class directionVector
{
 public:
  bool operator < (const directionVector &rhs) 
    const;
  bool operator == (const directionVector &rhs) 
    const;
  void shiftLoopLevels(int k, int p);
  void print();
  directionVector(dir_t *v);
  dir_t v[MAX_DEPTH+1];
  int level;
  unsigned long hashed;
};

class dd_coefficient
{
 public:
  int      coeff;        /* Value of this coefficient */
};

/* The subscript table itself.                                    */

class dd_subscript
{
public:
  dd_subscript();
  bool isIndirect;
  MemRef *indirectMR;
  void print();
  void setCoeff(int index, int value);

  /* need pointer to AST location */
  dd_coefficient coeffs[MAX_DEPTH + 1];     
};

class dd_statement
{
 public:
  dd_statement();
  dd_statement(ScalarRef *SR);
  dd_statement(MemRef *MR);
  int depth;
  void addSubscript(dd_subscript* ss);
  std::list<dd_subscript*> subscripts;
  int loopCounts[MAX_DEPTH+1];
  MemRef *MR;
  bool isIndirect;
  int Id;
  void print();
};


int findLevel(dir_t *dirVec);

  /* returns false if no dependence */
bool banerjee(dd_statement &s1, dd_statement &s2, 
	      dir_t *dirVec, 
	      int common_nesting);

bool recurseVectors(dd_statement &s1, dd_statement &s2, 
		    std::set<directionVector> &vSet,
		    int common_nesting,
		    dir_t *dirVector, int k);

#endif
