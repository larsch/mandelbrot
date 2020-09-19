#include <gmp.h>
#include "float.hpp"

class gmpfloat {
 public:
  gmpfloat() { mpf_init(mpf); }
  gmpfloat(const gmpfloat& g) { mpf_init_set(mpf, g.mpf); }
  gmpfloat& operator=(const gmpfloat& b) { mpf_clear(mpf); mpf_init_set(mpf, b.mpf); return *this; }
  ~gmpfloat() { mpf_clear(mpf); }
   gmpfloat(mpf_t w) { mpf[0] = w[0]; }
   gmpfloat(flt _f) { mpf_init_set_d(mpf, _f); }
   gmpfloat(double _f) { mpf_init_set_d(mpf, _f); }
   gmpfloat(int i) { mpf_init_set_si(mpf, i); }
   gmpfloat(unsigned int i) { mpf_init_set_ui(mpf, i); }
  operator double() const { return mpf_get_d(mpf); }
  gmpfloat& operator+=(const gmpfloat& o) { mpf_add(mpf, mpf, o.mpf); return *this; }
  gmpfloat& operator-=(const gmpfloat& o) { mpf_sub(mpf, mpf, o.mpf); return *this; }
  mpf_t mpf;
};

gmpfloat operator*(const gmpfloat& f, const gmpfloat &g) {
  mpf_t r;
  mpf_init(r);
  mpf_mul(r, f.mpf, g.mpf);
  return gmpfloat(r);
}

gmpfloat operator+(const gmpfloat& f, const gmpfloat &g) {
  mpf_t r;
  mpf_init(r);
  mpf_add(r, f.mpf, g.mpf);
  return gmpfloat(r);
}

gmpfloat operator-(const gmpfloat& f, const gmpfloat &g) {
  mpf_t r;
  mpf_init(r);
  mpf_sub(r, f.mpf, g.mpf);
  return gmpfloat(r);
}
