#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <limits.h>

#include "graph.h"
#include "scc.h"
#include "ast.h"
#include "helper.h"
static unsigned int gid = 1;


depGraph::depGraph(std::string name)
{
  this->name = name;
  this->id = gid++;
  this->ae = 0;
  NodeEdges = new std::list<depEdge>[MAX_DEPTH];
}

depGraph::depGraph(const depGraph *g)
{
  this->name = g->name;
  this->id = g->id;
  this->ae = g->ae;
  NodeEdges = new std::list<depEdge>[MAX_DEPTH];
}


depGraph::~depGraph()
{
  delete [] NodeEdges;
}

void depGraph::AddEdge(depGraph *n, dep_t d, int l)
{
  //printf("ADDING EDGE\n");
  for(std::list<depEdge>::iterator it = NodeEdges[l].begin();
      it != NodeEdges[l].end(); it++)
    {
      depEdge_t t = *it;
      if(t.edgeType==d && t.node == n)
	{
	  //printf("DUPLICATE EDGE\n");
	  return;
	}
    }

  /* edge starts at this and goes to n */
  depEdge_t e;
  e.edgeType = d;
  e.node = n;
  assert(l>=1); assert(l < MAX_DEPTH);

  NodeEdges[l].push_back(e); 
}

void depGraph::shiftLoopLevels(int k, int p)
{
  int OldEdges = 0,NewEdges=0;
  
  std::list<depEdge> *TempEdges = 
    new std::list<depEdge>[MAX_DEPTH];
  
  for(int i=0;i<MAX_DEPTH;i++)
    {
      OldEdges += NodeEdges[i].size();
      TempEdges[i] = NodeEdges[i];
    }

  for(int i=k;i<p;i++)
    TempEdges[i+1] = NodeEdges[i];
  
  TempEdges[k] = NodeEdges[p];

  for(int i=0;i<MAX_DEPTH;i++)
    {
      NodeEdges[i].clear();
      NodeEdges[i] = TempEdges[i];
      NewEdges += NodeEdges[i].size();
    }
  assert(OldEdges == NewEdges);
  delete [] TempEdges;

  assert(ae);
  ae->shiftLoopLevels(k,p);

}


void depGraph::RemoveEdgeLevel(int l)
{
  /* clear all edges at this level */
  NodeEdges[l].clear();
}


bool depGraph::hasEdgeAtLevel(int l)
{
  assert(l>=1); assert(l < MAX_DEPTH);
  
  if(NodeEdges[l].size() > 0)
    return true;
  else
    return false;
}

std::string depGraph::PrintEdges(int level)
{
  std::string as_dot;
  std::string levelStr =  int2string(level);

  for(std::list<depEdge>::iterator it = NodeEdges[level].begin();
      it != NodeEdges[level].end(); it++)
    {
      depEdge e = (*it);
      as_dot += name + " -> " + e.node->name + 
	std::string("[label = \"") + 
	depNames[e.edgeType] + " " + levelStr + 
	std::string("\"] ") + 
	std::string(";\n");
    }
  return as_dot;
}

bool depGraph::checkForFusion(depGraph *other)
{
  int numEdges = 0;
  for(int level=0; level < MAX_DEPTH; level++)
    {
      for(std::list<depEdge>::iterator it = NodeEdges[level].begin();
	  it != NodeEdges[level].end(); it++)
	{
	  depEdge e = (*it);
	  if(e.node->name == other->name)
	    {
	      if(level != LOOP_INDEP)
		return false;
	      /*
	      printf("name=%s,e->name=%s,other->name=%s\n",
		     name.c_str(),
		     e.node->name.c_str(),
		     other->name.c_str());
	      */
	      numEdges++;
	    }
	}
    }

  if(numEdges > 1)
    return false;

  return true;
}

std::string dumpGraph(std::list<depGraph*> &nodes, int level)
{
  std::string as_dot = std::string("digraph G {\n"); 
  for(std::list<depGraph*>::iterator it = nodes.begin();
      it != nodes.end(); it++)
    {
      as_dot += (*it)->PrintEdges(level);
    }
  as_dot += std::string("}\n\n");
  return as_dot;
}

std::string dumpGraph(std::list<depGraph*> &nodes)
{
  std::string as_dot = std::string("digraph G {\n"); 
  for(int i = 0; i < MAX_DEPTH; i++) 
    {
      for(std::list<depGraph*>::iterator it = nodes.begin();
	  it != nodes.end(); it++)
	{
	  as_dot += (*it)->PrintEdges(i);
	}
    }
  as_dot += std::string("}\n\n");
  return as_dot;
}

void regionGraph::removeNode(depGraph *g)
{
  depGraph *gPtr = 0;
  unsigned int id = g->id;

  for(std::list<depGraph*>::iterator it = nodes.begin();
      it != nodes.end(); it++)
    {
      depGraph *gg = *it;
      if(gg->id == id)
	{
	  gPtr = gg;
	  nodes.erase(it);
	  delete gg;
	  break;
	}
    }
  if(gPtr==0)
    {
      printf("unable to find node\n");
      return;
    }
  //iterate over all nodes in graph to remove edges
  for(std::list<depGraph*>::const_iterator it = nodes.begin();
      it != nodes.end(); it++)
    {
      depGraph *gg = (*it);
      for(int i = 0; i < MAX_DEPTH; i++)
	{
	  bool removed_node;
	  do
	    {
	      removed_node = false;
	      for(std::list<depEdge>::iterator eit = gg->NodeEdges[i].begin();
		  eit != gg->NodeEdges[i].end(); eit++)
		{
		  depEdge e = (*eit);
		  if(e.node == gPtr)
		    {
		      gg->NodeEdges[i].erase(eit);
		      removed_node=true;
		      break;
		    }
		}
	    } while(removed_node);
	}
    }
}

regionGraph::regionGraph(std::list<depGraph*> &nodes)
{
  this->nodes = nodes;
}

regionGraph::regionGraph(const regionGraph &g, int level)
{
  std::map<depGraph*, depGraph*> ptrMap;

  /* copy graph */
  for(std::list<depGraph*>::const_iterator it = g.nodes.begin();
      it != g.nodes.end(); it++)
    {
      depGraph *oldNode = (*it);
      depGraph *newNode = new depGraph(oldNode);
      ptrMap[oldNode] = newNode;

      nodes.push_back(newNode);
    }

  /* add edges */
  for(std::list<depGraph*>::const_iterator it = g.nodes.begin();
      it != g.nodes.end(); it++)
    {
      depGraph *oldNode = (*it);
      depGraph *newNode = ptrMap[oldNode];
 
      for(int i = level; i < MAX_DEPTH; i++)
	{
	  for(std::list<depEdge_t>::const_iterator jt = 
		oldNode->NodeEdges[i].begin();
	      jt != oldNode->NodeEdges[i].end(); jt++)
	    {
	      depEdge_t e = *jt;
	      depGraph *oldEdge = e.node;
	      depGraph *newEdge = ptrMap[oldEdge];
	      newNode->AddEdge(newEdge, e.edgeType, i);
	    }
	}
    }
 }


void regionGraph::shiftLoopLevels(int k, int p)
{
  if(k==(MAX_DEPTH-1) || p==(MAX_DEPTH-1))
    {
      printf("error: k=%d, p=%d\n at %d in %s",
	     k,p,__LINE__,__FILE__);
      exit(-1);
    }

  for(std::list<depGraph*>::iterator it = nodes.begin();
      it != nodes.end(); it++)
    {
      (*it)->shiftLoopLevels(k,p);
    }
}

regionGraph::~regionGraph()
{
  while(nodes.size()!=0)
    {
      depGraph *g = nodes.front();
      nodes.pop_front();
      delete g;
    }
}

int regionGraph::outerMostDepLevel()
{
  int ml=MAX_DEPTH;
  for(int i=0;i<MAX_DEPTH;i++)
    {
      for(std::list<depGraph*>::iterator it = nodes.begin();
	  it != nodes.end(); it++)
	{
	  depGraph *dG = *it;
	  if(dG->NodeEdges[i].size() > 0)
	    ml = min(ml,i);
	}
    }
  if(ml==MAX_DEPTH) 
    return 0;
  else
    return ml;
}

std::string regionGraph::dumpGraph()
{
  std::string as_dot = std::string("digraph G {\n"); 
  for(int i = 0; i < MAX_DEPTH; i++)
    {
      for(std::list<depGraph*>::iterator it = nodes.begin();
	  it != nodes.end(); it++)
	{
	  as_dot += (*it)->PrintEdges(i);
	}
    }
  as_dot += std::string("}\n\n");
  return as_dot;
}

size_t regionGraph::numNodes()
{
  return nodes.size();
}



