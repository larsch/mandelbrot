#include "gmpfloat.hpp"
#include "mpfrfloat.hpp"

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
const char* tname<mpfrfloat<128, MPFR_RNDD>>() {
  return "mpfrfloat<128>";
}

template <>
const char* tname<mpfrfloat<256, MPFR_RNDD>>() {
  return "mpfrfloat<256>";
}

std::ostream& operator<<(std::ostream& os, const __float128 rhs) {
  return os << (long double)rhs;
}

#define assert(expr)                                                 \
  {                                                                  \
    std::cout << tname<FLT>() << "," << sizeof(FLT) << ","           \
              << approxepsilon<FLT>() << ": " << #expr << std::endl; \
    if (!(expr)) std::cout << "Failed: " #expr << std::endl;         \
  }

#define dassert(expr)                                        \
  {                                                          \
    std::cout << #expr << std::endl;                         \
    if (!(expr)) std::cout << "Failed: " #expr << std::endl; \
  }

template <typename FLT>
constexpr FLT approxepsilon() {
  FLT one{1.0};
  FLT epsilon{1.0};
  FLT half{0.5};
  // std::cout << tname<FLT>() << " ********" << std::endl;
  while (one + half * epsilon != one) {
    epsilon = half * epsilon;
    // std::cout << epsilon << std::endl;
  }
  return epsilon;
}

template <typename FLT>
void test_float_type() {
  const auto eps = approxepsilon<FLT>();
  assert(eps != FLT(0.0));
  assert(double(FLT(1.0)) == 1.0);
  assert(FLT(1.0) + FLT(1.0) - FLT(2.0) < eps);
  assert(FLT(1.0) - FLT(1.0) < eps);
  assert(FLT(1.0) - eps < FLT(1.0));
  assert(FLT(1.0) + eps > FLT(1.0));
  assert(FLT(1.0) + eps != FLT(1.0));
  assert(FLT(1.0) - eps != FLT(1.0));
  assert(FLT(1.0) == FLT(1.0));
}

void test_gmpfloat() {
  dassert(mpf_get_prec((gmpfloat<128>(8.0) * gmpfloat<128>(4.0)).mpf) == 128);
  dassert(mpf_get_prec((gmpfloat<256>(8.0) * gmpfloat<256>(4.0)).mpf) == 256);
  dassert(gmpfloat<1024>(1.0) +
              gmpfloat<1024>(std::numeric_limits<__float80>::epsilon()) >
          gmpfloat<1024>(1.0));

  typedef gmpfloat<1024> x;
  long double one{1.0};
  long double eps = approxepsilon<long double>();
  auto bitmore = x(1.0) + x(eps);
  std::cout << "converting " << bitmore << " to ld: ";
  long double res{bitmore};
  std::cout << "got " << res << "." << std::endl;
  std::cout << (bitmore > x(1.0)) << std::endl;
  std::cout << res << std::endl;
  std::cout << one + eps << std::endl;
  std::cout << (one + eps > one) << std::endl;
  std::cout << (res > one) << std::endl;
}

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;

  test_gmpfloat();

  test_float_type<float>();
  test_float_type<double>();
  test_float_type<long double>();
  test_float_type<doubledouble<float>>();
  test_float_type<doubledouble<double>>();
  test_float_type<doubledouble<long double>>();
  test_float_type<__float80>();
  test_float_type<doubledouble<__float80>>();
  test_float_type<__float128>();
  test_float_type<doubledouble<__float128>>();
  test_float_type<gmpfloat<128>>();
  test_float_type<gmpfloat<256>>();
  test_float_type<mpfrfloat<128, MPFR_RNDD>>();
  test_float_type<mpfrfloat<256, MPFR_RNDD>>();
  // test_float_type<mpfrfloat<128, MPFR_RNDA>>();
  // test_float_type<mpfrfloat<256, MPFR_RNDA>>();
  return 0;

  // printf("float %e\n", std::numeric_limits<float>::epsilon());
  // printf("double %e\n", std::numeric_limits<double>::epsilon());
  // printf("long double %Le\n", std::numeric_limits<long double>::epsilon());
  // printf("__float80 %Le\n", std::numeric_limits<__float80>::epsilon());
  // // printf("__float128 %Le\n", std::numeric_limits<__float128>::epsilon());
  // gmpfloat flt(1.0);
  // long double fl2{1.0};
  // for (int i = 0; i < 10000; ++i) {
  //   flt = flt * gmpfloat(1e-1);
  //   fl2 = fl2 * 1e-1;
  //   gmp_printf("%Fe %Le %Le %e\n", flt.mpf, fl2, (long double)flt,
  //   (double)flt);
  // }
  // return 0;
}
