/**
 * @file strop.hpp
 *
 * Streaming operators for custom float point types.
 */

#ifndef _strop_hpp
#define _strop_hpp

#include "doubledouble.hpp"
#include "gmpfloat.hpp"
#include "mpfrfloat.hpp"

template <typename T>
std::ostream& fmt(std::ostream& os, const char* str, T exp) {
  if (str[0] == 0) {
    return os << '0';
  }

  if (str[0] == '-') {
    os << '-';
    ++str;
  }

  if (str[0]) {
    if (str[1]) {
      return os << str[0] << '.' << (str + 1) << 'e' << (exp - 1);
    } else {
      return os << str[0] << 'e' << (exp - 1);
    }
  } else {
    return os << '0';
  }
}

template <mp_bitcnt_t PREC>
std::ostream& operator<<(std::ostream& os, const gmpfloat<PREC>& rhs) {
  mp_exp_t exp;
  std::unique_ptr<char, decltype(&std::free)> str(
      mpf_get_str(NULL, &exp, 10, os.precision(), rhs.mpf), std::free);
  return fmt(os, str.get(), exp);
}

template <mp_bitcnt_t PREC>
std::istream& operator>>(std::istream& os, gmpfloat<PREC>& rhs) {
  std::string str;
  os >> str;
  mpf_set_str(rhs.mpf, str.c_str(), 10);
  return os;
}

template <int PREC, mpfr_rnd_t RND>
std::ostream& operator<<(std::ostream& os, const mpfrfloat<PREC, RND>& rhs) {
  mpfr_exp_t exp;
  std::unique_ptr<char, decltype(&std::free)> str(
      mpfr_get_str(NULL, &exp, 10, os.precision(), rhs.mpfr, RND), std::free);
  return fmt(os, str.get(), exp);
}

template <int PREC, mpfr_rnd_t RND>
std::istream& operator>>(std::istream& os, mpfrfloat<PREC, RND>& rhs) {
  std::string str;
  os >> str;
  mpfr_set_str(rhs.mpfr, str.c_str(), 10, RND);
  return os;
}

template <typename FLT>
std::ostream& operator<<(std::ostream& os, const doubledouble<FLT>& rhs) {
  return os << gmpfloat<2 * std::numeric_limits<FLT>::digits>(rhs);
}

template <typename FLT>
std::istream& operator>>(std::istream& os, doubledouble<FLT>& rhs) {
  gmpfloat<2 * std::numeric_limits<FLT>::digits> flt;
  os >> flt;
  rhs = doubledouble<FLT>(flt);
  return os;
}

inline std::ostream& operator<<(std::ostream& os, const __float128& rhs) {
  return os << gmpfloat<113>(rhs);
}

inline std::istream& operator>>(std::istream& os, __float128& rhs) {
  gmpfloat<113> flt;
  os >> flt;
  rhs = __float128(flt);
  return os;
}

#endif  // _strop_hpp
