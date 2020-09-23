#ifndef _doubledouble_hpp
#define _doubledouble_hpp

template <typename FLT>
class doubledouble {
 public:
  explicit doubledouble(FLT _r = 0.0, FLT _e = 0.0) : r(_r), e(_e) {}
  explicit operator double() const { return double(r); }
  doubledouble& operator+=(const doubledouble& other);
  doubledouble& operator-=(const doubledouble& other);
  FLT r, e;
};

template <typename FLT>
doubledouble<FLT> two_difference(FLT x, FLT y) {
  FLT r = x - y;
  FLT t = r - x;
  FLT e = (x - (r - t)) - (y + t);
  return doubledouble<FLT>(r, e);
}

template <typename FLT>
doubledouble<FLT> two_sum_quick(FLT x, FLT y) {
  FLT r = x + y;
  return doubledouble<FLT>(r, y - (r - x));
}

template <typename FLT>
doubledouble<FLT> two_sum(FLT x, FLT y) {
  FLT r = x + y;
  FLT t = r - x;
  FLT e = (x - (r - t)) + (y - t);
  return doubledouble<FLT>(r, e);
}

template <typename FLT>
doubledouble<FLT> two_product(FLT x, FLT y) {
  FLT u = x * 134217729.0;
  FLT v = y * 134217729.0;
  FLT s = u - (u - x);
  FLT t = v - (v - y);
  FLT f = x - s;
  FLT g = y - t;
  FLT r = x * y;
  FLT e = ((s * t - r) + s * g + f * t) + f * g;
  return doubledouble<FLT>(r, e);
}

template <typename FLT>
doubledouble<FLT>& doubledouble<FLT>::operator+=(
    const doubledouble<FLT>& other) {
  doubledouble<FLT> re = two_sum(r, other.r);
  re.e += e + other.e;
  *this = two_sum_quick(re.r, re.e);
  return *this;
}

template <typename FLT>
doubledouble<FLT>& doubledouble<FLT>::operator-=(
    const doubledouble<FLT>& other) {
  doubledouble<FLT> re = two_difference(r, other.r);
  re.e += e - other.e;
  *this = two_sum_quick(re.r, re.e);
  return *this;
}

template <typename FLT>
doubledouble<FLT> operator*(const doubledouble<FLT> lhs, FLT rhs) {
  doubledouble<FLT> re = two_product(rhs, lhs.r);
  re.e += rhs * lhs.e;
  return two_sum_quick(re.r, re.e);
}

template <typename FLT>
doubledouble<FLT> operator+(const doubledouble<FLT>& lhs,
                            const doubledouble<FLT>& rhs) {
  doubledouble<FLT> re = two_sum(lhs.r, rhs.r);
  re.e += lhs.e + rhs.e;
  return two_sum_quick(re.r, re.e);
}

template <typename FLT>
doubledouble<FLT> operator-(const doubledouble<FLT>& lhs,
                            const doubledouble<FLT>& rhs) {
  doubledouble re = two_difference(lhs.r, rhs.r);
  re.e += lhs.e - rhs.e;
  return two_sum_quick(re.r, re.e);
}

template <typename FLT>
doubledouble<FLT> operator*(const doubledouble<FLT>& lhs,
                            const doubledouble<FLT>& rhs) {
  doubledouble<FLT> re = two_product(lhs.r, rhs.r);
  re.e += lhs.r * rhs.e + lhs.e * rhs.r;
  return two_sum_quick(re.r, re.e);
}

template <typename FLT>
bool operator<(const doubledouble<FLT>& lhs, doubledouble<FLT> rhs) {
  return lhs.r < rhs.r || (lhs.r == rhs.r && lhs.e < rhs.e);
}

template <typename FLT>
bool operator>(const doubledouble<FLT>& lhs, doubledouble<FLT> rhs) {
  return lhs.r > rhs.r || (lhs.r == rhs.r && lhs.e > rhs.e);
}

template <typename FLT>
bool operator<(const doubledouble<FLT>& lhs, double rhs) {
  return lhs.r < rhs;
}
template <typename FLT>
bool operator>(const doubledouble<FLT>& lhs, double rhs) {
  return lhs.r > rhs;
}

template <typename FLT>
std::ostream& operator<<(std::ostream& os, const doubledouble<FLT>& rhs) {
  return os << rhs.r + rhs.e;
}

template <typename FLT>
bool operator==(const doubledouble<FLT>& lhs, const doubledouble<FLT>& rhs) {
  return lhs.r == rhs.r && lhs.e == rhs.e;
}

template <typename FLT>
bool operator!=(const doubledouble<FLT>& lhs, const doubledouble<FLT>& rhs) {
  return lhs.r != rhs.r || lhs.e != rhs.e;
}

#endif  // _doubledouble_hpp
