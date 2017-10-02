#ifndef NGRAPH_SGCOMPILER_H_
#define NGRAPH_SGCOMPILER_H_

#include "ngraph_graph.h"
#include "ngraph_emitter.h"

namespace ngraph_bridge {

class SGCompiler : public Emitter {
 public:
  SGCompiler(){};
  std::shared_ptr<Graph> Compile(NodePtr sub_graph);
 private:
  // compile subgraph into ngraph python objects
  void CompileSubgraph(std::shared_ptr<Graph> sub_graph);
  // compile inputs to a node
  void CompileInput(NodePtr input);
  void CompileInputs(NodePtr node);
  // compile a single node into an ngraph python object
  void CompileNode(NodePtr node);
  void ClearOpMap();
};

}  // end namespace ngraph
#endif