/**
 * @file mpfrfloat.hpp
 *
 * Arbitrary precision floating point type using GNU MPFR.
 */

#ifndef _mpfrfloat_hpp
#define _mpfrfloat_hpp

#include "float.hpp"
#include "mpfr.h"

/**
 * Arbitrary precision floating point using GNU GMP
 */
template <int PREC, mpfr_rnd_t RND>
class mpfrfloat {
 public:
  mpfrfloat() { mpfr_init2(mpfr, PREC); }

  explicit mpfrfloat(mpfr_t f) { mpfr[0] = f[0]; }

  mpfrfloat(const mpfrfloat& g) { mpfr_init_set(mpfr, g.mpfr, RND); }

  ~mpfrfloat() { mpfr_clear(mpfr); }

  mpfrfloat& operator=(const mpfrfloat& b) {
    if (this != &b) {
      mpfr_clear(mpfr);
      mpfr_init_set(mpfr, b.mpfr, RND);
    }
    return *this;
  }

  explicit mpfrfloat(int i) {
    mpfr_init2(mpfr, PREC);
    mpfr_set_si(mpfr, i, RND);
  }

  explicit mpfrfloat(unsigned int i) {
    mpfr_init2(mpfr, PREC);
    mpfr_init_set_ui(mpfr, i, RND);
  }

  explicit mpfrfloat(float _f) {
    mpfr_init2(mpfr, PREC);
    mpfr_set_flt(mpfr, _f, RND);
  }

  explicit mpfrfloat(double _f) {
    mpfr_init2(mpfr, PREC);
    mpfr_set_d(mpfr, _f, RND);
  }

  explicit mpfrfloat(const long double& f) {
    mpfr_init2(mpfr, PREC);
    mpfr_set_ld(mpfr, f, MPFR_RNDD);
  }

  explicit mpfrfloat(const __float128& f) {
    mpfr_init2(mpfr, PREC);
    mpfr_set_float128(mpfr, f, RND);
  }

  explicit mpfrfloat(const doubledouble<float>& g) {
    mpfr_init2(mpfr, PREC);
    mpfr_set_d(mpfr, g.r, RND);
    mpfr_add_d(mpfr, mpfr, g.e, RND);
  }

  explicit mpfrfloat(const doubledouble<double>& g) {
    mpfr_init2(mpfr, PREC);
    mpfr_set_d(mpfr, g.r, RND);
    mpfr_add_d(mpfr, mpfr, g.e, RND);
  }

  explicit mpfrfloat(const doubledouble<long double>& g) {
    mpfr_init2(mpfr, PREC);
    mpfr_set_ld(mpfr, g.r, MPFR_RNDD);
    mpfr_t r;
    mpfr_init2(r, PREC);
    mpfr_set_ld(r, g.e, MPFR_RNDD);
    mpfr_add(mpfr, mpfr, r, MPFR_RNDD);
    mpfr_clear(r);
  }

  explicit mpfrfloat(const doubledouble<__float128>& g) {
    mpfr_init2(mpfr, PREC);
    mpfr_set_float128(mpfr, g.r, MPFR_RNDD);
    mpfr_t r;
    mpfr_init2(r, PREC);
    mpfr_set_float128(r, g.e, MPFR_RNDD);
    mpfr_add(mpfr, mpfr, r, MPFR_RNDD);
    mpfr_clear(r);
  }

  explicit operator float() const { return mpfr_get_flt(mpfr, RND); }

  explicit operator double() const { return mpfr_get_d(mpfr, RND); }

  explicit operator long double() const { return mpfr_get_ld(mpfr, RND); }

  explicit operator __float128() const { return mpfr_get_float128(mpfr, RND); }

  template <typename FLT>
  explicit operator doubledouble<FLT>() const {
    FLT r = FLT(*this);
    FLT e = FLT(*this - mpfrfloat<PREC, RND>(r));
    return doubledouble<FLT>(r, e);
  }

  mpfrfloat& operator+=(const mpfrfloat& o) {
    mpfr_add(mpfr, mpfr, o.mpfr, RND);
    return *this;
  }

  mpfrfloat& operator-=(const mpfrfloat& o) {
    mpfr_sub(mpfr, mpfr, o.mpfr, RND);
    return *this;
  }

  mpfrfloat& operator*=(const mpfrfloat& o) {
    mpfr_mul(mpfr, mpfr, o.mpfr, RND);
    return *this;
  }

  mpfrfloat& operator/=(const mpfrfloat& o) {
    mpfr_div(mpfr, mpfr, o.mpfr, RND);
    return *this;
  }

  mpfr_t mpfr;
};

template <int PREC, mpfr_rnd_t RND>
mpfrfloat<PREC, RND> operator+(const mpfrfloat<PREC, RND>& lhs,
                               const mpfrfloat<PREC, RND>& rhs) {
  mpfr_t result;
  mpfr_init2(result, PREC);
  mpfr_add(result, lhs.mpfr, rhs.mpfr, RND);
  return mpfrfloat<PREC, RND>(result);
}

template <int PREC, mpfr_rnd_t RND>
mpfrfloat<PREC, RND> operator-(const mpfrfloat<PREC, RND>& lhs,
                               const mpfrfloat<PREC, RND>& rhs) {
  mpfr_t result;
  mpfr_init2(result, PREC);
  mpfr_sub(result, lhs.mpfr, rhs.mpfr, RND);
  return mpfrfloat<PREC, RND>(result);
}

template <int PREC, mpfr_rnd_t RND>
mpfrfloat<PREC, RND> operator*(const mpfrfloat<PREC, RND>& lhs,
                               const mpfrfloat<PREC, RND>& rhs) {
  mpfr_t result;
  mpfr_init2(result, PREC);
  mpfr_mul(result, lhs.mpfr, rhs.mpfr, RND);
  return mpfrfloat<PREC, RND>(result);
}

template <int PREC, mpfr_rnd_t RND>
mpfrfloat<PREC, RND> operator/(const mpfrfloat<PREC, RND>& lhs,
                               const mpfrfloat<PREC, RND>& rhs) {
  mpfr_t result;
  mpfr_init2(result, PREC);
  mpfr_div(result, lhs.mpfr, rhs.mpfr, RND);
  return mpfrfloat<PREC, RND>(result);
}

template <int PREC, mpfr_rnd_t RND>
bool operator<(const mpfrfloat<PREC, RND>& lhs,
               const mpfrfloat<PREC, RND>& rhs) {
  return mpfr_cmp(lhs.mpfr, rhs.mpfr) < 0;
}

template <int PREC, mpfr_rnd_t RND>
bool operator<(const mpfrfloat<PREC, RND>& lhs, double rhs) {
  return mpfr_cmp_d(lhs.mpfr, rhs) < 0;
}

template <int PREC, mpfr_rnd_t RND>
bool operator>(const mpfrfloat<PREC, RND>& lhs,
               const mpfrfloat<PREC, RND>& rhs) {
  return mpfr_cmp(lhs.mpfr, rhs.mpfr) > 0;
}

template <int PREC, mpfr_rnd_t RND>
bool operator>(const mpfrfloat<PREC, RND>& lhs, double rhs) {
  return mpfr_cmp_d(lhs.mpfr, rhs) > 0;
}

template <int PREC, mpfr_rnd_t RND>
bool operator==(const mpfrfloat<PREC, RND>& lhs,
                const mpfrfloat<PREC, RND>& rhs) {
  return mpfr_cmp(lhs.mpfr, rhs.mpfr) == 0;
}

template <int PREC, mpfr_rnd_t RND>
bool operator!=(const mpfrfloat<PREC, RND>& lhs,
                const mpfrfloat<PREC, RND>& rhs) {
  return mpfr_cmp(lhs.mpfr, rhs.mpfr) != 0;
}

namespace std {

template <int PREC>
double _epsilon();

template <>
double _epsilon<128>() {
  return 0.58774717541114375e-38;
}

template <>
double _epsilon<256>() {
  return 0.17272337110188889e-76;
}

// 0.58774717541114375e-38
template <int PREC, mpfr_rnd_t RND>
class numeric_limits<mpfrfloat<PREC, RND>> {
 public:
  static const size_t digits10 = PREC * 1 / 3;
  static const size_t digits = PREC;
  static mpfrfloat<PREC, RND> epsilon();
};

template <int PREC, mpfr_rnd_t RND>
mpfrfloat<PREC, RND> numeric_limits<mpfrfloat<PREC, RND>>::epsilon() {
  return mpfrfloat<PREC, RND>(_epsilon<PREC>());
}

template <int PREC, mpfr_rnd_t RND>
mpfrfloat<PREC, RND> abs(mpfrfloat<PREC, RND> f) {
  mpfrfloat<PREC, RND> result;
  mpfr_abs(result.mpfr, f.mpfr, RND);
  return result;
}

}  // namespace std

#endif  // _mpfrfloat_hpp
