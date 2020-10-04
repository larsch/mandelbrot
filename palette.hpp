#ifndef _palette_hpp
#define _palette_hpp

#include <cstdint>
#include <cstddef>

/**
 * Palette
 */
class palette {
 public:
  uint32_t pal[256];
  palette();
  uint32_t operator[](size_t i) { return pal[i]; }
};

#endif  // _palette_hpp
