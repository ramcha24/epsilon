
#include <glog/logging.h>

#include "epsilon/vector/block_vector.h"

BlockVector& BlockVector::operator+=(const BlockVector& rhs) {
  for (auto iter : rhs.data_)
    InsertOrAdd(iter.first, iter.second);
  return *this;
}

BlockVector& BlockVector::operator-=(const BlockVector& rhs) {
  for (auto iter : rhs.data_)
    InsertOrAdd(iter.first, -iter.second);
  return *this;
}

BlockVector operator+(BlockVector lhs, const BlockVector& rhs) {
  lhs += rhs;
  return lhs;
}

BlockVector operator-(BlockVector lhs, const BlockVector& rhs) {
  lhs -= rhs;
  return lhs;
}

void BlockVector::InsertOrAdd(
    const std::string& key,
    DenseVector value) {
  auto res = data_.insert(std::make_pair(key, value));
  if (!res.second) (res.first)->second += value;
}

const BlockVector::DenseVector& BlockVector::operator()(
    const std::string& key) const {
  auto iter = data_.find(key);
  if (iter == data_.end())
    LOG(FATAL) << key << " not in BlockVector";
  return iter->second;
}

double BlockVector::norm() const {
  double norm_squared = 0;
  for (auto iter : data_) {
    norm_squared += iter.second.squaredNorm();
  }
  return sqrt(norm_squared);
}
