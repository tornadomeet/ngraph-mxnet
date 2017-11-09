// ----------------------------------------------------------------------------
// Copyright 2017 Nervana Systems Inc.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// ----------------------------------------------------------------------------

#include <functional>
#include <stack>

#include "ngraph_graph.h"
#include "reverse_iterate.h"

namespace ngraph_bridge {
// Type Aliases
using OpNodePtr = std::shared_ptr<OpNode>;

/**
 * Function to identify and label connected ngraph ops as subgraphs
 * @param func
 */
void NgraphBuilder::IdentifySubgraphs(std::function<bool(NodePtr)> func) {
  int sg = 1;
  // loop over the nodes from the back
  for (auto i : reverse_iterate(graph_->GetNodes())) {
    if (i->subgraph_ == 0) {
      // select nodes in the a subgraph starting here and going up the graph
      auto subgraph_nodes = FindSubgraph(i, func);
      // if we found a significantly large subgraph, label it
      if (subgraph_nodes.size() > 2) {
        for (auto node : subgraph_nodes) node->subgraph_ = sg;
        for (auto node : subgraph_nodes)
          for (auto i : node->inputs_)
            if (i->subgraph_ != sg) i->subgraph_ = -1;
        sg += 1;
      }
    }
  }
}

/**
 * Function to collapse the intermediary graph into a graph
 * with subgraphs for nodes
 */
void NgraphBuilder::CollapseSubgraphs() {
  // loop variable for undefined number of subgraphs
  int i = 1;
  auto &nodes = graph_->GetNodes();
  while (true) {
    auto tmpGraph = std::make_shared<Graph>("subgraph_" + std::to_string(i));
    // loop over all nodes and add nodes in the current subgraph to
    for (auto node : nodes)
      if (node->subgraph_ == i) tmpGraph->AddNode(node);
    auto &tmp_nodes = tmpGraph->GetNodes();
    if (tmp_nodes.size() == 0) {
      // if we don't find any nodes, assume we've run out of subgraphs
      break;
    } else {
      // if we found nodes, setup subgraph
      tmpGraph->in_ngraph_ = true;
      tmpGraph->subgraph_ = i;
      // set node name and shape based on last node in the subgraph
      auto name = tmp_nodes.back()->name_;
      auto shape = tmp_nodes.back()->shape_;
      tmpGraph->shape_ = shape;
      tmpGraph->dtype_ = tmp_nodes.back()->dtype_;
      auto in_tmpGraphInputs = [&tmpGraph](NodePtr n) {
        if (!in_vec(tmpGraph->inputs_, n)) return false;
        return true;
      };
      // setup inputs to this subgraph (as a node)
      for (auto node : tmp_nodes) {
        for (auto input : node->inputs_) {
          if (input->subgraph_ != i && !in_tmpGraphInputs(input))
            tmpGraph->inputs_.emplace_back(input);
        }
      }
      // set subgraph as input to all of the nodes downline.
      for (auto n : nodes)
        for (size_t i = 0; i < n->inputs_.size(); ++i)
          if (n->inputs_[i]->name_ == name) n->inputs_[i] = tmpGraph;

      // find the position we're replacing in the graph
      auto it = std::find_if(
          nodes.begin(), nodes.end(),
          [name](NodePtr n) -> bool { return (n->name_ == name); });
      // insert the subgraph as a node
      nodes.insert(it, tmpGraph);
    }
    i += 1;
  }

  // delete all the nodes we're replacing with the subgraph
  nodes.erase(std::remove_if(nodes.begin(), nodes.end(),
                             [](NodePtr n) -> bool {
                               return ((n->subgraph_ > 0) &&
                                   (n->type_ == NodeType::kOp));
                             }),
              nodes.end());
}

/**
 * Find a subgraph, check it for bad branches
 * @param s
 * @param func
 * @return
 */
std::vector<NodePtr> NgraphBuilder::FindSubgraph(NodePtr s,
                                                std::function<bool(NodePtr)> func) {
  auto subgraph_nodes = SelectNodes(s, func);
  std::vector<NodePtr> outNodes;
  outNodes = subgraph_nodes;
  if (subgraph_nodes.size() > 2) {
    // search for broken loops
    // remove nodes on broken loops
    outNodes = RemoveBroken(s, outNodes, func);
    outNodes = PruneSubgraphOutputs(s, outNodes, func);
  }
  return outNodes;
}

std::vector<NodePtr> NgraphBuilder::SelectNodes(NodePtr s,
                                               std::function<bool(NodePtr)> func) {
  // init visited vector
  std::unordered_set<NodePtr> visited;
  // init output vector
  std::vector<NodePtr> outNodes;
  // recursiveliy search the graph
  DFSUtil(s, visited, outNodes, func);
  return outNodes;
}

void NgraphBuilder::DFSUtil(NodePtr s,
                           std::unordered_set<NodePtr> &visited,
                           std::vector<NodePtr> &outNodes,
                           std::function<bool(NodePtr)> &func) {
  // Mark the current node as visited
  visited.insert(s);
  // if this node matches func condition
  if (func(s)) {
    // add it to the output
    outNodes.push_back(s);
    // visit it's inputs
    for (auto i : s->inputs_) {
      if (!visited.count(i) && i->subgraph_ == 0) {
        DFSUtil(i, visited, outNodes, func);
      }
    }
  }
}

/**
 * Utility for removing bad branches in a directed, acylic subraph.
 * Will fail for cyclic graphs
 * @param s
 * @param outNodes
 * @param func
 * @param visited_edges
 */
void NgraphBuilder::RemoveUtil(NodePtr s,
                              std::vector<NodePtr> &outNodes,
                              std::function<bool(NodePtr)> func,
                              std::set<edgeRemoveTup> &visited_edges) {
  // if this node doesn't match the function condition, delete it
  if (!func(s))
    outNodes.erase(std::remove(outNodes.begin(), outNodes.end(), s),
                   outNodes.end());

  // visit it's inputs if they're still in the subgraph
  for (auto i : s->inputs_)
    if (in_vec(outNodes, i)) {
      // ask if we've already gone up this branch in this closure state.
      // if so, don't revisit, if not, try it both good and bad.
      auto edge_tup = edgeRemoveTup{s, i, func(s)};
      if (!visited_edges.count(edge_tup)) {
        visited_edges.insert(edge_tup);
        RemoveUtil(i, outNodes, func, visited_edges);
      }
    }
}

/**
 * This function searches for non-local issues that make parts of an
 * ngraph identified subgraph non-computable
 * @param s
 * @param subgraph_nodes
 * @param func
 * @return
 */
std::vector<NodePtr> NgraphBuilder::RemoveBroken(NodePtr s,
                                                std::vector<NodePtr> &subgraph_nodes,
                                                std::function<bool(NodePtr)> func) {
  // create storage for the ouputs and the visited nodes
  std::vector<NodePtr> outNodes;
  std::unordered_set<NodePtr> visited;
  std::set<edgeRemoveTup> visited_edges;

  // This function searches the nodes that are inputs to the final
  // subgraph output AND outputs of other subgraph nodes
  // to minimize what needs to be searched for broken loops
  std::function<bool(NodePtr)> get_nodes;
  get_nodes = [&outNodes, &visited, &get_nodes, &func](NodePtr s) {
    visited.insert(s);
    bool im_an_output = false;
    if (func(s)) im_an_output = true;

    for (auto i : s->inputs_) {
      if (!in_vec(outNodes, i)) {
        if (!visited.count(i))
          if (get_nodes(i)) im_an_output = true;
      } else {
        im_an_output = true;
      }
    }

    if (im_an_output) outNodes.push_back(s);
    return im_an_output;
  };

  get_nodes(s);

  // This is a mutable closure, copied on each step up the graph,
  // that tells us weather or not this branch of the graph is good or bad
  bool found_bad = false;
  auto good_subgraph_node = [subgraph_nodes, func,
      found_bad](NodePtr s) mutable {
    if (!func(s)) found_bad = true;
    if (found_bad) return false;
    if (in_vec(subgraph_nodes, s)) {
      return true;
    } else {
      return false;
    }
  };

  // recursive search for bad branches
  RemoveUtil(s, outNodes, good_subgraph_node, visited_edges);
  return outNodes;
}

// This removes mutiple outputs from a graph, because the subgraph compiler
// doesn't currently support multiple outputs
// TODO: make the subgraph compiler handle multiple outputs and get rid of this
// graph pass
std::vector<NodePtr> NgraphBuilder::PruneSubgraphOutputs(NodePtr s,
                                                        std::vector<NodePtr> &subgraph_nodes,
                                                        std::function<bool(
                                                            NodePtr)> func) {
  // function to get all the outputs of the subgraph
  auto get_subgraph_outputs = [this, &subgraph_nodes]() {
    std::vector<NodePtr> outNodes;
    for (auto n : graph_->GetNodes())
      if (!in_vec(subgraph_nodes, n))
        for (auto i : n->inputs_)
          if (in_vec(subgraph_nodes, i) && !in_vec(outNodes, i))
            outNodes.emplace_back(i);
    return outNodes;
  };

  // function to remove all of the outputs that aren't the last one
  auto prune_subgraph = [&subgraph_nodes](std::vector<NodePtr> outNodes) {
    for (auto n : outNodes)
      if (n != subgraph_nodes[0])
        subgraph_nodes.erase(
            std::remove(subgraph_nodes.begin(), subgraph_nodes.end(), n),
            subgraph_nodes.end());
  };

  // main pass
  // count is for debugging purposes in case the recursive logic is broken
  std::vector<NodePtr> outNodes;
  bool single_output = false;
  int count = 0;
  while (!single_output && count < 100) {
    // get the current outputs
    outNodes = get_subgraph_outputs();
    if (outNodes.size() <= 1) {
      single_output = true;
    } else {
      // we have more than 1 output, remove them and clean any broken loops
      prune_subgraph(outNodes);
      subgraph_nodes = RemoveBroken(s, subgraph_nodes, func);
    }
    count += 1;
  }

  return subgraph_nodes;
}

}  // namespace ngraph_bridge
