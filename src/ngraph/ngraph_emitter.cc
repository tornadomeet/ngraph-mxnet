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

#include "ngraph_emitter.h"
#include "ngraph_sgcompiler_utils.h"

namespace ngraph_bridge {

// Compiter initialization
Emitter::Emitter() {
  // Create Operation Maps
  CreateUnaryOps();
  CreateBinaryOps();
  CreateLayerOps();
}

// unary op genrating function generator
void Emitter::CreateUnaryOps() {
  ngraph_op_funcs_["relu"] = [this](const NodePtr& node) {
    auto zero = makeConstant(node, "0");
    return std::make_shared<ngraph::op::Maximum>(op_map_[node->inputs_[0]], zero);
  };
  ngraph_op_funcs_["sigmoid"] = [this](const NodePtr& node) {
    auto one = makeConstant(node, "1");
    return (one / (one + std::make_shared<ngraph::op::Exp>(
                             -op_map_[node->inputs_[0]])));
  };
  // ngraph_op_funcs_["softmax"] = [this](const NodePtr& node) {
  //   auto numer = std::make_shared<ngraph::op::Exp>(op_map_[node->inputs_[0]]);
  //   auto denom = std::make_shared<ngraph::op::Sum>(numer, ngraph::AxisSet{1});
  //   return ;
  // };
  // ngraph_op_funcs_["log_softmax"] = [this](const NodePtr& node){
  //   return ;
  // };
  ngraph_op_funcs_["_copy"] = [this](const NodePtr& node) {
    return op_map_[node->inputs_[0]]; //TODO: Return this as a reference. Does it actually need to be copied?
  };
  ngraph_op_funcs_["negative"] = [this](const NodePtr& node) {
    return -op_map_[node->inputs_[0]];
  };
  ngraph_op_funcs_["reciprocal"] = [this](const NodePtr& node){
    auto one = makeConstant(node, "1");
    return one / op_map_[node->inputs_[0]];
  };
  ngraph_op_funcs_["abs"] = [this](const NodePtr& node) {
    return std::make_shared<ngraph::op::Abs>(op_map_[node->inputs_[0]]);
  };
  // ngraph_op_funcs_["sign"] = [this](const NodePtr& node){
  //   return ;
  // };
  // ngraph_op_funcs_["round"] = [this](const NodePtr& node){
  //   return ;
  // };
  // ngraph_op_funcs_["rint"] = [this](const NodePtr& node){
  //   return ;
  // };
  ngraph_op_funcs_["ceil"] = [this](const NodePtr& node) {
    return std::make_shared<ngraph::op::Ceiling>(op_map_[node->inputs_[0]]);
  };
  ngraph_op_funcs_["floor"] = [this](const NodePtr& node) {
    return std::make_shared<ngraph::op::Floor>(op_map_[node->inputs_[0]]);
  };
  // ngraph_op_funcs_["trunc"] = [this](const NodePtr& node){
  //   return ;
  // };
  // ngraph_op_funcs_["fix"] = [this](const NodePtr& node){
  //   return ;
  // };
  ngraph_op_funcs_["square"] = [this](const NodePtr& node) {
    auto two = makeConstant(node, "2");
    return std::make_shared<ngraph::op::Power>(op_map_[node->inputs_[0]], two);
  };
  ngraph_op_funcs_["sqrt"] = [this](const NodePtr& node) {
    auto one = makeConstant(node, "1");
    auto two = makeConstant(node, "2");
    return std::make_shared<ngraph::op::Power>(op_map_[node->inputs_[0]],
                                               one / two);
  };
  ngraph_op_funcs_["rsqrt"] = [this](const NodePtr& node) {
    auto one = makeConstant(node, "1");
    auto two = makeConstant(node, "2");
    return one / std::make_shared<ngraph::op::Power>(op_map_[node->inputs_[0]],
                                                     one / two);
  };
  ngraph_op_funcs_["cbrt"] = [this](const NodePtr& node) {
    auto one = makeConstant(node, "1");
    auto three = makeConstant(node, "3");
    return std::make_shared<ngraph::op::Power>(op_map_[node->inputs_[0]],
                                               one / three);
  };
  ngraph_op_funcs_["rcbrt"] = [this](const NodePtr& node) {
    auto one = makeConstant(node, "1");
    auto three = makeConstant(node, "3");
    return one / std::make_shared<ngraph::op::Power>(op_map_[node->inputs_[0]],
                                                     one / three);
  };
  ngraph_op_funcs_["exp"] = [this](const NodePtr& node) {
    return std::make_shared<ngraph::op::Exp>(op_map_[node->inputs_[0]]);
  };
  ngraph_op_funcs_["log"] = [this](const NodePtr& node) {
    return std::make_shared<ngraph::op::Log>(op_map_[node->inputs_[0]]);
  };
  ngraph_op_funcs_["log10"] = [this](const NodePtr& node){
    auto ten = makeConstant(node, "10");
    return std::make_shared<ngraph::op::Log>(op_map_[node->inputs_[0]]) / 
           std::make_shared<ngraph::op::Log>(ten);
  };
  ngraph_op_funcs_["log2"] = [this](const NodePtr& node){
    auto two = makeConstant(node, "2");
    return std::make_shared<ngraph::op::Log>(op_map_[node->inputs_[0]]) / 
           std::make_shared<ngraph::op::Log>(two);
  };
  // ngraph_op_funcs_["log1p"] = [this](const NodePtr& node){
  //   return ;
  // };
  // ngraph_op_funcs_["expm1"] = [this](const NodePtr& node){
  //   return ;
  // };
  ngraph_op_funcs_["sin"] = [this](const NodePtr& node) {
    return std::make_shared<ngraph::op::Sin>(op_map_[node->inputs_[0]]);
  };
  ngraph_op_funcs_["cos"] = [this](const NodePtr& node) {
    return std::make_shared<ngraph::op::Cos>(op_map_[node->inputs_[0]]);
  };
  ngraph_op_funcs_["tan"] = [this](const NodePtr& node) {
    return std::make_shared<ngraph::op::Tan>(op_map_[node->inputs_[0]]);
  };
  ngraph_op_funcs_["arcsin"] = [this](const NodePtr& node) {
    return std::make_shared<ngraph::op::Asin>(op_map_[node->inputs_[0]]);
  };
  ngraph_op_funcs_["arccos"] = [this](const NodePtr& node) {
    return std::make_shared<ngraph::op::Acos>(op_map_[node->inputs_[0]]);
  };
  ngraph_op_funcs_["arctan"] = [this](const NodePtr& node) {
    return std::make_shared<ngraph::op::Atan>(op_map_[node->inputs_[0]]);
  };
  ngraph_op_funcs_["sinh"] = [this](const NodePtr& node) {
    return std::make_shared<ngraph::op::Sinh>(op_map_[node->inputs_[0]]);
  };
  ngraph_op_funcs_["cosh"] = [this](const NodePtr& node) {
    return std::make_shared<ngraph::op::Cosh>(op_map_[node->inputs_[0]]);
  };
  ngraph_op_funcs_["tanh"] = [this](const NodePtr& node) {
    return std::make_shared<ngraph::op::Tanh>(op_map_[node->inputs_[0]]);
  };
  // ngraph_op_funcs_["arcsinh"] = [this](const NodePtr& node){
  //   return ;
  // };
  // ngraph_op_funcs_["arccosh"] = [this](const NodePtr& node){
  //   return ;
  // };
  // ngraph_op_funcs_["arctanh"] = [this](const NodePtr& node){
  //   return ;
  // };
  ngraph_op_funcs_["degrees"] = [this](const NodePtr& node){
    auto pi = makeConstant(node, "3.14159265359");
    auto oneeighty = makeConstant(node, "180");
    return op_map_[node->inputs_[0]] * (oneeighty / pi);
  };
  ngraph_op_funcs_["radians"] = [this](const NodePtr& node){
    auto pi = makeConstant(node, "3.14159265359");
    auto oneeighty = makeConstant(node, "180");
    return op_map_[node->inputs_[0]] * (pi / oneeighty);
  };
  // ngraph_op_funcs_["gamma"] = [this](const NodePtr& node){
  //   return ;
  // };
  // ngraph_op_funcs_["gammaln"] = [this](const NodePtr& node){
  //   return ;
  // };
}

AutoBroadcast Emitter::CreateAutoBroadcast(const NodePtr& node) {
  auto lhsNode = op_map_[node->inputs_[0]];
  auto lhsShape = TShape_to_NShape(node->inputs_[0]->shape_);
  auto rhsNode = op_map_[node->inputs_[1]];
  auto rhsShape = TShape_to_NShape(node->inputs_[1]->shape_);
  return AutoBroadcast(lhsNode, lhsShape, rhsNode, rhsShape);
}

// binary op generating function generator
void Emitter::CreateBinaryOps() {
  ngraph_op_funcs_["_plus"] = [this](const NodePtr& node) { 
    return (op_map_[node->inputs_[0]] + op_map_[node->inputs_[1]]);
  };
  ngraph_op_funcs_["_minus"] = [this](const NodePtr& node) {
    return (op_map_[node->inputs_[0]] - op_map_[node->inputs_[1]]);
  };
  ngraph_op_funcs_["_mul"] = [this](const NodePtr& node) {
    return (op_map_[node->inputs_[0]] * op_map_[node->inputs_[1]]);
  };
  ngraph_op_funcs_["_div"] = [this](const NodePtr& node) {
    return (op_map_[node->inputs_[0]] / op_map_[node->inputs_[1]]);
  };
  ngraph_op_funcs_["_mod"] = [this](const NodePtr& node) {
    return std::make_shared<ngraph::op::Remainder>(op_map_[node->inputs_[0]],
                                                   op_map_[node->inputs_[1]]);
  };
  ngraph_op_funcs_["_power"] = [this](const NodePtr& node) {
    return std::make_shared<ngraph::op::Power>(op_map_[node->inputs_[0]],
                                               op_map_[node->inputs_[1]]);
  };
  ngraph_op_funcs_["_maximum"] = [this](const NodePtr& node) {
    return std::make_shared<ngraph::op::Maximum>(op_map_[node->inputs_[0]],
                                                 op_map_[node->inputs_[1]]);
  };
  ngraph_op_funcs_["_minimum"] = [this](const NodePtr& node) {
    return std::make_shared<ngraph::op::Minimum>(op_map_[node->inputs_[0]],
                                                 op_map_[node->inputs_[1]]);
  };
  ngraph_op_funcs_["_hypot"] = [this](const NodePtr& node) {
    auto one = makeConstant(node, "1");
    auto two = makeConstant(node, "2");
    return std::make_shared<ngraph::op::Power>(
        (std::make_shared<ngraph::op::Power>(op_map_[node->inputs_[0]], two) +
         std::make_shared<ngraph::op::Power>(op_map_[node->inputs_[1]], two)),
        one / two);
  };
  ngraph_op_funcs_["_equal"] = [this](const NodePtr& node) {
    return std::make_shared<ngraph::op::Equal>(op_map_[node->inputs_[0]],
                                               op_map_[node->inputs_[1]]);
  };
  ngraph_op_funcs_["_not_equal"] = [this](const NodePtr& node) {
    return std::make_shared<ngraph::op::NotEqual>(op_map_[node->inputs_[0]],
                                                  op_map_[node->inputs_[1]]);
  };
  ngraph_op_funcs_["_greater"] = [this](const NodePtr& node) {
    return std::make_shared<ngraph::op::Greater>(op_map_[node->inputs_[0]],
                                                 op_map_[node->inputs_[1]]);
  };
  ngraph_op_funcs_["_greater_equal"] = [this](const NodePtr& node) {
    return std::make_shared<ngraph::op::GreaterEq>(op_map_[node->inputs_[0]],
                                                   op_map_[node->inputs_[1]]);
  };
  ngraph_op_funcs_["_lesser"] = [this](const NodePtr& node) {
    return std::make_shared<ngraph::op::Less>(op_map_[node->inputs_[0]],
                                              op_map_[node->inputs_[1]]);
  };
  ngraph_op_funcs_["_lesser_equal"] = [this](const NodePtr& node) {
    return std::make_shared<ngraph::op::LessEq>(op_map_[node->inputs_[0]],
                                                op_map_[node->inputs_[1]]);
  };
  ngraph_op_funcs_["dot"] = [this](const NodePtr& node) {
    return std::make_shared<ngraph::op::Dot>(op_map_[node->inputs_[0]],
                                             op_map_[node->inputs_[1]]);
  };
  ngraph_op_funcs_["broadcast_add"] = [this](const NodePtr& node) {
    auto ab = CreateAutoBroadcast(node);
    return ab.lhs() + ab.rhs();
  };
  ngraph_op_funcs_["broadcast_sub"] = [this](const NodePtr& node) {
    auto ab = CreateAutoBroadcast(node);
    return ab.lhs() - ab.rhs();
  };
  ngraph_op_funcs_["broadcast_mul"] = [this](const NodePtr& node) {
    auto ab = CreateAutoBroadcast(node);
    return ab.lhs() * ab.rhs();
  };
  ngraph_op_funcs_["broadcast_div"] = [this](const NodePtr& node) {
    auto ab = CreateAutoBroadcast(node);
    return ab.lhs() / ab.rhs();
  };
  ngraph_op_funcs_["broadcast_mod"] = [this](const NodePtr& node) {
    auto ab = CreateAutoBroadcast(node);
    return std::make_shared<ngraph::op::Remainder>(ab.lhs(), ab.rhs());
  };
  ngraph_op_funcs_["broadcast_power"] = [this](const NodePtr& node) {
    auto ab = CreateAutoBroadcast(node);
    return std::make_shared<ngraph::op::Power>(ab.lhs(), ab.rhs());
  };
  ngraph_op_funcs_["broadcast_maximum"] = [this](const NodePtr& node) {
    auto ab = CreateAutoBroadcast(node);
    return std::make_shared<ngraph::op::Maximum>(ab.lhs(), ab.rhs());
  };
  ngraph_op_funcs_["broadcast_minimum"] = [this](const NodePtr& node) {
    auto ab = CreateAutoBroadcast(node);
    return std::make_shared<ngraph::op::Minimum>(ab.lhs(), ab.rhs());
  };
  ngraph_op_funcs_["broadcast_hypot"] = [this](const NodePtr& node) {
    auto ab = CreateAutoBroadcast(node);
    auto one = makeConstant(node, "1");
    auto two = makeConstant(node, "2");
    return std::make_shared<ngraph::op::Power>(
        (std::make_shared<ngraph::op::Power>(ab.lhs(), two) +
         std::make_shared<ngraph::op::Power>(ab.rhs(), two)),
        one / two);
  };
  ngraph_op_funcs_["broadcast_equal"] = [this](const NodePtr& node) {
    auto ab = CreateAutoBroadcast(node);
    return std::make_shared<ngraph::op::Equal>(ab.lhs(), ab.rhs());
  };
  ngraph_op_funcs_["broadcast_not_equal"] = [this](const NodePtr& node) {
    auto ab = CreateAutoBroadcast(node);
    return std::make_shared<ngraph::op::NotEqual>(ab.lhs(), ab.rhs());
  };
  ngraph_op_funcs_["broadcast_greater"] = [this](const NodePtr& node) {
    auto ab = CreateAutoBroadcast(node);
    return std::make_shared<ngraph::op::Greater>(ab.lhs(), ab.rhs());
  };
  ngraph_op_funcs_["broadcast_greater_equal"] = [this](const NodePtr& node) {
    auto ab = CreateAutoBroadcast(node);
    return std::make_shared<ngraph::op::GreaterEq>(ab.lhs(), ab.rhs());
  };
  ngraph_op_funcs_["broadcast_lesser"] = [this](const NodePtr& node) {
    auto ab = CreateAutoBroadcast(node);
    return std::make_shared<ngraph::op::Less>(ab.lhs(), ab.rhs());
  };
  ngraph_op_funcs_["broadcast_lesser_equal"] = [this](const NodePtr& node) {
    auto ab = CreateAutoBroadcast(node);
    return std::make_shared<ngraph::op::LessEq>(ab.lhs(), ab.rhs());
  };
}

inline int get_default(const NodePtr& node, const std::string& key, int default_val) 
{
  return node->orig_node_->attrs.dict.count(key)
             ? std::stoi(node->orig_node_->attrs.dict[key])
             : default_val;
}

// MXNet high level ops generating function
void Emitter::CreateLayerOps() {

  // In mxnet, split takes a tensor and creates multiple tensors from
  // equal slices along 1 axis. The compiler creates a subgraph where 
  // each of those outputs is a single node.  This function creates
  // the slice op for making each tensor.
  ngraph_op_funcs_["split"] = [this](const NodePtr& node) {
    
    size_t axis = get_default(node, "axis", 1);
    int num_outputs = get_default(node, "num_outputs", 1);
    int index = node->multi_output_index_;
    bool squeeze_axis = get_default(node, "squeeze_axis", 0);

    // create lower and upper bounds for slice
    auto upper = TShape_to_NShape(node->inputs_[0]->shape_);
    std::vector<size_t> lower(upper.size(), 0);

    lower[axis] = index * upper[axis] / num_outputs;
    upper[axis] = (index + 1) * upper[axis] / num_outputs;

    // perform the slice
    std::shared_ptr<ngraph::Node> op = std::make_shared<ngraph::op::Slice>(
        op_map_[node->inputs_[0]], lower, upper);

    // remove dimension 1 axis if needed
    if (squeeze_axis && ((upper[axis] - lower[axis]) == 1)) {
      std::vector<size_t> reshape;
      for (size_t i = 0; i < upper.size(); ++i)
        if (i != axis) reshape.push_back(upper[i]);

      // can this be a reshape default?
      ngraph::AxisVector order(upper.size());
      std::iota(order.begin(), order.end(), 0);

      op = std::make_shared<ngraph::op::Reshape>(op, order, reshape);
    }

    return op;
  };

  // concat takes a list of tensors of equal shape and 
  // concatenates them along a given axis
  ngraph_op_funcs_["concat"] = [this](const NodePtr& node) {
    // get the concat axis
    size_t axis = get_default(node, "dim", 1);
    
    // grab in input ngraph nodes
    std::vector<NgraphNodePtr> args;
    for (auto i : node->inputs_) args.push_back(op_map_[i]);

    // run concat
    return std::make_shared<ngraph::op::Concat>(args, axis);
  };
  
  // Fully connected is the main linear transformation layer in MXNet
  // it implements dot(data, W.T) + b
  ngraph_op_funcs_["FullyConnected"] = [this](const NodePtr& node){
    auto X = op_map_[node->inputs_[0]];
    auto W = op_map_[node->inputs_[1]];
    auto beta = op_map_[node->inputs_[2]];
    auto dot = std::make_shared<ngraph::op::Dot>(
        X, NgraphTranspose(W, TShape_to_NShape(node->inputs_[1]->shape_)));

    auto dotShape = TShape_to_NShape(node->shape_);
    auto betaShape = TShape_to_NShape(node->inputs_[2]->shape_);

    auto ab = AutoBroadcast(dot, dotShape, beta, betaShape);
    return ab.lhs() + ab.rhs();
  };

  // flatten converts an array of shape (x0, x1, x2, ...)
  // to an array of shape (x0, x1*x2*...)
  ngraph_op_funcs_["flatten"] = [this](const NodePtr& node) {
    auto in_shape = TShape_to_NShape(node->inputs_[0]->shape_);
    auto out_shape = ngraph::Shape({in_shape[0], 1});
    out_shape[1] = std::accumulate(in_shape.begin() + 1, in_shape.end(), 1,
                                   std::multiplies<int>());
    // Create a range vector indicating that 
    // Reshape should take the axes in order
    // these two lines are use all over the place
    ngraph::AxisVector order(in_shape.size());
    std::iota(order.begin(), order.end(), 0);

    return std::make_shared<ngraph::op::Reshape>(op_map_[node->inputs_[0]],
                                                 order, out_shape);
  };

  // Implement transpose with a utility function that returns
  // a reshape op. Not ideal, we should have a ngraph transpose op
  ngraph_op_funcs_["transpose"] = [this](const NodePtr& node) {
    return NgraphTranspose(op_map_[node->inputs_[0]],
                           TShape_to_NShape(node->inputs_[0]->shape_));
  };

  // expand dims inserts an axis of length 1 somewhere in the tensor shape
  ngraph_op_funcs_["expand_dims"] = [this](const NodePtr& node) {
    size_t axis = get_default(node, "axis", 1);
    
    auto in_shape = TShape_to_NShape(node->inputs_[0]->shape_);

    // Create a range vector indicating that 
    // Reshape should take the axes in order
    ngraph::AxisVector order(in_shape.size());
    std::iota(order.begin(), order.end(), 0);

    // copy the shape and insert a 1 at the axis position to expand the dimension
    auto out_shape = in_shape;
    out_shape.insert(out_shape.begin() + axis, 1);

    return std::make_shared<ngraph::op::Reshape>(op_map_[node->inputs_[0]],
                                                 order, out_shape);
  };
  
}

}  // end namespace ngraph

