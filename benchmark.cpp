#include <chrono>
#include <ctime>
#include <iostream>

#include "mandelbrot.hpp"
#if HAVE_LIBGMP
#include <gmpxx.h>

#include "gmpfloat.hpp"
double get_double(const mpf_class& f) { return f.get_d(); }
#endif

#include "mpfrfloat.hpp"

static const int MIN_DURATION = 1000;
static const int INNER_ITERATIONS = 32768;

template <typename FLT>
void benchmark(const char* type, double xc0, double yc0) {
  // FLT xc = FLT(xc0);
  // FLT yc = FLT(yc0);

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
      std::cout << std::endl;
      break;
    } else if (duration <= 8) {
      iterations *= 8;
    } else {
      iterations = (iterations * MIN_DURATION * 5 / duration) / 4;
    }
  }
}

#define BENCHMARK(type) benchmark<type>(#type, xc, yc)

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  srand(time(NULL));
  double xc = 0.01 + (0.1 * rand() / RAND_MAX);
  double yc = 0.01 + (0.1 * rand() / RAND_MAX);

  benchmark<float>("float", xc, yc);
  benchmark<double>("double", xc, yc);
  BENCHMARK(float);
  BENCHMARK(double);
#if HAVE_FLOAT80
  BENCHMARK(__float80);
#endif
#if HAVE_FLOAT128
  BENCHMARK(__float128);
#endif
#if HAVE_LIBGMP
  BENCHMARK(gmpfloat);
  BENCHMARK(mpf_class);
#endif
  benchmark<mpfrfloat<256, MPFR_RNDN>>("mpfrfloat<256, MPFR_RNDN>", xc, yc);
  benchmark<mpfrfloat<256, MPFR_RNDZ>>("mpfrfloat<256, MPFR_RNDZ>", xc, yc);
  benchmark<mpfrfloat<256, MPFR_RNDU>>("mpfrfloat<256, MPFR_RNDU>", xc, yc);
  benchmark<mpfrfloat<256, MPFR_RNDD>>("mpfrfloat<256, MPFR_RNDD>", xc, yc);
  benchmark<mpfrfloat<256, MPFR_RNDA>>("mpfrfloat<256, MPFR_RNDA>", xc, yc);
  benchmark<mpfrfloat<256, MPFR_RNDF>>("mpfrfloat<256, MPFR_RNDF>", xc, yc);
  benchmark<mpfrfloat<256, MPFR_RNDNA>>("mpfrfloat<256, MPFR_RNDNA>", xc, yc);
}