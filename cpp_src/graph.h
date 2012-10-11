#ifndef __DEPGRAPH__
#define __DEPGRAPH__

#include <list>
#include <map>
#include <string>
#include <cassert>

enum dep_t {TRUE=0, ANTI, OUTPUT, INDEP};
const std::string depNames[] = {"&delta;", "&delta; -1", "&delta; o", "indep"};

#define MAX_DEPTH 10
#define LOOP_INDEP (MAX_DEPTH-1)


class depGraph;
class AssignExpr;

typedef struct depEdge_t
{
 public:
  dep_t edgeType;
  depGraph *node;
} depEdge;


class depGraph
{
 public:
  AssignExpr *ae;
  depGraph(std::string name);
  depGraph(const depGraph *g);
  depGraph() {assert(1==0); }
  ~depGraph();
  void AddEdge(depGraph *n, dep_t d, int l);
  void RemoveEdgeLevel(int l);
  bool hasEdgeAtLevel(int l);
  void shiftLoopLevels(int k, int p);
  std::string PrintEdges(int level);
  bool checkForFusion(depGraph *other);
  std::string name;
  unsigned int id;
  std::list<depEdge> *NodeEdges;
};

class regionGraph
{
 public:
  void removeNode(depGraph *g);
  int outerMostDepLevel();
  void shiftLoopLevels(int k, int p);
  regionGraph(std::list<depGraph*> &nodes);
  explicit regionGraph(const regionGraph &g, int level);
  ~regionGraph();
  std::string dumpGraph();
  size_t numNodes();
  std::list<depGraph*> nodes;
};

std::string dumpGraph(std::list<depGraph*> &nodes);

#endif
