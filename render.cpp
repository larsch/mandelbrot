#include "render.hpp"

#include <SDL2/SDL.h>

#include <cstring>
#include <thread>
#include <vector>

#include "application.hpp"
#include "floattype.hpp"
#include "mandelbrot.hpp"
#include "palette.hpp"
#include "semaphore.hpp"

flt center_x{-0.60};
flt center_y{0};
flt screen_size{2.0};
uint8_t *pixels = nullptr;
int w;
flt min_x;
flt min_y;
flt pixel_size;
int pitch;
semaphore jobs_left;
std::atomic_int next_job = 0;
semaphore jobs_done;
int virtual_rows = 0;
int rows = 0;
int row_bits;
bool rendering = false;
std::atomic_bool running = true;
int jobs_remaining = 0;
FloatType user_chosen_float_type = FT_AUTO; /** Type chosen by user */
FloatType render_float_type = FT_AUTO;      /**< Type used for render */
std::vector<std::thread> threads;

static palette pal;

/// Map sequentially numbered row to order to reverse bit order. Lets us render
/// all parts of the screen at the same time instead of top to bottom.
static int maprow(int row) {
  row = row % virtual_rows;
  int res = 0;
  for (int i = 0; i < row_bits; ++i) {
    res = (res << 1) | (row & 1);
    row >>= 1;
  }
  return res;
}

void notify_row_complete(int row) {
  SDL_Event event;
  memset(&event, 0, sizeof(event));
  event.type = ROWRENDER_COMPLETE_EVENT;
  event.user.code = row;
  SDL_PushEvent(&event);
}

/// Blend two colors by fraction of each.
uint32_t blend(uint32_t c1, uint32_t c2, float f) {
  uint8_t r1 = c1 >> 24;
  uint8_t g1 = (c1 >> 16) & 0xff;
  uint8_t b1 = (c1 >> 8) & 0xff;
  uint8_t a1 = (c1)&0xff;
  uint8_t r2 = c2 >> 24;
  uint8_t g2 = (c2 >> 16) & 0xff;
  uint8_t b2 = (c2 >> 8) & 0xff;
  uint8_t a2 = (c2)&0xff;
  int s1 = 255 * f;
  int s2 = 255 - s1;
  uint8_t r3 = (r1 * s1 + r2 * s2) / 256;
  uint8_t g3 = (g1 * s1 + g2 * s2) / 256;
  uint8_t b3 = (b1 * s1 + b2 * s2) / 256;
  uint8_t a3 = (a1 * s1 + a2 * s2) / 256;
  return (r3 << 24) | (g3 << 16) | (b3 << 8) | (a3);
}

/**
 * Render a single row of the mandelbrot set
 */
template <typename FLT>
void render_rowx(int row) {
  const FLT scl = FLT(pixel_size);
  const FLT minx = FLT(min_x);
  const FLT miny = FLT(min_y);
  row = maprow(row);
  if (row < rows) {
    FLT yc = miny + FLT(row) * scl;
    uint32_t *row_pixels = reinterpret_cast<uint32_t *>(pixels + row * pitch);
    for (int col = 0; col < w; ++col) {
      auto result = iter(minx + FLT(col) * scl, yc);
      if (result.iterations == LIMIT) {
        *row_pixels++ = 0x00;
      } else {
        FLT zx2 = result.x;
        FLT zy2 = result.y;
        double sum = result.iterations + fraction(zx2, zy2);
        unsigned int n = (unsigned int)floor(double(sum));
        double f2 = sum - double(n);
        unsigned int n1 = n % 256;
        double f1 = 1.0 - f2;
        unsigned int n2 = ((n1 + 1) % 256);
        *row_pixels++ = blend(pal[n1], pal[n2], f1);
      }
    }
  }

  notify_row_complete(row);
}

/// Shorthand for epsilon
template <typename FLT>
flt epsilon() {
  // return std::numeric_limits<FLT>::epsilon();
  return flt(std::numeric_limits<FLT>::epsilon());
};

/// Chose the floating point type that is the fastest at the require precision.
FloatType determine_type() {
  if (pixel_size > epsilon<float>())
    return FT_FLOAT;
  else if (pixel_size > epsilon<double>())
    return FT_DOUBLE;
  else if (pixel_size > epsilon<long double>())
    return FT_LONG_DOUBLE;
  else if (pixel_size > epsilon<__float80>())
    return FT_FLOAT80;
  else if (pixel_size > epsilon<doubledouble<float>>())
    return FT_DOUBLEFLOAT;
  else if (pixel_size > epsilon<doubledouble<double>>())
    return FT_DOUBLEDOUBLE;
  else if (pixel_size > epsilon<doubledouble<__float80>>())
    return FT_DOUBLEFLOAT80;
  else if (pixel_size > epsilon<__float128>())
    return FT_FLOAT128;
  else if (pixel_size > epsilon<doubledouble<long double>>())
    return FT_LONG_DOUBLE;
  else if (pixel_size > epsilon<doubledouble<__float128>>())
    return FT_DOUBLEFLOAT128;
  else if (pixel_size > epsilon<gmpfloat<128>>())
    return FT_GMPFLOAT128;
  else
    return FT_GMPFLOAT256;
}

/**
 * Render specified row using selected floating point type.
 */
void render_row(int row) {
  switch (render_float_type) {
    case FT_FLOAT:
      render_rowx<float>(row);
      break;
    case FT_DOUBLE:
      render_rowx<double>(row);
      break;
    case FT_DOUBLEDOUBLE:
      render_rowx<doubledouble<double>>(row);
      break;
#if HAVE_FLOAT80
    case FT_FLOAT80:
      render_rowx<__float80>(row);
      break;
    case FT_DOUBLEFLOAT80:
      render_rowx<doubledouble<__float80>>(row);
      break;
#endif
#if HAVE_FLOAT128
    case FT_FLOAT128:
      render_rowx<__float128>(row);
      break;
    case FT_DOUBLEFLOAT128:
      render_rowx<doubledouble<__float128>>(row);
      break;
#endif
#if HAVE_LONG_DOUBLE
    case FT_LONG_DOUBLE:
      render_rowx<long double>(row);
      break;
#endif
#if HAVE_LIBGMP
    case FT_GMPFLOAT128:
      render_rowx<gmpfloat<128>>(row);
      break;
    case FT_GMPFLOAT256:
      render_rowx<gmpfloat<256>>(row);
      break;
#endif
#if HAVE_LIBMPFR
    case FT_MPFRFLOAT128:
      render_rowx<mpfrfloat<128>>(row);
      break;
    case FT_MPFRFLOAT256:
      render_rowx<mpfrfloat<256>>(row);
      break;
#endif
    default:
      abort();
  }
}

/**
 * Cancel current rendering progress. Auto to abort render when rending
 * parameters change.
 */
void cancel_render() {
  if (!rendering) return;
  // Steal remaining jobs
  while (jobs_left.try_acquire()) jobs_done.release(1);

  // Wait for all workers to complete
  for (int i = 0; i < virtual_rows; ++i) jobs_done.acquire();

  rendering = false;
}

/**
 * Start render
 */
void start_render() {
  pixel_size = screen_size / flt(rows);
  render_float_type = user_chosen_float_type == FT_AUTO
                          ? determine_type()
                          : user_chosen_float_type;
  min_x = center_x - w * pixel_size / flt(2.0);
  min_y = center_y - rows * pixel_size / flt(2.0);

  // std::cout.precision(std::numeric_limits<decltype(min_x)>::digits10);
  // std::cout << "type: " << tname<decltype(min_x)>() << std::endl;
  // std::cout << "digits: " << std::numeric_limits<decltype(min_x)>::digits10
  //           << std::endl;
  // std::cout << "min_x: " << min_x << std::endl;
  // std::cout << "min_y: " << min_y << std::endl;

  row_bits = log2(rows) + 1;
  virtual_rows = 1 << row_bits;
  jobs_left.release(virtual_rows);
  jobs_remaining = virtual_rows;
  rendering = true;
}

/**
 * Rendering worker main function.
 */
void worker() {
  while (true) {
    jobs_left.acquire();
    if (!running) {
      break;
    }
    auto job = next_job++;
    render_row(job);
    jobs_done.release(1);
  }
}

void render_init() {
  const int thread_count = std::thread::hardware_concurrency();
  std::cout << "starting " << thread_count << " workers" << std::endl;
  for (int i = 0; i < thread_count; ++i) {
    threads.push_back(std::thread(worker));
  }
}

void render_reconfigure(int width, int height) {
  delete[] pixels;
  pixels = new uint8_t[width * height * 4];
  pitch = width * 4;
  rows = height;
  w = width;
}

void render_stop() {
  jobs_left.release(threads.size());
  for (auto &thr : threads) thr.join();
}

void render_copy_pixels(void *dest, size_t offset, size_t length) {
  memcpy(dest, pixels + offset, length);
}

const char *render_get_float_type_name() {
  return floattypenames[render_float_type];
}
