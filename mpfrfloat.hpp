#include "float.hpp"
#include "mpfr.h"

template <int PREC, mpfr_rnd_t RND>
class mpfrfloat {
 public:
  mpfrfloat() { mpfr_init2(mpfr, PREC); };
  mpfrfloat(const mpfrfloat& g) { mpfr_init_set(mpfr, g.mpfr, RND); }
  mpfrfloat& operator=(const mpfrfloat& b) {
    mpfr_clear(mpfr);
    mpfr_init_set(mpfr, b.mpfr, RND);
    return *this;
  }
  ~mpfrfloat() { mpfr_clear(mpfr); }
  mpfrfloat(mpfr_t w) { mpfr[0] = w[0]; }
  // mpfrfloat(flt _f) {
  //   mpfr_init2(mpfr, PREC);
  //   mpfr_set_d(mpfr, _f, RND);
  // }
  mpfrfloat(double _f) {
    mpfr_init2(mpfr, PREC);
    mpfr_set_d(mpfr, _f, RND);
  }
  mpfrfloat(int i) {
    mpfr_init2(mpfr, PREC);
    mpfr_set_si(mpfr, i, RND);
  }
  mpfrfloat(unsigned int i) {
    mpfr_init2(mpfr, PREC);
    mpfr_init_set_ui(mpfr, i, RND);
  }
  explicit operator double() const { return mpfr_get_d(mpfr, RND); }
  mpfrfloat& operator+=(const mpfrfloat& o) {
    mpfr_add(mpfr, mpfr, o.mpfr, RND);
    return *this;
  }
  mpfrfloat& operator-=(const mpfrfloat& o) {
    mpfr_sub(mpfr, mpfr, o.mpfr, RND);
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
bool operator<(const mpfrfloat<PREC, RND>& lhs,
               const mpfrfloat<PREC, RND>& rhs) {
  return mpfr_cmp(lhs.mpfr, rhs.mpfr) < 0;
}

template <int PREC, mpfr_rnd_t RND>
bool operator<(const mpfrfloat<PREC, RND>& lhs, double rhs) {
  return mpfr_cmp_d(lhs.mpfr, rhs) < 0;
}

template <int PREC, mpfr_rnd_t RND>
bool operator>(const mpfrfloat<PREC, RND>& lhs, double rhs) {
  return mpfr_cmp_d(lhs.mpfr, rhs) > 0;
}

template <int PREC, mpfr_rnd_t RND>
bool operator>(const mpfrfloat<PREC, RND>& lhs,
               const mpfrfloat<PREC, RND>& rhs) {
  return mpfr_cmp(lhs.mpfr, rhs.mpfr) > 0;
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

template <int PREC, mpfr_rnd_t RND>
std::ostream& operator<<(std::ostream& os, const mpfrfloat<PREC, RND>& rhs) {
  char str[32];
  mpfr_sprintf(str, "%Re", rhs.mpfr);
  return os << str;
}
