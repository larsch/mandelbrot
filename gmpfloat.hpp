#ifndef _gmpfloat_hpp
#define _gmpfloat_hpp

#include <gmp.h>

#include <cmath>
#include <iostream>

#include "doubledouble.hpp"

template <typename FLT>
FLT convert_to_float(const mpf_t mpf) {
  signed long exp;
  double val = mpf_get_d_2exp(&exp, mpf);
  std::cout << val << " " << exp << std::endl;
  return FLT(val) * FLT(exp2((long double)exp));
}

template <>
long double convert_to_float(const mpf_t mpf) {
  double truncated_double = mpf_get_d(mpf);
  long double result{truncated_double};

  mpf_t truncated_mpf;
  mpf_init_set_d(truncated_mpf, truncated_double);

  mpf_t residual;
  mpf_init2(residual, mpf_get_prec(mpf));
  mpf_sub(residual, mpf, truncated_mpf);
  mpf_clear(truncated_mpf);
  truncated_double = mpf_get_d(residual);
  mpf_clear(residual);
  result += truncated_double;
  return result;
}

template <>
doubledouble<double> convert_to_float(const mpf_t mpf) {
  double truncated_double = mpf_get_d(mpf);
  mpf_t truncated_mpf;
  mpf_init_set_d(truncated_mpf, truncated_double);
  mpf_t residual_mpf;
  mpf_init2(residual_mpf, mpf_get_prec(mpf));
  mpf_sub(residual_mpf, mpf, truncated_mpf);
  mpf_clear(truncated_mpf);
  double residual = mpf_get_d(residual_mpf);
  mpf_clear(residual_mpf);
  return doubledouble<double>{truncated_double, residual};
}

template <mp_bitcnt_t PREC>
class gmpfloat {
 public:
  gmpfloat() { mpf_init2(mpf, PREC); }

  template <mp_bitcnt_t PREC2>
  gmpfloat(const gmpfloat<PREC2>& g) {
    mpf_init2(mpf, PREC);
    mpf_set(mpf, g.mpf);
  }
  gmpfloat(const gmpfloat<PREC>& g) { mpf_init_set(mpf, g.mpf); }

  gmpfloat& operator=(const gmpfloat<PREC>& b) {
    mpf_clear(mpf);
    mpf_init_set(mpf, b.mpf);
    return *this;
  }

  ~gmpfloat<PREC>() { mpf_clear(mpf); }

  explicit gmpfloat<PREC>(mpf_t w) { mpf[0] = w[0]; }

  explicit gmpfloat<PREC>(double _f) { mpf_init_set_d(mpf, _f); }

  explicit gmpfloat<PREC>(int i) { mpf_init_set_si(mpf, i); }

  explicit gmpfloat<PREC>(unsigned int i) { mpf_init_set_ui(mpf, i); }

  explicit gmpfloat<PREC>(long double f) {
    mpf_init2(mpf, PREC);
    mpf_set_d(mpf, double(f));
    f -= mpf_get_d(mpf);
    mpf_t temp;
    mpf_init_set_d(temp, double(f));
    mpf_add(mpf, mpf, temp);
    mpf_clear(temp);
  }

  explicit operator long double() const {
    return convert_to_float<long double>(mpf);
  }

  explicit operator __float128() const {
    return convert_to_float<__float128>(mpf);
  }

  template <typename FLT>
  explicit operator doubledouble<FLT>() const {
    return convert_to_float<doubledouble<FLT>>(mpf);
  }

  explicit operator double() const { return mpf_get_d(mpf); }

  explicit operator float() const { return static_cast<float>(mpf_get_d(mpf)); }

  explicit operator doubledouble<double>() const;

  gmpfloat<PREC>& operator+=(const gmpfloat<PREC>& o) {
    mpf_add(mpf, mpf, o.mpf);
    return *this;
  }

  gmpfloat<PREC>& operator-=(const gmpfloat<PREC>& o) {
    mpf_sub(mpf, mpf, o.mpf);
    return *this;
  }

  gmpfloat<PREC>& operator*=(const gmpfloat<PREC>& o) {
    mpf_mul(mpf, mpf, o.mpf);
    return *this;
  }

  mpf_t mpf;
};

template <mp_bitcnt_t PREC>
gmpfloat<PREC> operator+(const gmpfloat<PREC>& f, const gmpfloat<PREC>& g) {
  mpf_t r;
  mpf_init2(r, PREC);
  mpf_add(r, f.mpf, g.mpf);
  return gmpfloat<PREC>(r);
}

template <mp_bitcnt_t PREC>
gmpfloat<PREC> operator-(const gmpfloat<PREC>& f, const gmpfloat<PREC>& g) {
  mpf_t r;
  mpf_init2(r, PREC);
  mpf_sub(r, f.mpf, g.mpf);
  return gmpfloat<PREC>(r);
}

template <mp_bitcnt_t PREC>
gmpfloat<PREC> operator*(const gmpfloat<PREC>& f, const gmpfloat<PREC>& g) {
  mpf_t r;
  mpf_init2(r, PREC);
  mpf_mul(r, f.mpf, g.mpf);
  return gmpfloat<PREC>(r);
}

template <mp_bitcnt_t PREC>
gmpfloat<PREC> operator*(const gmpfloat<PREC>& lhs, int rhs) {
  return lhs * gmpfloat<PREC>(rhs);
}

template <mp_bitcnt_t PREC>
gmpfloat<PREC> operator*(int lhs, const gmpfloat<PREC>& rhs) {
  return gmpfloat<PREC>(lhs) * rhs;
}

template <mp_bitcnt_t PREC>
gmpfloat<PREC>::operator doubledouble<double>() const {
  double r = mpf_get_d(mpf);
  double e = double(*this - gmpfloat<PREC>(r));
  return doubledouble<double>(r, e);
}

template <mp_bitcnt_t PREC>
gmpfloat<PREC> operator/(const gmpfloat<PREC>& lhs, const gmpfloat<PREC>& rhs) {
  mpf_t r;
  mpf_init2(r, PREC);
  mpf_div(r, lhs.mpf, rhs.mpf);
  return gmpfloat<PREC>(r);
}

template <mp_bitcnt_t PREC>
bool operator<(const gmpfloat<PREC>& lhs, double rhs) {
  return double(lhs) < rhs;
}

template <mp_bitcnt_t PREC>
bool operator>(const gmpfloat<PREC>& lhs, double rhs) {
  return double(lhs) > rhs;
}

template <mp_bitcnt_t PREC>
bool operator>(const gmpfloat<PREC>& lhs, const gmpfloat<PREC>& rhs) {
  return mpf_cmp(lhs.mpf, rhs.mpf) > 0;
}

template <mp_bitcnt_t PREC>
bool operator<(const gmpfloat<PREC>& lhs, const gmpfloat<PREC>& rhs) {
  return mpf_cmp(lhs.mpf, rhs.mpf) < 0;
}

template <mp_bitcnt_t PREC>
std::ostream& operator<<(std::ostream& os, const gmpfloat<PREC>& rhs) {
  char str[32];
  gmp_sprintf(str, "%Fe", rhs.mpf);
  return os << str;
}

template <mp_bitcnt_t PREC>
bool operator==(const gmpfloat<PREC>& lhs, const gmpfloat<PREC>& rhs) {
  return mpf_cmp(lhs.mpf, rhs.mpf) == 0;
}

template <mp_bitcnt_t PREC>
bool operator!=(const gmpfloat<PREC>& lhs, const gmpfloat<PREC>& rhs) {
  return mpf_cmp(lhs.mpf, rhs.mpf) != 0;
}

#endif  // _gmpfloat_hpp
