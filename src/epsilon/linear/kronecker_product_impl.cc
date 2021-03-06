#include "epsilon/linear/kronecker_product_impl.h"
#include "epsilon/vector/vector_util.h"
#include "epsilon/util/time.h"

namespace linear_map {

LinearMap::DenseMatrix KroneckerProductImpl::AsDense() const {
  VLOG(1) << "Converting kron to dense (" << m() << " x " << n() << ")";

  DenseMatrix A = A_.impl().AsDense();
  DenseMatrix B = B_.impl().AsDense();
  DenseMatrix C(m(), n());

  for (int i = 0; i < A.rows(); i++) {
    for (int j = 0; j < A.cols(); j++) {
      C.block(i*B.rows(), j*B.cols(), B.rows(), B.cols()) = A(i,j)*B;
    }
  }

  return C;
}

LinearMap::SparseMatrix KroneckerProductImpl::AsSparse() const {
  VLOG(1) << "Converting kron to sparse (" << m() << " x " << n() << ")";

  DenseMatrix A = A_.impl().AsDense();
  DenseMatrix B = B_.impl().AsDense();
  SparseMatrix C(m(), n());

  {
    std::vector<Eigen::Triplet<double> > coeffs;
    for (int i = 0; i < A.rows(); i++) {
      for (int j = 0; j < A.cols(); j++) {
        if (A(i,j) == 0)
          continue;
        AppendBlockTriplets(A(i,j)*B, i*B.rows(), j*B.cols(), &coeffs);
      }
    }
    C.setFromTriplets(coeffs.begin(), coeffs.end());
  }

  return C;
}

LinearMapImpl::DenseVector KroneckerProductImpl::Apply(
    const LinearMapImpl::DenseVector& x) const {
  const int m = B_.impl().n();
  const int n = A_.impl().n();

  // Have to make copy to get DenseMatrixImpl
  // TODO(mwytock): fix this
  std::shared_ptr<DenseMatrixImpl::Data> data_ptr(new DenseMatrixImpl::Data);
  data_ptr->data.reset(new double[m*n]);
  memcpy(data_ptr->data.get(), x.data(), m*n*sizeof(double));
  LinearMap X(new DenseMatrixImpl(m, n, data_ptr, 'N'));

  return ToVector((A_*(B_*X).Transpose()).Transpose().impl().AsDense());
}

bool KroneckerProductImpl::operator==(const LinearMapImpl& other) const {
  if (other.type() != KRONECKER_PRODUCT ||
      other.m() != m() ||
      other.n() != n())
    return false;

  auto const& K = static_cast<const KroneckerProductImpl&>(other);
  return K.A() == A() && K.B() == B();
}


}  // namespace linear_map
