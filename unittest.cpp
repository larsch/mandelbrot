#include "gmpfloat.hpp"
#include "mpfrfloat.hpp"
#include "typenames.hpp"
// #include "format.hpp"
#include <cmath>
#include <limits>
#include <sstream>

#include "floatext.hpp"
#include "strop.hpp"

static unsigned int assert_count = 0;
static unsigned int assert_failures = 0;

static bool debug_enabled = false;
#define DBG(expr)      \
  if (debug_enabled) { \
    std::cout << expr; \
  }

#define assert_flt(expr)                                               \
  {                                                                    \
    ++assert_count;                                                    \
    std::cout << tname<FLT>() << "," << sizeof(FLT) << ","             \
              << approxepsilon<FLT>() << ": " << #expr << std::endl;   \
    if (!(expr)) {                                                     \
      ++assert_failures;                                               \
      std::cout << "Failed: " #expr << " (FLT=" << tname<FLT>() << ")" \
                << std::endl;                                          \
    }                                                                  \
  }

#define assert(expr)                              \
  {                                               \
    ++assert_count;                               \
    std::cout << #expr << std::endl;              \
    if (!(expr)) {                                \
      ++assert_failures;                          \
      std::cout << "Failed: " #expr << std::endl; \
    }                                             \
  }

template <typename FLT>
constexpr FLT approxepsilon() {
  FLT one{1.0};
  FLT epsilon{1.0};
  FLT half{0.5};
  DBG(tname<FLT>() << " ********");
  while (one + half * epsilon != one) {
    epsilon = half * epsilon;
    DBG(epsilon);
  }
  return epsilon;
}

template <typename FLT>
void test_float_type() {
  const auto eps = std::numeric_limits<FLT>::epsilon();
  assert_flt(eps != FLT(0.0));
  assert_flt(FLT(1.0) + FLT(1.0) - FLT(2.0) < eps);
  assert_flt(FLT(1.0) - FLT(1.0) < eps);
  assert_flt(FLT(1.0) - eps < FLT(1.0));
  assert_flt(FLT(1.0) + eps > FLT(1.0));
  assert_flt(FLT(1.0) + eps != FLT(1.0));
  assert_flt(FLT(1.0) - eps != FLT(1.0));
  assert_flt(FLT(1.0) == FLT(1.0));
  assert_flt(std::abs(FLT(1.0)) == FLT(1.0));
  assert_flt(std::abs(FLT(-1.0)) == FLT(1.0));

  auto x = std::abs(FLT{99.9});
  FLT* ptr = &x;  // std::abs must return FLT type
  (void)ptr;
  DBG("epsilon: " << tname<FLT>() << ": "
             << std::numeric_limits<FLT>::epsilon()
             << ", approx: " << approxepsilon<FLT>());
}

void test_gmpfloat() {
  assert(mpf_get_prec((gmpfloat<128>(8.0) * gmpfloat<128>(4.0)).mpf) == 128);
  assert(mpf_get_prec((gmpfloat<256>(8.0) * gmpfloat<256>(4.0)).mpf) == 256);
  assert(gmpfloat<1024>(1.0) +
             gmpfloat<1024>(std::numeric_limits<__float80>::epsilon()) >
         gmpfloat<1024>(1.0));
}

/**
 * Should be possible to stream a float type in and out without losing
 * information.
 */
template <typename FLT>
void test_format() {
  {
    FLT one{1.0};
    FLT event{7.0};
    auto one_seventh = (one / event);
    std::ostringstream ostr;
    DBG("digits: " << std::numeric_limits<FLT>::digits10);
    // std::endl;
    ostr.precision(std::numeric_limits<FLT>::digits10 + 2);
    ostr << one_seventh;
    DBG("str: " << ostr.str());
    std::istringstream istr{ostr.str()};
    FLT input;
    istr >> input;
    std::cout.precision(std::numeric_limits<FLT>::digits10 + 2);
    DBG("compare: " << one_seventh << " =? " << input);
    DBG("equal: " << (input == one_seventh));
    DBG("diff: " << (one_seventh - input));
    DBG("diff: " << (input - one_seventh));
    DBG("absdiff: " << std::abs(input - one_seventh));
    auto epsilon = std::numeric_limits<FLT>::epsilon();
    DBG("epsilon: " << epsilon);
    assert_flt(std::abs(input - one_seventh) < epsilon);
  }
  {
    FLT one{-1.0};
    FLT event{7.0};
    auto one_seventh = (one / event);
    std::ostringstream ostr;
    DBG("digits: " << std::numeric_limits<FLT>::digits10);
    // std::endl;
    ostr.precision(std::numeric_limits<FLT>::digits10 + 2);
    ostr << one_seventh;
    DBG("str: " << ostr.str());
    std::istringstream istr{ostr.str()};
    FLT input;
    istr >> input;
    std::cout.precision(std::numeric_limits<FLT>::digits10 + 2);
    DBG("compare: " << one_seventh << " =? " << input);
    DBG("diff: " << (one_seventh - input));
    DBG("diff: " << (input - one_seventh));
    DBG("absdiff: " << std::abs(input - one_seventh));
    auto epsilon = std::numeric_limits<FLT>::epsilon();
    DBG("epsilon: " << epsilon);
    assert_flt(std::abs(input - one_seventh) < epsilon);
  }
}

/**
 * Converting to a higher precision float point type and back should retain the
 * original value.
 */
template <typename FLT, typename FLT2>
void test_convert_up_down() {
  FLT one{1.0};
  FLT event{7.0};
  auto one_seventh = (one / event);
  DBG("orig: " << one_seventh);

  FLT2 up{one_seventh};
  DBG("  up: " << up);

  FLT down{up};
  DBG("down: " << down);

  DBG("diff: " << (down - one_seventh));
  assert_flt(down == one_seventh);
}

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;

  test_convert_up_down<
      doubledouble<float>,
      gmpfloat<std::numeric_limits<doubledouble<float>>::digits>>();
  test_convert_up_down<
      doubledouble<double>,
      gmpfloat<std::numeric_limits<doubledouble<double>>::digits>>();
  test_convert_up_down<
      doubledouble<long double>,
      gmpfloat<std::numeric_limits<doubledouble<long double>>::digits>>();
  test_convert_up_down<
      doubledouble<__float128>,
      gmpfloat<std::numeric_limits<doubledouble<__float128>>::digits>>();

  test_convert_up_down<
      doubledouble<float>,
      mpfrfloat<std::numeric_limits<doubledouble<float>>::digits, MPFR_RNDD>>();
  test_convert_up_down<
      doubledouble<double>,
      mpfrfloat<std::numeric_limits<doubledouble<double>>::digits,
                MPFR_RNDD>>();
  test_convert_up_down<
      doubledouble<long double>,
      mpfrfloat<std::numeric_limits<doubledouble<long double>>::digits,
                MPFR_RNDD>>();
  test_convert_up_down<
      doubledouble<__float128>,
      mpfrfloat<std::numeric_limits<doubledouble<__float128>>::digits,
                MPFR_RNDD>>();

  test_convert_up_down<float, doubledouble<float>>();
  test_convert_up_down<double, doubledouble<double>>();
  test_convert_up_down<long double, doubledouble<long double>>();
  test_convert_up_down<__float128, doubledouble<__float128>>();

  test_convert_up_down<float, gmpfloat<128>>();
  test_convert_up_down<double, gmpfloat<128>>();
  test_convert_up_down<long double, gmpfloat<128>>();
  test_convert_up_down<__float128, gmpfloat<128>>();

  test_convert_up_down<float, mpfrfloat<128, MPFR_RNDD>>();
  test_convert_up_down<double, mpfrfloat<128, MPFR_RNDD>>();
  test_convert_up_down<long double, mpfrfloat<128, MPFR_RNDD>>();
  test_convert_up_down<__float128, mpfrfloat<128, MPFR_RNDD>>();

  test_format<float>();
  test_format<double>();
  test_format<long double>();
  test_format<__float80>();
  test_format<__float128>();
  test_format<doubledouble<double>>();
  test_format<doubledouble<float>>();
  test_format<gmpfloat<128>>();
  test_format<gmpfloat<256>>();
  test_format<mpfrfloat<128, MPFR_RNDD>>();
  test_format<mpfrfloat<256, MPFR_RNDD>>();
  test_format<doubledouble<__float80>>();
  test_format<doubledouble<__float128>>();

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

  std::cout << assert_count << " assertions, " << assert_failures << " failures"
            << std::endl;
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
