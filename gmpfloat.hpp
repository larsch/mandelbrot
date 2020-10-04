/**
 * @file gmpfloat.hpp
 *
 * Arbitrary precision floating point type using GNU GMP.
 */

#ifndef _gmpfloat_hpp
#define _gmpfloat_hpp

#include <gmp.h>
#include <mpfr.h>

#include <cmath>
#include <iostream>
#include <memory>

#include "doubledouble.hpp"

/**
 * Arbitrary precision floating point using GNU GMP
 */
template <mp_bitcnt_t PREC>
class gmpfloat {
 public:
  gmpfloat() { mpf_init2(mpf, PREC); }

  explicit gmpfloat<PREC>(mpf_t w) { mpf[0] = w[0]; }

  gmpfloat(const gmpfloat<PREC>& g) { mpf_init_set(mpf, g.mpf); }

  gmpfloat(gmpfloat<PREC>&& g) {
    *mpf = *g.mpf;
    mpf_init2(g.mpf, PREC);
  }

  ~gmpfloat<PREC>() { mpf_clear(mpf); }

  gmpfloat& operator=(const gmpfloat<PREC>& b) {
    if (this != &b) {
      mpf_clear(mpf);
      mpf_init_set(mpf, b.mpf);
    }
    return *this;
  }

  template <mp_bitcnt_t PREC2>
  explicit gmpfloat(const gmpfloat<PREC2>& g) {
    mpf_init2(mpf, PREC);
    mpf_set(mpf, g.mpf);
  }

  explicit gmpfloat<PREC>(int i) {
    mpf_init2(mpf, PREC);
    mpf_set_si(mpf, i);
  }

  explicit gmpfloat<PREC>(unsigned int i) {
    mpf_init2(i, PREC);
    mpf_set_ui(mpf, i);
  }

  explicit gmpfloat<PREC>(float _f) {
    mpf_init2(mpf, PREC);
    mpf_set_d(mpf, _f);
  }

  explicit gmpfloat<PREC>(double _f) {
    mpf_init2(mpf, PREC);
    mpf_set_d(mpf, _f);
  }

  explicit gmpfloat(const long double& f) {
    mpfr_t mpfr;
    mpfr_init2(mpfr, 64);
    mpfr_set_ld(mpfr, f, MPFR_RNDD);
    mpf_init2(mpf, PREC);
    mpfr_get_f(mpf, mpfr, MPFR_RNDD);
    mpfr_clear(mpfr);
  }

  explicit gmpfloat(const __float128& f) {
    mpfr_t mpfr;
    mpfr_init2(mpfr, 113);
    mpfr_set_float128(mpfr, f, MPFR_RNDD);
    mpf_init2(mpf, PREC);
    mpfr_get_f(mpf, mpfr, MPFR_RNDD);
    mpfr_clear(mpfr);
  }

  explicit gmpfloat(const doubledouble<float>& g) {
    mpf_init2(mpf, PREC);
    mpf_set_d(mpf, g.r);
    mpf_t temp;
    mpf_init_set_d(temp, g.e);
    mpf_add(mpf, mpf, temp);
    mpf_clear(temp);
  }

  explicit gmpfloat(const doubledouble<double>& g) {
    mpf_init2(mpf, PREC);
    mpf_set_d(mpf, g.r);
    mpf_t temp;
    mpf_init_set_d(temp, g.e);
    mpf_add(mpf, mpf, temp);
    mpf_clear(temp);
  }

  explicit gmpfloat(const doubledouble<long double>& g) {
    mpfr_t e;
    mpfr_init2(e, 130);
    mpfr_set_ld(e, g.r, MPFR_RNDD);
    mpfr_t r;
    mpfr_init2(r, 130);
    mpfr_set_ld(r, g.e, MPFR_RNDD);
    mpfr_add(e, e, r, MPFR_RNDD);
    mpf_init2(mpf, PREC);
    mpfr_get_f(mpf, e, MPFR_RNDD);
    mpfr_clear(e);
    mpfr_clear(r);
  }

  explicit gmpfloat(const doubledouble<__float128>& g) {
    mpfr_t e;
    mpfr_init2(e, 256);
    mpfr_set_float128(e, g.r, MPFR_RNDD);
    mpfr_t r;
    mpfr_init2(r, 256);
    mpfr_set_float128(r, g.e, MPFR_RNDD);
    mpfr_add(e, e, r, MPFR_RNDD);
    mpf_init2(mpf, PREC);
    mpfr_get_f(mpf, e, MPFR_RNDD);
    mpfr_clear(e);
    mpfr_clear(r);
  }

  explicit operator float() const { return static_cast<float>(mpf_get_d(mpf)); }

  explicit operator double() const { return mpf_get_d(mpf); }

  explicit operator long double() const {
    mpfr_t mpfr;
    mpfr_init2(mpfr, 113);
    mpfr_set_f(mpfr, mpf, MPFR_RNDD);
    long double result = mpfr_get_ld(mpfr, MPFR_RNDD);
    mpfr_clear(mpfr);
    return result;
  }

  explicit operator __float128() const {
    mpfr_t mpfr;
    mpfr_init2(mpfr, 113);
    mpfr_set_f(mpfr, mpf, MPFR_RNDD);
    __float128 result = mpfr_get_float128(mpfr, MPFR_RNDD);
    mpfr_clear(mpfr);
    return result;
  }

  template <typename FLT>
  explicit operator doubledouble<FLT>() const {
    FLT r = FLT(*this);
    FLT e = FLT(*this - gmpfloat<PREC>(r));
    return doubledouble<FLT>(r, e);
  }

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

  gmpfloat<PREC>& operator/=(const gmpfloat<PREC>& o) {
    mpf_div(mpf, mpf, o.mpf);
    return *this;
  }

  mpf_t mpf;
};

template <mp_bitcnt_t PREC>
gmpfloat<PREC> operator+(const gmpfloat<PREC>& lhs, const gmpfloat<PREC>& rhs) {
  mpf_t result;
  mpf_init2(result, PREC);
  mpf_add(result, lhs.mpf, rhs.mpf);
  return gmpfloat<PREC>(result);
}

template <mp_bitcnt_t PREC>
gmpfloat<PREC> operator-(const gmpfloat<PREC>& lhs, const gmpfloat<PREC>& rhs) {
  mpf_t result;
  mpf_init2(result, PREC);
  mpf_sub(result, lhs.mpf, rhs.mpf);
  return gmpfloat<PREC>(result);
}

template <mp_bitcnt_t PREC>
gmpfloat<PREC> operator*(const gmpfloat<PREC>& lhs, const gmpfloat<PREC>& rhs) {
  mpf_t result;
  mpf_init2(result, PREC);
  mpf_mul(result, lhs.mpf, rhs.mpf);
  return gmpfloat<PREC>(result);
}

template <mp_bitcnt_t PREC>
gmpfloat<PREC> operator/(const gmpfloat<PREC>& lhs, const gmpfloat<PREC>& rhs) {
  mpf_t result;
  mpf_init2(result, PREC);
  mpf_div(result, lhs.mpf, rhs.mpf);
  return gmpfloat<PREC>(result);
}

template <mp_bitcnt_t PREC>
bool operator<(const gmpfloat<PREC>& lhs, const gmpfloat<PREC>& rhs) {
  return mpf_cmp(lhs.mpf, rhs.mpf) < 0;
}

template <mp_bitcnt_t PREC>
bool operator<(const gmpfloat<PREC>& lhs, double rhs) {
  return double(lhs) < rhs;
}

template <mp_bitcnt_t PREC>
bool operator>(const gmpfloat<PREC>& lhs, const gmpfloat<PREC>& rhs) {
  return mpf_cmp(lhs.mpf, rhs.mpf) > 0;
}

template <mp_bitcnt_t PREC>
bool operator>(const gmpfloat<PREC>& lhs, double rhs) {
  return double(lhs) > rhs;
}

template <mp_bitcnt_t PREC>
bool operator==(const gmpfloat<PREC>& lhs, const gmpfloat<PREC>& rhs) {
  return mpf_cmp(lhs.mpf, rhs.mpf) == 0;
}

template <mp_bitcnt_t PREC>
bool operator!=(const gmpfloat<PREC>& lhs, const gmpfloat<PREC>& rhs) {
  return mpf_cmp(lhs.mpf, rhs.mpf) != 0;
}

template <mp_bitcnt_t PREC>
gmpfloat<PREC> operator*(const gmpfloat<PREC>& lhs, int rhs) {
  mpf_t result;
  mpf_init2(result, PREC);
  if (rhs < 0) {
    mpf_mul_ui(result, lhs.mpf, -rhs);
    mpf_neg(result, result);
  } else {
    mpf_mul_ui(result, lhs.mpf, rhs);
  }
  return gmpfloat<PREC>(result);
}

template <mp_bitcnt_t PREC>
gmpfloat<PREC> operator*(int lhs, const gmpfloat<PREC>& rhs) {
  return rhs * lhs;
}

namespace std {

template <mp_bitcnt_t PREC>
class numeric_limits<gmpfloat<PREC>> {
 public:
  static const size_t digits10 = PREC * 1 / 3;
  static const size_t digits = PREC;
  static gmpfloat<PREC> epsilon();
};

template <>
inline gmpfloat<128> numeric_limits<gmpfloat<128>>::epsilon() {
  return gmpfloat<128>(0.58774717541114375e-38);
}

template <>
inline gmpfloat<256> numeric_limits<gmpfloat<256>>::epsilon() {
  return gmpfloat<256>(0.17272337110188889e-76);
}

template <mp_bitcnt_t PREC>
gmpfloat<PREC> abs(const gmpfloat<PREC> f) {
  gmpfloat<PREC> result;
  mpf_abs(result.mpf, f.mpf);
  return result;
}

}  // namespace std

#endif  // _gmpfloat_hpp
