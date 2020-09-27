#ifndef _typename_hpp
#define _typename_hpp

template <typename T>
const char* tname() {
  return typeid(T).name();
}

template <>
const char* tname<float>() {
  return "float";
}

template <>
const char* tname<double>() {
  return "double";
}

template <>
const char* tname<long double>() {
  return "long double";
}

template <>
const char* tname<__float128>() {
  return "__float128";
}

template <>
const char* tname<doubledouble<float>>() {
  return "doubledouble<float>";
}

template <>
const char* tname<doubledouble<double>>() {
  return "doubledouble<double>";
}

template <>
const char* tname<doubledouble<long double>>() {
  return "doubledouble<long double>";
}

template <>
const char* tname<doubledouble<__float128>>() {
  return "doubledouble<__float128>";
}

template <>
const char* tname<gmpfloat<128>>() {
  return "gmpfloat<128>";
}

template <>
const char* tname<gmpfloat<256>>() {
  return "gmpfloat<256>";
}

template <>
const char* tname<gmpfloat<1024>>() {
  return "gmpfloat<1024>";
}

template <>
const char* tname<mpfrfloat<128, MPFR_RNDD>>() {
  return "mpfrfloat<128>";
}

template <>
const char* tname<mpfrfloat<256, MPFR_RNDD>>() {
  return "mpfrfloat<256>";
}

#endif  // _typename_hpp
