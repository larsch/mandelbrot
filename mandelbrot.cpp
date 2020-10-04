/**
 * @file mandelbrot.cpp
 *
 * Optimized mandelbrot set explorer.
 *
 * Copyright 2020 by Lars Christensen <larsch@belunktum.dk>
 */

#if HAVE_LIBGMP
#include "gmpfloat.hpp"
#endif
#if HAVE_LIBMPFR
#include "mpfrfloat.hpp"
#endif
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <quadmath.h>

#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>

#include "doubledouble.hpp"
#include "float.hpp"
#include "mandelbrot.hpp"
#include "semaphore.hpp"
#include "strop.hpp"
#include "typenames.hpp"

#include "application.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#if 0
static std::mutex log_mutex;
#define LOG(x)                                    \
  do {                                            \
    std::unique_lock<std::mutex> lock(log_mutex); \
    std::cout << x << std::endl;                  \
  } while (0)
#else
#define LOG(x)
#endif


// /**
//  * Position as stored in bookmarks
//  */
// struct pos {
//   flt x;
//   flt y;
//   flt s;
// };

// struct pos saves[10];

// /**
//  * Load bookmarks
//  */
// void load() {
//   std::ifstream inf("bookmarks");
//   if (inf) {
//     if (inf.read(reinterpret_cast<char *>(&saves), sizeof(saves))) {
//     }
//   }
// }

// /**
//  * Save bookmark
//  */
// void save(int slot) {
//   load();
//   saves[slot] = pos{center_x, center_y, screen_size};
//   std::ofstream of("bookmarks");
//   if (of) {
//     if (of.write(reinterpret_cast<const char *>(&saves), sizeof(saves))) {
//     }
//   }
// }

// /**
//  * Load bookmark
//  */
// void load(int slot) {
//   load();
//   center_x = saves[slot].x;
//   center_y = saves[slot].y;
//   screen_size = saves[slot].s;
// }

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  try {
    mandelbrot_application app;
    app.run();

    for (int i = 0; i < 10; ++i) {
      // mp_exp_t exp;
      // mpf_init2(saves[i].x.mpf, 256);
      // mpf_set_d(saves[i].x.mpf, 3.14159265);
      // char *str = mpf_get_str(0, &exp, 10, 0, saves[i].x.mpf);
      // std::cout << i << "," << str << "," << exp << std::endl;
      // int exp;
      // std::frexp(saves[i].x, &exp);
      // printf("%d %.36f\n", i, (double)saves[i].x);
      // std::cout << float2string(saves[i].x) << " " <<
      // float2string(saves[i].y) << " " << float2string(saves[i].s) <<
      // std::endl;

      // char str[256];
      // __float128 f = 3.141592653;
      // auto r = quadmath_snprintf(str, 256, "%.20Qe", f);
      // std::cout << "result=" << r << std::endl;
      // std::cout << str << std::endl;
    }
    return EXIT_SUCCESS;
  } catch (const std::exception &exc) {
    std::cerr << exc.what() << std::endl;
    return EXIT_FAILURE;
  }
}
