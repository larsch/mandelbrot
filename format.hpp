#ifndef _format_hpp
#define _format_hpp

#include <quadmath.h>

#include <limits>
#include <memory>
#include <string>

#include "mpfrfloat.hpp"

template <typename FLT>
std::string float2string(FLT f) {
  char *str;
  asprintf(&str, "%.*e", std::numeric_limits<FLT>::digits10, f);
  std::unique_ptr<char> ptr(str);
  return std::string(str);
}

template <>
std::string float2string<long double>(long double f) {
  char *str;
  asprintf(&str, "%.*le", std::numeric_limits<long double>::digits10, f);
  std::unique_ptr<char> ptr(str);
  return std::string(str);
}

template <>
std::string float2string<doubledouble<__float128>>(doubledouble<__float128> f) {
  return float2string(gmpfloat<256>(f));
}

template <>
std::string float2string<doubledouble<long double>>(
    doubledouble<long double> f) {
  return float2string(gmpfloat(f));
}

template <>
std::string float2string<doubledouble<double>>(doubledouble<double> f) {
  return float2string(gmpfloat(f));
}

template <>
std::string float2string<doubledouble<float>>(doubledouble<float> f) {
  return float2string(gmpfloat(f));
}

template <>
std::string float2string<__float128>(__float128 f) {
  char *str;
  // auto size = quadmath_snprintf(NULL, 0, "%.*e", __LDBL_MANT_DIG__, f);
  // printf("size: %lld\n", size);
  auto size = 128;
  str = new char[size + 1];
  quadmath_snprintf(str, size + 1, "%.*Qe", __LDBL_MANT_DIG__, f);
  std::unique_ptr<char> ptr(str);
  return std::string(str);
}

template <int N>
std::string float2string<gmpfloat<N>>(gmpfloat<N> f) {
  (void)f;
  return str::string();
}

template <int N>
std::string float2string<mpfrfloat<N, MPFR_RNDZ>>(mpfrfloat<N, MPFR_RNDD> f) {
  (void)f;
  return str::string();
}

template <typename FLT>
FLT string2float(const std::string &str) {
  FLT f;
  sscanf(str.c_str(), "%e", &f);
  return f;
}

template <

#endif  // _format_hpp
