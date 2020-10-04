/**
 * @file floattype.cpp
 */

#include "floattype.hpp"

const char *floattypenames[FT_MAX] = {
    "auto",                  // FT_AUTO
    "float",                 // FT_FLOAT
    "double",                // FT_DOUBLE
    "doubledouble<float>",   // FT_DOUBLEFLOAT
    "doubledouble<double>",  // FT_DOUBLEDOUBLE
#if HAVE_FLOAT80
    "__float80",                // FT_FLOAT80
    "doubledouble<__float80>",  // FT_DOUBLEFLOAT80
#endif
#if HAVE_FLOAT128
    "__float128",                // FT_FLOAT80
    "doubledouble<__float128>",  // FT_DOULEFLOAT80
#endif
#if HAVE_LONG_DOUBLE
    "long double",                // FT_LONGDOUBLE
    "doubledouble<long double>",  // FT_DOUBLELONG_DOUBLE,
#endif
#if HAVE_LIBGMP
    "gmpfloat<128>",  // FT_GMPFLOAT128
    "gmpfloat<256>",  // FT_GMPFLOAT256
#endif
#if HAVE_LIBMPFR
    "mpfrfloat<128>",
    "mpfrfloat<256>",
#endif
};
