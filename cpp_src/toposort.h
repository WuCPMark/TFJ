#include "graph.h"

#ifndef __TOPOSORT__
#define __TOPOSORT__

std::list<std::list<depGraph*> > 
topoSort(std::list<std::list<depGraph*> > &sccs);
#endif
