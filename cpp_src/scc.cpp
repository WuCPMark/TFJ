#include "scc.h"
#include <cassert>
#include <cstdio>
#include <vector>
#include <set>
#include <map>
#include <algorithm>

/* determine if a graph is cyclic */
bool isCyclic(std::list<depGraph*> &nodes)
{
  /* find all possible roots of the
   * graph */
  std::map<depGraph*,bool> roots;
  for(std::list<depGraph*>::iterator it = nodes.begin();
      it != nodes.end(); it++)
    {
      roots[*it] = true;
    }

  for(std::list<depGraph*>::iterator it = nodes.begin();
      it != nodes.end(); it++)
    {
      depGraph *n = *it;
      for(int i = 0; i < MAX_DEPTH; i++)
	{
	  for(std::list<depEdge>::iterator eit = n->NodeEdges[i].begin();
	      eit != n->NodeEdges[i].end(); eit++)
	    {
	      depEdge e = *eit;
	      depGraph * nn = e.node;
	      /* node nn has inbound edges,
	       * cant be root */
	      roots[nn] = false;
	    }
	}
    }

  bool allFalse = false;
  
  for(std::map<depGraph*, bool>::iterator mit=roots.begin();
      mit != roots.end(); mit++)
    {
      allFalse |= mit->second;
    }

  return (!allFalse);
}



void dfs(int idx, std::set<int> *adjList, 
	 std::list<int> &stack, 
	 std::set<int> &seen, int size)
{

  if(seen.find(idx)!=seen.end()) return;
  seen.insert(idx);

  for(std::set<int>::iterator it = adjList[idx].begin();
      it != adjList[idx].end(); it++)
    {
      int nidx = *it;
      if(nidx==idx) continue;
   
      if(seen.find(nidx)!=seen.end()) continue;
    
      dfs(nidx, adjList, stack, seen, size);
    }
    
  stack.push_back(idx);
}

void rfs(int start, int idx, std::set<int> *adjList, 
	 std::list<int> &stack, 
	 std::set<int> &seen, 
	 std::list<int> &cluster)
{
  seen.insert(idx);
  for(std::set<int>::iterator it = adjList[idx].begin();
      it != adjList[idx].end(); it++)
    {
      int nidx = *it;
      if(seen.find(nidx)!=seen.end()) continue;
      rfs(start, nidx, adjList, stack, seen, cluster);
    }
  
  std::list<int>::iterator pos = find(stack.begin(), stack.end(), idx);
  assert(pos != stack.end());
  stack.erase(pos);
  cluster.push_back(idx);
}


std::list<std::list<depGraph*> > kosaraju(std::list<depGraph*> &nodes, int level)
{
  //printf("SCC: called at level = %d\n", level);

  /* i'm lazy */
  std::map<depGraph*, int> nodeMap;
  std::map<int, depGraph*> revNodeMap;

  std::set<int> *adjFwdMatrix;
  std::set<int> *adjRevMatrix;

  std::list<int> theStack;
  std::set<int> seenNodes;

  std::list<int> cluster;
  std::list<std::list<depGraph*> > sccs;

  adjFwdMatrix = new std::set<int>[nodes.size()];
  adjRevMatrix = new std::set<int>[nodes.size()];

  int deepestFwd = -1, 
    deepestRev = -1;

  for(std::list<depGraph*>::iterator it = nodes.begin();
      it != nodes.end(); it++)
    {
      depGraph *g = *it;
      int p = (int)nodeMap.size();
      nodeMap[g] = p;
      revNodeMap[p] = g;

    }
  assert(nodeMap.size() == nodes.size());
  
  for(std::list<depGraph*>::iterator it0 = nodes.begin();
      it0 != nodes.end(); it0++)
    {
      depGraph *v = *it0;
      int i = nodeMap[v];
      for(int l = level; l < MAX_DEPTH; l++)
	{
	  for(std::list<depEdge>::iterator it1 = v->NodeEdges[l].begin();
	      it1 != v->NodeEdges[l].end(); it1++)
	    {
	      depGraph *u = (*it1).node;
	      int j = nodeMap[u];
	      if(i==j) continue;
	      //printf("FWD: adding edge from node %d to %d\n", i, j);
	      adjFwdMatrix[i].insert(j);

	      if(l > deepestFwd)
		deepestFwd = l;

	    }
	}
    }

  // printf("getting ready to REV\n");
  /* reverse all edges */
  for(std::list<depGraph*>::iterator it0 = nodes.begin();
      it0 != nodes.end(); it0++)
    {
      depGraph *v = *it0;
      int i = nodeMap[v];
      for(int l = level; l < MAX_DEPTH; l++)
	{
	  for(std::list<depEdge>::iterator it1 = v->NodeEdges[l].begin();
	      it1 != v->NodeEdges[l].end(); it1++)
	    {
	      depGraph *u = (*it1).node;
	      int j = nodeMap[u];
	      if(i==j) continue;
	      
	      /* fwd: edge from i -> j */
	      /* rev: edge from j to i */
	      //printf("REV: adding edge from node %d to %d\n", i, j);
	      adjRevMatrix[j].insert(i);

	      if(l > deepestRev)
		deepestRev = l;
	    }
	}
    }

  //printf("DEEPEST REV = %d, DEEPEST FWD = %d\n", 
  //deepestFwd, deepestRev);
  //exit(-1);

  int c=0;
  int size = nodeMap.size();
  while((int)theStack.size() < size)
    {
      dfs(c, adjFwdMatrix, theStack, seenNodes, size);
      c++;
    }
  seenNodes.clear();
  
  /*
  for(std::list<int>::iterator it = theStack.begin();
      it != theStack.end(); it++)
    {
      depGraph *g = revNodeMap[*it];
      printf("%d: %s\n",*it, g->name.c_str());
    }
  */
  assert((int)theStack.size() == size);

  std::list<depGraph*> scc;
  while(theStack.size() != 0)
    {
      scc.clear();
      cluster.clear();
      c = theStack.back();
      rfs(c, c, adjRevMatrix, theStack, seenNodes, cluster);

      for(std::list<int>::iterator it = cluster.begin();
	  it != cluster.end(); it++)
	{
	  depGraph *g = revNodeMap[*it];
	  scc.push_back(g);
	}
      sccs.push_back(scc);
    }


  delete [] adjFwdMatrix;
  delete [] adjRevMatrix;

  return sccs;
}
