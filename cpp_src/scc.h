#ifndef __SCC__
#define __SCC__

#include "graph.h"
bool isCyclic(std::list<depGraph*> &nodes);

std::list<std::list<depGraph*> > kosaraju(std::list<depGraph*> &nodes, int level);

#endif
