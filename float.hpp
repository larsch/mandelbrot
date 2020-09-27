#ifndef _float_hpp
#define _float_hpp

#if HAVE_LIBGMP
#include "gmpfloat.hpp"
typedef gmpfloat<256> flt;
#elif HAVE_FLOAT128
typedef __float128 flt;
#elif HAVE_FLOAT80
typedef __float80 flt;
#else
typedef double flt;
#endif

#endif  // _float_hpp
