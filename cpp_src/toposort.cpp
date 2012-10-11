#include "toposort.h"
#include <map>
#include <set>
#include <cstdlib>
#include <cstdio>
#include <algorithm>

static void topoDFS(depGraph *g, 
		    std::list<depGraph*> &tSorted,
                    std::set<depGraph*> &sNodes)
{
  //printf("traversing %s\n", g->name.c_str());
  if(sNodes.find(g) == sNodes.end())
    {
      sNodes.insert(g);
    }
  else
    {
      return;
    }
  for(int i = 0; i <= LOOP_INDEP; i++)
    {
      for(std::list<depEdge>::iterator it = g->NodeEdges[i].begin();
	  it != g->NodeEdges[i].end(); it++)
	{
	  depGraph *gg = (*it).node;
	  if(gg==g) continue;
	  /*
	  printf("edge from %s to %s at level %d\n", 
		 g->name.c_str(),
		 gg->name.c_str(),
		 i);
	  */

	  topoDFS(gg, tSorted, sNodes);
	}
    }
  if(find(tSorted.begin(), tSorted.end(), g)==tSorted.end())
    tSorted.push_front(g);
}


std::list<std::list<depGraph*> > 
 topoSort(std::list<std::list<depGraph*> > &sccs)
{
  /* from old node to new cluster */ 
  std::map<depGraph*, depGraph*> fMap;

  /* new cluster to old cluster */
  std::map<depGraph*, std::list<depGraph*> >rMap;

  std::list<depGraph*> clusters;
  for(std::list<std::list<depGraph*> >::iterator it0 = sccs.begin();
      it0 != sccs.end(); it0++)
    {
      depGraph *g;

      /* multi-node cluster */
      if(it0->size()>1)
	{
	  std::string n;
	  g = new depGraph("unknown");
	  for(std::list<depGraph*>::iterator lit = it0->begin();
	      lit != it0->end(); lit++)
	    {
	      depGraph *o = (*lit);
	      n += o->name + "__";
	      fMap[o]=g;
	      rMap[g].push_back(o);
	    }
	  g->name = n;
	}
      /* single entry */
      else
	{
	  std::list<depGraph*>::iterator lit = it0->begin();
	  depGraph *o = *lit;
	  g = new depGraph(o);
	  fMap[o]=g;
	  rMap[g].push_back(o);
	}
      clusters.push_back(g);
    }



  /* iterate over sccs */
  for(std::list<std::list<depGraph*> >::iterator it0 = sccs.begin();
      it0 != sccs.end(); it0++)
    {
      for(std::list<depGraph*>::iterator lit = it0->begin();
	  lit != it0->end(); lit++)
	{
	  depGraph *o = (*lit);
	  /* new, cluster, node */
	  depGraph *n = fMap[o];

	  /*
	  printf("o name = %s, n name = %s\n", 
		 o->name.c_str(),
		 n->name.c_str());
	  */

	  /* iterate over edges */
	  for(int i = 0; i < MAX_DEPTH; i++)
	    {
	      for(std::list<depEdge>::iterator eit = o->NodeEdges[i].begin();
		  eit != o->NodeEdges[i].end(); eit++)
		{
		  depEdge oe = *eit;
		  depGraph* oo = oe.node;
		  depGraph* nn = fMap[oo];
		  
		  /*
		  printf("\too name = %s, nn name = %s\n", 
			 oo->name.c_str(),
			 nn->name.c_str());
		  */

		  bool found = false;

		  /*
		  printf("edge from %s to %s\n", 
			 n->name.c_str(),
			 nn->name.c_str());
		  */

		  /* okay this is really inefficient.. */
		  for( std::list<depEdge>::iterator eeit = 
			 n->NodeEdges[i].begin();
		       eeit != n->NodeEdges[i].end(); 
		       eeit++)
		    {
		      depEdge ooe = *eeit;
		      if(ooe.edgeType == oe.edgeType 
			 && ooe.node == nn)
			{
			  //printf("edge already exists\n");
			  found = true;
			  break;
			}
		    }

		  if(!found)
		    {
		      /*
		      printf("edge from %s to %s\n", 
			     n->name.c_str(),
			     nn->name.c_str());
		      */
		      n->AddEdge(nn, oe.edgeType, i);
		    }
		  
		}
	    }
	}
    }

  /* find all possible roots of the
   * graph */
  std::map<depGraph*,bool> roots;
  for(std::list<depGraph*>::iterator it = clusters.begin();
      it != clusters.end(); it++)
    {
      roots[*it] = true;
    }

  for(std::list<depGraph*>::iterator it = clusters.begin();
      it != clusters.end(); it++)
    {
      depGraph *n = *it;
      for(int i = 0; i < MAX_DEPTH; i++)
	{
	  for(std::list<depEdge>::iterator eit = n->NodeEdges[i].begin();
	      eit != n->NodeEdges[i].end(); eit++)
	    {
	      depEdge e = *eit;
	      depGraph * nn = e.node;
	      if(nn == n)
		continue;

	      /* node nn has inbound edges,
	       * cant be root */
	      roots[nn] = false;
	    }
	}
    }

  std::list<depGraph*> tSorted;
  std::set<depGraph*> sNodes;
  std::list<depGraph*> pRoots;
  for(std::map<depGraph*,bool>::iterator it = roots.begin();
      it != roots.end(); it++)
    {
      depGraph *g = it->first;
      bool b = it->second;
      if(b)
	pRoots.push_back(g);
    }



  for(std::list<depGraph*>::iterator it = pRoots.begin();
      it != pRoots.end(); it++)
    {
      depGraph *g = *it;
      //printf("searching from %s\n", g->name.c_str());
      if(find(tSorted.begin(),tSorted.end(),g) != tSorted.end())
	continue;

      topoDFS(g, tSorted, sNodes);
      //printf("tSorted.size()=%d\n", (int)tSorted.size()); 
    }
  
  //printf("afterwards: tSorted.size()=%d\n", (int)tSorted.size()); 
  //exit(-1);

  while(tSorted.size() < clusters.size())
    {
      //printf("tSorted.size()=%d\n", (int)tSorted.size());

      for(std::list<depGraph*>::iterator it = clusters.begin();
	  it != clusters.end(); it++)
	{
	  depGraph *g = *it;
	  if(find(tSorted.begin(),tSorted.end(),g) != tSorted.end())
	    continue;
	  
	  topoDFS(g, tSorted, sNodes);
	}
    }
  
  std::list<std::list<depGraph*> > sortSCCs;

  for(std::list<depGraph*>::iterator it = tSorted.begin();
      it != tSorted.end(); it++)
    {
      depGraph *g = *it;
      sortSCCs.push_back(rMap[g]);
    }
  
  while(clusters.size()!=0)
    {
      depGraph *g = clusters.front();
      clusters.pop_front();
      delete g;
    }

  
  


  /*
  std::string as_dot = std::string("digraph G {\n"); 
  for(int i = 0; i < MAX_DEPTH; i++) 
    {
      for(std::list<depGraph*>::iterator it = clusters.begin();
	  it != clusters.end(); it++)
	{
	  as_dot += (*it)->PrintEdges(i);
	}
    }
  as_dot += std::string("}\n\n");

  std::string fname = "topo.dot";
  FILE *fp = fopen(fname.c_str(), "w");
  fprintf(fp, "%s\n", as_dot.c_str());
  fclose(fp);
  */

  return sortSCCs;
}
