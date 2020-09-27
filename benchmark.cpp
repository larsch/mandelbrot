#include <chrono>
#include <ctime>
#include <iostream>

#if HAVE_LIBGMP
#include <gmpxx.h>

namespace std {
mpf_class abs(mpf_class f) { return ::abs(f); }
}  // namespace std

#include "gmpfloat.hpp"
double get_double(const mpf_class& f) { return f.get_d(); }
#endif

#include "doubledouble.hpp"
#include "mpfrfloat.hpp"
#include "mandelbrot.hpp"
#include "strop.hpp"

static const int MIN_DURATION = 1000;
static const int INNER_ITERATIONS = 32768;

template <typename FLT>
FLT approxepsilon() {
  FLT one{1.0};
  FLT epsilon{1.0};
  FLT half{0.5};
  while (one + half * epsilon != one) {
    epsilon = half * epsilon;
  }
  return epsilon;
}

template <typename FLT>
void benchmark(const char* type) {
  std::cout << type << ": " << std::flush;
  int iterations = 16;
  while (true) {
    unsigned long sum = 0;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
      for (int j = 0; j < INNER_ITERATIONS; ++j) {
        FLT xc = FLT(0.01 + (0.1 * rand() / RAND_MAX));
        FLT yc = FLT(0.01 + (0.1 * rand() / RAND_MAX));
        auto result = iter(xc, yc);
        sum += result.iterations;
      }
    }
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::high_resolution_clock::now() - start)
                        .count();
    if (duration >= MIN_DURATION) {
      long int iterations_per_millisecond =
          iterations * INNER_ITERATIONS / duration;
      std::cout << "\r" << sum / iterations << " - " << type << ": "
                << iterations_per_millisecond << " iteration/msec";
      std::cout << " " << iterations << " in " << duration << " milliseconds";
      std::cout << ",size=" << sizeof(FLT)
                << ",epsilon=" << std::numeric_limits<FLT>::epsilon();
      std::cout << ",aepsilon=" << approxepsilon<FLT>();
      std::cout << std::endl;
      break;
    } else if (duration <= 8) {
      iterations *= 8;
    } else {
      iterations = (iterations * MIN_DURATION * 5 / duration) / 4;
    }
  }
}

#define BENCHMARK(type) benchmark<type>(#type)

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  srand(time(NULL));

  BENCHMARK(float);
  BENCHMARK(double);
#if HAVE_FLOAT80
  BENCHMARK(__float80);
#endif
  BENCHMARK(doubledouble<float>);
  BENCHMARK(doubledouble<double>);
#if HAVE_FLOAT128
  BENCHMARK(__float128);
#endif
#if HAVE_LONG_DOUBLE
  BENCHMARK(long double);
#endif
  BENCHMARK(doubledouble<long double>);
  BENCHMARK(doubledouble<__float80>);
  BENCHMARK(doubledouble<__float128>);
#if HAVE_LIBGMP
  BENCHMARK(gmpfloat<128>);
  BENCHMARK(gmpfloat<256>);
  BENCHMARK(mpf_class);
#endif
  benchmark<mpfrfloat<128, MPFR_RNDN>>("mpfrfloat<128, MPFR_RNDN>");
  benchmark<mpfrfloat<128, MPFR_RNDZ>>("mpfrfloat<128, MPFR_RNDZ>");
  benchmark<mpfrfloat<256, MPFR_RNDN>>("mpfrfloat<256, MPFR_RNDN>");
  benchmark<mpfrfloat<256, MPFR_RNDZ>>("mpfrfloat<256, MPFR_RNDZ>");
  // benchmark<mpfrfloat<256, MPFR_RNDU>>("mpfrfloat<256, MPFR_RNDU>");
  // benchmark<mpfrfloat<256, MPFR_RNDD>>("mpfrfloat<256, MPFR_RNDD>");
  // benchmark<mpfrfloat<256, MPFR_RNDA>>("mpfrfloat<256, MPFR_RNDA>");
  // benchmark<mpfrfloat<256, MPFR_RNDF>>("mpfrfloat<256, MPFR_RNDF>");
  // benchmark<mpfrfloat<256, MPFR_RNDNA>>("mpfrfloat<256, MPFR_RNDNA>");
}
