/**
 * @file mandelbrot.hpp
 *
 * Optimized mandelbrot set explorer.
 *
 * Copyright 2020 by Lars Christensen <larsch@belunktum.dk>
 */

#ifndef _mandelbrot_hpp
#define _mandelbrot_hpp

#include <cmath>

const int LIMIT = 2048;

template<typename FLT>
double get_double(const FLT& f) {
    return double(f);
}

/**
 * Check if x,y is inside some of the obvious areas of the mandelbrot set. This
 * improves performance greatly when rendering initial screen where the main set
 * is visible.
 */
template <typename FLT>
bool isinside(FLT x, FLT y) {
  FLT absy = FLT(abs(get_double(y)));
  if (x > -0.75 && absy < 0.75) {
    // check if x,y is inside the main cardioid
    x -= FLT(0.25);
    FLT c1 = (x * x + y * y);
    FLT a = FLT(0.25);
    FLT c2 = c1 * c1 + FLT(4.0) * a * x * c1 - FLT(4.0) * a * a * y * y;
    return c2 < 0;
  } else if (x > -1.25 && absy < 0.25) {
    // check if x,y is inside the left circle
    x += FLT(1);
    return x * x + y * y < 0.0625;
  } else {
    return false;
  }
}

/**
 * Result of iteration of the function, including the squares of the x/y of the
 * result, used to calculate the fraction.
 */
template <typename FLT>
struct iter_result {
  unsigned int iterations;
  FLT x;
  FLT y;
};

/**
 * Perform mandelbrot iterations and return the number of iteration required
 * before escape.
 */
template <typename FLT>
iter_result<FLT> iter(FLT xc, FLT yc) {
  FLT x = xc;
  FLT y = yc;
  if (isinside(xc, yc)) return {LIMIT, 0, 0};

  unsigned int iterations = 0;
  FLT x2 = x * x;
  FLT y2 = y * y;

  while (x2 + y2 < 4.0 && ++iterations < LIMIT) {
    y = x * y * FLT(2.0) + yc;
    x = x2 - y2 + xc;
    x2 = x * x;
    y2 = y * y;
  }

  for (int j = 0; j < 4; ++j) {
    y = x * y * FLT(2) + yc;
    x = x2 - y2 + xc;
    x2 = x * x;
    y2 = y * y;
  }

  return {iterations, x2, y2};
}

template <typename FLT>
double fraction(FLT zx2, FLT zy2) {
  const double log2Inverse = 1.0 / log(2.0);
  const double logHalflog2Inverse = log(0.5) * log2Inverse;
  return 5.0 - logHalflog2Inverse -
         log(log(get_double(zx2) + get_double(zy2))) * log2Inverse;
}

#endif // _mandelbrot_hpp