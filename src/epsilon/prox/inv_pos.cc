#include <float.h>

#include "epsilon/affine/affine.h"
#include "epsilon/expression/expression_util.h"
#include "epsilon/prox/prox.h"
#include "epsilon/prox/newton.h"
#include "epsilon/vector/dynamic_matrix.h"
#include "epsilon/vector/vector_util.h"
#include "epsilon/prox/ortho_invariant.h"
#include <cmath>

// \sum_i 1/(xi)
class InvPos final : public SmoothFunction {
public:
  double eval(const Eigen::VectorXd &x) const override {
    int n = x.rows();
    double sum = 0;
    for(int i=0; i<n; i++){
      sum += 1/x(i);
    }
    return sum;
  }
  Eigen::VectorXd gradf(const Eigen::VectorXd &x) const override {
    int n = x.rows();
    Eigen::VectorXd g(n);
    for(int i=0; i<n; i++)
      g(i) = -1/(x(i)*x(i));
    return g;
  }
  Eigen::VectorXd hessf(const Eigen::VectorXd &x) const override {
    int n = x.rows();
    Eigen::VectorXd h(n);
    for(int i=0; i<n; i++)
      h(i) = 2/(x(i)*x(i)*x(i));
    return h;
  }
  Eigen::VectorXd proj_feasible(const Eigen::VectorXd& x) const override {
    return x.cwiseMax(1e-6);
  }
};

class InvPosProx : public NewtonProx {
public:
  InvPosProx() : NewtonProx(std::make_unique<InvPos>()) {}
};
REGISTER_PROX_OPERATOR(InvPosProx);

class InvPosEpigraph : public NewtonEpigraph {
public:
  InvPosEpigraph() : NewtonEpigraph(std::make_unique<InvPos>()) {}
};
REGISTER_PROX_OPERATOR(InvPosEpigraph);