#include "epsilon/affine/affine_matrix.h"
#include <unordered_map>

#include <glog/logging.h>

#include "epsilon/expression.pb.h"
#include "epsilon/expression/expression_util.h"
#include "epsilon/vector/vector_file.h"
#include "epsilon/vector/vector_util.h"

// Convenice functions to handle zeros
#define ADD(A, B) (!(A).rows() ? (B) : (!(B).rows() ? (A) : (A)+(B)))
#define MULTIPLY(A, B) (!(A).rows() ? (A) : (!(B).rows() ? (B) : (A)*(B)))
#define MULTIPLY_SCALAR(a, B) (                                 \
    (!(B).rows() ? (B) : static_cast<Eigen::MatrixXd>(a*B)))


namespace affine {

namespace {

Eigen::MatrixXd ReadConstant(const Expression& expr) {
  const int m = GetDimension(expr, 0);
  const int n = GetDimension(expr, 1);
  const Constant& c = expr.constant();
  if (c.data_location() == "")
    return Eigen::MatrixXd::Constant(m, n, c.scalar());

  VLOG(1) << "Read: " << c.data_location();
  std::unique_ptr<const Data> d = ReadSplitData(c.data_location());
  VLOG(1) << "Read done: " << c.data_location();
  return GetMatrixData(*d);
}

MatrixOperator Add(const Expression& expr) {
  CHECK(expr.arg_size() != 0);

  MatrixOperator op = BuildMatrixOperator(expr.arg(0));
  for (int i = 1; i < expr.arg_size(); i++) {
    MatrixOperator op_i = BuildMatrixOperator(expr.arg(i));
    op.A = ADD(op.A, op_i.A);
    op.B = ADD(op.B, op_i.B);
    op.C = ADD(op.C, op_i.C);
  }
  return op;
}

MatrixOperator Multiply(const Expression& expr) {
  // NOTE(mwytock): Assumes LHS is a constant (this is assumed in CVXPY) but
  // this could be relaxed.
  CHECK_EQ(2, expr.arg_size());
  CHECK_EQ(GetDimension(expr, 0), GetDimension(expr.arg(0), 0));
  CHECK_EQ(GetDimension(expr, 1), GetDimension(expr.arg(1), 1));
  CHECK_EQ(GetDimension(expr.arg(0), 1), GetDimension(expr.arg(1), 0));

  MatrixOperator lhs = BuildMatrixOperator(expr.arg(0));
  MatrixOperator rhs = BuildMatrixOperator(expr.arg(1));
  CHECK(lhs.A.isZero());
  CHECK(lhs.B.isZero());
  rhs.A = MULTIPLY(lhs.C, rhs.A);
  rhs.C = MULTIPLY(lhs.C, rhs.C);
  return rhs;
}

MatrixOperator Negate(const Expression& expr) {
  CHECK_EQ(1, expr.arg_size());
  MatrixOperator op = BuildMatrixOperator(expr.arg(0));
  op.A = MULTIPLY_SCALAR(-1, op.A);
  op.C = MULTIPLY_SCALAR(-1, op.C);
  return op;
}

MatrixOperator Variable(const Expression& expr) {
  const int m = GetDimension(expr, 0);
  const int n = GetDimension(expr, 1);
  MatrixOperator op;
  op.A = Eigen::MatrixXd::Identity(m, m);
  op.B = Eigen::MatrixXd::Identity(n, n);
  return op;
}

MatrixOperator Constant(const Expression& expr) {
  MatrixOperator op;
  op.C = ReadConstant(expr);
  return op;
}

}  // namespace

typedef MatrixOperator(*AffineMatrixFunction)(
    const Expression& expr);
std::unordered_map<int, AffineMatrixFunction> kAffineMatrixFunctions = {
  {Expression::ADD, &Add},
  {Expression::MULTIPLY, &Multiply},
  {Expression::NEGATE, &Negate},
  {Expression::VARIABLE, &Variable},
  {Expression::CONSTANT, &Constant},
};

MatrixOperator BuildMatrixOperator(const Expression& expr) {
  VLOG(2) << "BuildMatrixOperator\n" << expr.DebugString();
  auto iter = kAffineMatrixFunctions.find(expr.expression_type());
  if (iter == kAffineMatrixFunctions.end()) {
    LOG(FATAL) << "No affine matrix function for "
               << Expression::Type_Name(expr.expression_type());
  }
  return iter->second(expr);
}

}  // namespace affine
