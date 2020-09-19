#include <chrono>
#include <iostream>

#include "mandelbrot.hpp"
#if HAVE_LIBGMP
#include <gmpxx.h>

#include "gmpfloat.hpp"
double get_double(const mpf_class& f) { return f.get_d(); }
#endif

static const int MIN_DURATION = 1000;
static const int INNER_ITERATIONS = 32768;

template <typename FLT>
void benchmark(const char* type) {
  FLT xc = FLT(0.25 + (0.1 * rand()) / RAND_MAX);
  FLT yc = FLT(0.25 + (0.1 * rand()) / RAND_MAX);

  int sum;
  int iterations = 100;
  std::cout << type << ": " << std::flush;
  while (true) {
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
      for (int j = 0; j < INNER_ITERATIONS; ++j) {
        auto result = iter(xc, yc);
        sum += result.iterations;
      }
    }
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::high_resolution_clock::now() - start)
                        .count();
    if (duration >= MIN_DURATION) {
      auto iterations_per_second = iterations * 1000 / duration;
      std::cout << "\r" << sum << "\r" << type << ": " << iterations_per_second
                << " iteration/sec" << std::endl;
      break;
    } else if (duration <= 8) {
      iterations *= 8;
    } else {
      iterations = (iterations * MIN_DURATION * 5 / duration) / 4;
    }
  }
}

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  benchmark<float>("float");
  benchmark<double>("double");
#if HAVE_FLOAT80
  benchmark<__float80>("__float80");
#endif
#if HAVE_FLOAT128
  benchmark<__float128>("__float128");
#endif
#if HAVE_LIBGMP
  benchmark<gmpfloat>("gmpfloat");
  benchmark<mpf_class>("mpf_class");
#endif
}