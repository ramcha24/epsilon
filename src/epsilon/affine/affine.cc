#include "epsilon/affine/affine.h"

#include <memory>
#include <mutex>

#include <glog/logging.h>

#include <Eigen/SparseCore>

#include "epsilon/expression.pb.h"
#include "epsilon/expression/expression_util.h"
#include "epsilon/linear/dense_matrix_impl.h"
#include "epsilon/linear/diagonal_matrix_impl.h"
#include "epsilon/linear/kronecker_product_impl.h"
#include "epsilon/linear/scalar_matrix_impl.h"
#include "epsilon/linear/sparse_matrix_impl.h"
#include "epsilon/util/string.h"
#include "epsilon/vector/vector_util.h"

namespace affine {

void BuildAffineOperatorImpl(
    const Expression& expr,
    const DataMap& data_map,
    const std::string& row_key,
    linear_map::LinearMap L,
    BlockMatrix* A,
    BlockVector* b);

void Add(
    const Expression& expr,
    const DataMap& data_map,
    const std::string& row_key,
    linear_map::LinearMap L,
    BlockMatrix* A,
    BlockVector* b) {
  for (const Expression& arg : expr.arg())
    BuildAffineOperatorImpl(arg, data_map, row_key, L, A, b);
}

void Variable(
    const Expression& expr,
    const DataMap& data_map,
    const std::string& row_key,
    linear_map::LinearMap L,
    BlockMatrix* A,
    BlockVector* b) {
  A->InsertOrAdd(row_key, expr.variable().variable_id(), L);
}

void Constant(
    const Expression& expr,
    const DataMap& data_map,
    const std::string& row_key,
    linear_map::LinearMap L,
    BlockMatrix* A,
    BlockVector* b) {
  Eigen::VectorXd b_dense;
  const ::Constant& c = expr.constant();
  if (c.data_location() == "") {
    // TODO(mwytock): This should probably be made explicit
    // Handle promotion if necessary by using L
    b_dense = Eigen::VectorXd::Constant(L.impl().n(), c.scalar());
  } else {
    b_dense = ToVector(BuildMatrix(c, data_map));
  }

  b->InsertOrAdd(row_key, L*b_dense);
}

void LinearMap(
    const Expression& expr,
    const DataMap& data_map,
    const std::string& row_key,
    linear_map::LinearMap L,
    BlockMatrix* A,
    BlockVector* b) {
  BuildAffineOperatorImpl(
      GetOnlyArg(expr),
      data_map,
      row_key,
      L*linear_map::BuildLinearMap(expr.linear_map(), data_map),
      A, b);
}

typedef void(*LinearFunction)(
    const Expression&,
    const DataMap& data_map,
    const std::string& row_key,
    linear_map::LinearMap L,
    BlockMatrix* A,
    BlockVector* b);

std::unordered_map<int, LinearFunction> kLinearFunctions = {
  {Expression::ADD, &Add},
  {Expression::CONSTANT, &Constant},
  {Expression::LINEAR_MAP, &LinearMap},
  {Expression::RESHAPE, &Add},  // No-op
  {Expression::VARIABLE, &Variable},
};

void BuildAffineOperatorImpl(
    const Expression& expr,
    const DataMap& data_map,
    const std::string& row_key,
    linear_map::LinearMap L,
    BlockMatrix* A,
    BlockVector* b) {
  VLOG(3) << "BuildAffineOperatorImpl\n"
          << "L: " << L.impl().DebugString() << "\n"
          << expr.DebugString();

  auto iter = kLinearFunctions.find(expr.expression_type());
  if (iter == kLinearFunctions.end()) {
    LOG(FATAL) << "No linear function for "
               << Expression::Type_Name(expr.expression_type());
  }
  iter->second(expr, data_map, row_key, L, A, b);
}

void BuildAffineOperator(
    const Expression& expr,
    const DataMap& data_map,
    const std::string& row_key,
    BlockMatrix* A,
    BlockVector* b) {
  BuildAffineOperatorImpl(
      expr, data_map, row_key, linear_map::Identity(GetDimension(expr)), A, b);
}

const std::string kConstraintPrefix = "constraint:";
const std::string kArgPrefix = "arg:";

std::string constraint_key(int i) {
  return kConstraintPrefix + std::to_string(i);
}

std::string arg_key(int i) {
  return kArgPrefix + std::to_string(i);
}


}  // affine
