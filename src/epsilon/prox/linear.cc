#include "epsilon/affine/affine.h"
#include "epsilon/expression/expression_util.h"
#include "epsilon/prox/prox.h"

// c'x
class LinearProx final : public ProxOperator {
  void Init(const ProxOperatorArg& arg) override {
    BlockMatrix A;
    BlockVector b;
    affine::BuildAffineOperator(arg.f_expr(), "_", &A, &b);
    CHECK_EQ(1, A.col_keys().size());
    c_ = arg.lambda()*A("_", *A.col_keys().begin()).impl().AsDense();
  }

  Eigen::VectorXd Apply(const Eigen::VectorXd& v) override {
    return v - c_;
  }

private:
  Eigen::VectorXd c_;
};
REGISTER_PROX_OPERATOR(LinearProx);
