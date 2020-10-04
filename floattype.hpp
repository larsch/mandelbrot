/**
 * @file floattype.hpp
 */

#ifndef _floattype_hpp
#define _floattype_hpp

/// Enumeration of all possible floating point types that can be used for
/// rendering.
enum FloatType {
  FT_AUTO,
  FT_FLOAT,
  FT_DOUBLE,
  FT_DOUBLEFLOAT,
  FT_DOUBLEDOUBLE,
#if HAVE_FLOAT80
  FT_FLOAT80,
  FT_DOUBLEFLOAT80,
#endif
#if HAVE_FLOAT128
  FT_FLOAT128,
  FT_DOUBLEFLOAT128,
#endif
#if HAVE_LONG_DOUBLE
  FT_LONG_DOUBLE,
  FT_DOUBLELONG_DOUBLE,
#endif
#if HAVE_LIBGMP
  FT_GMPFLOAT128,
  FT_GMPFLOAT256,
#endif
#if HAVE_LIBMPFR
  FT_MPFRFLOAT128,
  FT_MPFRFLOAT256,
#endif
  FT_MAX
};

/// String names for each floating point type
extern const char *floattypenames[FT_MAX];

#endif  // _floattype_hpp
