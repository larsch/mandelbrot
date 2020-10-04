#include "palette.hpp"

#include <cmath>

#define RGB(r, g, b) (((r) << 16) | ((g) << 8) | ((b)))

/**
 * Convert HSV to RGB color.
 *
 * @param h Hue (0-359)
 * @param s Saturation (0.0-1.0)
 * @param v Value/lightness (0.0-1.0)
 */
static uint32_t hsv2rgb(int h, float s, float v) {
  float hm = h / 60.0;
  float c = v * s;
  float x = c * (1.0 - std::abs(std::fmod(hm, 2.0) - 1.0));
  int c1 = int(255.999 * c);
  int x1 = int(255.999 * x);
  switch (int(hm)) {
    case 0:
      return RGB(c1, x1, 0);
    case 1:
      return RGB(x1, c1, 0);
    case 2:
      return RGB(0, c1, x1);
    case 3:
      return RGB(0, x1, c1);
    case 4:
      return RGB(x1, 0, c1);
    default:
      return RGB(c1, 0, x1);
  }
}

palette::palette() {
  for (int i = 0; i < 256; i++) {
    int h = fmod((i * 360.0) / 64, 360);
    float v = 0.6 + 0.3 * sin(i / 16.0 * M_PI);
    float s = 0.75 + 0.23 * cos(i / 8.0 * M_PI);
    pal[i] = hsv2rgb(h, s, v);
  }
}
