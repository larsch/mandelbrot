#if HAVE_LIBGMP
#include "gmpfloat.hpp"
typedef gmpfloat<1024> flt;
#elif HAVE_FLOAT128
typedef __float128 flt;
#elif HAVE_FLOAT80
typedef __float80 flt;
#else
typedef double flt;
#endif
