#ifndef UTIL_VECTOR_H
#define UTIL_VECTOR_H

#include <memory>

#include <Eigen/Dense>
#include <Eigen/SparseCore>

#include "epsilon/data.pb.h"

using Eigen::CwiseNullaryOp;
using Eigen::EigenBase;
using Eigen::MatrixXd;
using Eigen::VectorXd;

typedef Eigen::SparseMatrix<double> SparseXd;
typedef void(*VectorFunction)(
    const std::vector<const VectorXd*>& input,
    const std::vector<VectorXd*>& output);

// Create a block diagonal matrix with A repeated k times
SparseXd BlockDiag(const MatrixXd& A, int k);
SparseXd RandomSparse(int m, int n, double d);
SparseXd DiagonalSparse(const VectorXd& a);
SparseXd SparseIdentity(int n);

bool IsDiagonal(const SparseXd& A);

// True if A = [ aI; 0; bI; ...]
bool IsBlockScalar(const SparseXd& A);

// Row and column norms of sparse matrices
VectorXd RowNorm(const SparseXd& A);
VectorXd ColNorm(const SparseXd& A);

// X = [A; B]
MatrixXd Stack(const MatrixXd& A, const MatrixXd& B);

// vec()/mat() operators
Eigen::VectorXd ToVector(const Eigen::MatrixXd& A);
Eigen::MatrixXd ToMatrix(const Eigen::VectorXd& a, int m, int n);

// Write matrices in text format for debugging
void WriteTextMatrix(const SparseXd& input, const std::string& file);
void WriteTextSparseMatrix(const SparseXd& input, const std::string& file);
void WriteTextVector(const VectorXd& input, const std::string& file);

// Debugging strings
std::string VectorDebugString(const VectorXd& x);
std::string MatrixDebugString(const MatrixXd& A);
std::string SparseMatrixDebugString(const SparseXd& A);


#endif  // UTIL_VECTOR_H
