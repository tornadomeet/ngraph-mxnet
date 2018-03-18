/*******************************************************************************
* Copyright 2018 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#ifndef MXNET_NGRAPH_NGRAPH_GRAPH_H_
#define MXNET_NGRAPH_NGRAPH_GRAPH_H_

#include <mxnet/base.h>
#include <nnvm/graph.h>
#include <nnvm/symbolic.h>
#include <nnvm/tuple.h>

#include <algorithm>
#include <deque>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <unordered_set>
#include <vector>

#include <ngraph/ngraph.hpp>
#include "ngraph_graph_utils.h"

namespace ngraph_bridge {

// Useful type aliases
using NgraphNodePtr = std::shared_ptr<ngraph::Node>;
using nnvmNodePtr = std::shared_ptr<nnvm::Node>;

// Possible Types of nodes in Current Version
enum class NodeType { kVariable, kAux, kOp, kGraph };
enum class GraphExeMode { kInfer = 0, kTrain };
constexpr int kGraphExeModeCount = static_cast<int>(GraphExeMode::kTrain) -
                                   static_cast<int>(GraphExeMode::kInfer) + 1;

// Base class for Nodes in Intermediary Analysis Graph
class Node {
 protected:
  Node(NodeType type, nnvmNodePtr node, const std::string &name)
      : type_(type),
        orig_node_(node),
        name_(name == "" ? randomString(6) : name) {}
  Node(NodeType type, nnvmNodePtr node, const std::string &name,
       const std::vector<NodePtr> &inputs)
      : type_(type),
        orig_node_(node),
        name_(name == "" ? randomString(6) : name),
        inputs_(inputs) {}

 public:
  // Function to create node label, used to export graph to graphviz for debug
  virtual std::string createNodeLabel() {
    std::ostringstream stream;
    stream << name_ << this << " [label = \"" << name_ << this << shape_
           << " \n sg=" << subgraph_ << "\"];";
    return stream.str();
  }
  // basic information about node
  const NodeType type_;
  const nnvmNodePtr orig_node_;
  const std::string name_;
  std::vector<NodePtr> inputs_;

  // mxnet type information
  nnvm::TShape shape_;
  int dtype_ = 0;

  // information to store graph parsing in
  size_t multi_output_index_ = 0;
  bool in_ngraph_ = false;
  std::string operation_ = "";
  int subgraph_ = 0;
};

// Class to store Variables
// Effectivly just a wrapper for setting Node Type
class VariableNode : public Node {
 public:
  // Overloaded constructors for ease of use
  VariableNode(nnvmNodePtr node, const std::string &name)
      : Node(NodeType::kVariable, node, name) {}
  VariableNode(nnvmNodePtr node, const std::string &name,
               const std::vector<NodePtr> &inputs)
      : Node(NodeType::kVariable, node, name, inputs) {}
};

// Class to store Auxillary Tensors, mostly for Batch Norm
// Effectivly just a wrapper for setting Node Type
class AuxNode : public Node {
 public:
  // Overloaded constructors for ease of use
  AuxNode(nnvmNodePtr node, const std::string &name)
      : Node(NodeType::kAux, node, name) {}
  AuxNode(nnvmNodePtr node, const std::string &name,
          const std::vector<NodePtr> &inputs)
      : Node(NodeType::kAux, node, name, inputs) {}
};

// Node for storing operations
class OpNode : public Node {
 public:
  // Include operation in graphviz
  std::string createNodeLabel() override {
    std::ostringstream stream;
    stream << name_ << this << " [label=\"" << name_ << this
           << "\nOp: " << operation_ << shape_ << " sg=" << subgraph_ << "\"";
    auto out = stream.str();
    if (in_ngraph_) out += ", fillcolor = red, style = filled";
    out += "];";
    return out;
  }

  // Overloaded constructors for ease of use
  OpNode(nnvmNodePtr node, const std::string &name,
         const std::string &operation)
      : Node(NodeType::kOp, node, name) {
    operation_ = operation;
  }
  OpNode(nnvmNodePtr node, const std::string &name,
         const std::string &operation, const std::vector<NodePtr> &inputs)
      : Node(NodeType::kOp, node, name, inputs) {
    operation_ = operation;
  }
};

// makes sure you have only one manager of one type

static std::unordered_map<std::string,
                          std::shared_ptr<ngraph::runtime::Manager>>
    backend_managers;

static std::unordered_map<std::string,
                          std::shared_ptr<ngraph::runtime::Backend>>
    backends;

inline std::string get_backend_name(const mxnet::Context &context) {
  if (context == mxnet::Context::NNP()) {
    return "ARGON";
    // } else if (context == mxnet::Context::GPU()) {
    //   return "GPU";
  } else if (context == mxnet::Context::CPU()) {
    return "CPU";
  } else {
    return "INTERPRETER";
  }
}

inline std::shared_ptr<ngraph::runtime::Manager> GetManagerFromContext(
    const mxnet::Context &context) {
  auto backend_name = get_backend_name(context);
  if (backend_managers.count(backend_name) == 0) {
    auto manager = ngraph::runtime::Manager::get(backend_name);
    backend_managers[backend_name] = manager;
  }
  return backend_managers[backend_name];
}

inline std::shared_ptr<ngraph::runtime::Backend> GetBackendFromContext(
    const mxnet::Context &context) {
  auto backend_name = get_backend_name(context);
  if (backend_managers.count(backend_name) == 0) GetManagerFromContext(context);

  if (backends.count(backend_name) == 0) {
    auto backend = backend_managers[backend_name]->allocate_backend();
    backends[backend_name] = backend;
  }
  return backends[backend_name];
}

/*
Graph class
Graph subclasses Node so that we can embed graphs into other graphs
This is useful when we take a graph and replace it with an ngraph computation
TODO: Refactor into Graph and subgraph?
*/
class Graph : public Node {
 public:
  // Graph with optional fprop cache
  Graph(const std::string &name = "",
        const mxnet::Context &context = mxnet::Context::CPU(),
        const bool enable_fprop_cache = true)
      : Node(NodeType::kGraph, nullptr, name),
        context_(context),
        enable_fprop_cache(enable_fprop_cache) {}
  // Delete the ngraph objects so we don't have a large memory leak
  // when running multiple graphs back to back
  void CleanUp() {
    for (int i = 0; i < kGraphExeModeCount; ++i) {
      cached_values[i].clear();
      cached_aux_values[i].clear();
      cached_aux_positions[i].clear();

      ngraph_forward[i] = nullptr;
      ngraph_backward[i] = nullptr;
    }
  }

  // Add a node to the graph
  void AddNode(NodePtr node) { nodes_.emplace_back(node); }

  // get the node corresponding to an orig_node
  NodePtr operator[](const nnvm::NodeEntry &entry) {
    for (auto n : nodes_)
      if ((n->orig_node_ == entry.node) &&
          (n->multi_output_index_ == entry.index)) {
        return n;
      }
    return nullptr;
  }

  bool forward_train_computed{false};
  int num_outputs = 1;
  // nodes in this graph
  std::vector<NodePtr> nodes_;
  // functions to execute this graph in ngraph
  std::shared_ptr<ngraph::runtime::CallFrame>
      ngraph_forward[kGraphExeModeCount];
  std::shared_ptr<ngraph::runtime::CallFrame>
      ngraph_backward[kGraphExeModeCount];

  const mxnet::Context context_;
  std::vector<std::shared_ptr<ngraph::runtime::TensorView>>
      cached_values[kGraphExeModeCount];
  std::vector<std::shared_ptr<ngraph::runtime::TensorView>>
      cached_aux_values[kGraphExeModeCount];

  std::vector<int> cached_aux_positions[kGraphExeModeCount];

  const bool enable_fprop_cache;
};

/**
 * High level function that does the subgraph identification
 */
void IdentifySubgraphs(const Graph &graph,
                       const std::function<bool(NodePtr)> &func);

/**
 * Convert graph from identified nodes to a network of nodes and graphs,
 * each graph node represented a combined ngraph operation
 */
void CollapseSubgraphs(Graph *graph);

/**
 * Selection of nodes based on function criterion.
 * Note: uses DFSUtil().
 */
std::vector<NodePtr> SelectNodes(NodePtr node,
                                 const std::function<bool(NodePtr)> &func);

/**
 * Finds simply connected ngraph operations
 */
std::vector<NodePtr> FindSubgraph(const Graph &graph, NodePtr node,
                                  const std::function<bool(NodePtr)> &func);

// Struct containing functors used as a utility for traversing a graph
struct GraphVisitor {
  std::function<void(NodePtr)> operation;
  std::function<bool(NodePtr, NodePtr)> stop_condition;
  std::function<std::vector<NodePtr>(NodePtr)> get_inputs = [](NodePtr n) {
    return n->inputs_;
  };
};

// Perform a DFS graph traversal non-recursively but always ensuring
// that the inputs to a node are operated on before the node. It also checks
// for graph cycles and throws an error if they are found.
void GraphTraverse(NodePtr node, const GraphVisitor &visitor);

}  // namespace ngraph_bridge

#endif  // MXNET_NGRAPH_NGRAPH_GRAPH_H_