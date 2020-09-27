#ifndef _floatext_hpp
#define _floatext_hpp

namespace std {

template <>
class numeric_limits<__float128> {
 public:
  static const size_t digits10 = 33;
  static const size_t digits = 113;
  static constexpr __float128 epsilon() {
    return 0.1925929944387235853055977942584927318538e-33;
  }
};

__float128 abs(__float128 flt) {
  if (flt < __float128(0.0)) {
    return -flt;
  } else {
    return flt;
  }
}

}  // namespace std

#endif  // _floatext_hpp
