/**
 * @file mandelbrot.cpp
 *
 * Optimized mandelbrot set explorer.
 *
 * Copyright 2020 by Lars Christensen <larsch@belunktum.dk>
 */

#if HAVE_LIBGMP
#include "gmpfloat.hpp"
#endif
#if HAVE_LIBMPFR
#include "mpfrfloat.hpp"
#endif
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <quadmath.h>

#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>

#include "doubledouble.hpp"
#include "float.hpp"
#include "mandelbrot.hpp"
#include "semaphore.hpp"
#include "strop.hpp"
#include "typenames.hpp"

static TTF_Font *font;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static const float zoom_factor = 0.9;

#if 0
static std::mutex log_mutex;
#define LOG(x)                                    \
  do {                                            \
    std::unique_lock<std::mutex> lock(log_mutex); \
    std::cout << x << std::endl;                  \
  } while (0)
#else
#define LOG(x)
#endif

SDL_PixelFormatEnum pixel_format = SDL_PIXELFORMAT_ARGB8888;

static flt center_x{-0.60};
static flt center_y{0};
static flt screen_size{2.0};

// static flt center_x{-1.93947719527526271517e0};
// static flt center_y{-1.86470576599940926984e-3};
// static flt screen_size{2.0e-20};

enum FloatType {
  FT_AUTO,
  FT_FLOAT,
  FT_DOUBLE,
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
// FT_DOUBLELONG_DOUBLE,
#endif
#if HAVE_LIBGMP
  FT_GMPFLOAT,
#endif
#if HAVE_LIBMPFR
  FT_MPFRFLOAT128,
  FT_MPFRFLOAT256,
#endif
  FT_MAX
};

const char *floattypenames[] = {
    "auto",           "float",
    "double",         "doubledouble<double>",
#if HAVE_FLOAT80
    "__float80",      "doubledouble<__float80>",
#endif
#if HAVE_FLOAT128
    "__float128",     "doubledouble<__float128>",
#endif
#if HAVE_LONG_DOUBLE
    "long double",
// FT_DOUBLELONG_DOUBLE,
#endif
#if HAVE_LIBGMP
    "gmpfloat",
#endif
#if HAVE_LIBMPFR
    "mpfrfloat<128>", "mpfrfloat<256>",
#endif
};

static uint8_t *pixels = nullptr;
static int w;
static flt min_x;
static flt min_y;
static flt pixel_size;
static int pitch;
static semaphore jobs_left;
static std::atomic_int next_job = 0;
static semaphore jobs_done;
static int virtual_rows = 0;
static int rows = 0;
static int row_bits;
static bool rendering = false;
static std::atomic_bool running = true;
static int jobs_remaining = 0;
static std::chrono::time_point<std::chrono::high_resolution_clock> start;
static FloatType user_chosen_float_type = FT_AUTO; /** Type chosen by user */
static FloatType render_float_type = FT_AUTO;      /**< Type used for render */

#define RGB(r, g, b) (((r) << 16) | ((g) << 8) | ((b)))

#define ROWRENDER_COMPLETE_EVENT SDL_USEREVENT

/**
 * Convert HSV to RGB color.
 *
 * @param h Hue (0-359)
 * @param s Saturation (0.0-1.0)
 * @param v Value/lightness (0.0-1.0)
 */
uint32_t hsv2rgb(int h, float s, float v) {
  float hm = h / 60.0;
  float c = v * s;
  float x = c * (1.0 - abs(fmod(hm, 2.0) - 1.0));
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

/**
 * Palette
 */
class palette {
 public:
  uint32_t pal[256];
  palette() {
    for (int i = 0; i < 256; i++) {
      int h = fmod((i * 360.0) / 64, 360);
      float v = 0.6 + 0.3 * sin(i / 16.0 * M_PI);
      float s = 0.75 + 0.23 * cos(i / 8.0 * M_PI);
      pal[i] = hsv2rgb(h, s, v);
    }
  }
  uint32_t operator[](size_t i) { return pal[i]; }
};

palette pal;

int maprow(int row) {
  row = row % virtual_rows;
  int res = 0;
  for (int i = 0; i < row_bits; ++i) {
    res = (res << 1) | (row & 1);
    row >>= 1;
  }
  return res;
}

/**
 * Position as stored in bookmarks
 */
struct pos {
  flt x;
  flt y;
  flt s;
};

struct pos saves[10];

/**
 * Load bookmarks
 */
void load() {
  std::ifstream inf("bookmarks");
  if (inf) {
    if (inf.read(reinterpret_cast<char *>(&saves), sizeof(saves))) {
    }
  }
}

/**
 * Save bookmark
 */
void save(int slot) {
  load();
  saves[slot] = pos{center_x, center_y, screen_size};
  std::ofstream of("bookmarks");
  if (of) {
    if (of.write(reinterpret_cast<const char *>(&saves), sizeof(saves))) {
    }
  }
}

/**
 * Load bookmark
 */
void load(int slot) {
  load();
  center_x = saves[slot].x;
  center_y = saves[slot].y;
  screen_size = saves[slot].s;
}

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
#include <typeinfo>
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

  SDL_Event event;
  memset(&event, 0, sizeof(event));
  event.type = ROWRENDER_COMPLETE_EVENT;
  event.user.code = row;
  SDL_PushEvent(&event);
}

/**
 * Shorthand for epsilon
 */
template <typename FLT>
flt epsilon() {
  // return std::numeric_limits<FLT>::epsilon();
  return flt(std::numeric_limits<FLT>::epsilon());
};

/**
 * Determine the floating point type that will provide enough precision at the
 * curent pixel size.
 */
FloatType determine_type() {
  if (pixel_size > epsilon<float>())
    return FT_FLOAT;
  else if (pixel_size > epsilon<double>())
    return FT_DOUBLE;
  else if (pixel_size > epsilon<__float80>())
    return FT_FLOAT80;
  else if (pixel_size > epsilon<doubledouble<double>>())
    return FT_DOUBLEDOUBLE;
  else if (pixel_size > epsilon<__float128>())
    return FT_FLOAT128;
  else if (pixel_size > epsilon<long double>())
    return FT_LONG_DOUBLE;
  else if (pixel_size > epsilon<doubledouble<__float80>>())
    return FT_DOUBLEFLOAT80;
  else if (pixel_size > epsilon<doubledouble<long double>>())
    return FT_LONG_DOUBLE;
  else if (pixel_size > epsilon<doubledouble<__float128>>())
    return FT_DOUBLEFLOAT128;
  else
    return FT_GMPFLOAT;
}

/**
 * Render specified row using selected floating point type.
 */
#include <cfloat>
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
    case FT_GMPFLOAT:
      render_rowx<gmpfloat<128>>(row);
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
 * Start render
 */
void start_render() {
  pixel_size = screen_size / flt(rows);
  render_float_type = user_chosen_float_type == FT_AUTO
                          ? determine_type()
                          : user_chosen_float_type;
  min_x = center_x - w * pixel_size / flt(2.0);
  min_y = center_y - rows * pixel_size / flt(2.0);

  std::cout.precision(std::numeric_limits<decltype(min_x)>::digits10);
  std::cout << "type: " << tname<decltype(min_x)>() << std::endl;
  std::cout << "digits: " << std::numeric_limits<decltype(min_x)>::digits10
            << std::endl;
  std::cout << "min_x: " << min_x << std::endl;
  std::cout << "min_y: " << min_y << std::endl;

  row_bits = log2(rows) + 1;
  virtual_rows = 1 << row_bits;
  jobs_left.release(virtual_rows);
  jobs_remaining = virtual_rows;
  rendering = true;
  start = std::chrono::high_resolution_clock::now();
}

/**
 * Cancel current rendering progress. Auto to abort render when rending
 * parameters change.
 */
void cancel_render() {
  // Steal remaining jobs
  while (jobs_left.try_acquire()) jobs_done.release(1);

  // Wait for all workers to complete
  for (int i = 0; i < virtual_rows; ++i) jobs_done.acquire();

  rendering = false;
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

class mandelbrot_application {
 public:
  mandelbrot_application();
  ~mandelbrot_application();
  mandelbrot_application(const mandelbrot_application &) = delete;
  mandelbrot_application &operator=(const mandelbrot_application &) = delete;

  /** Main program loop */
  void run();
  /** Recreate rendering texture. Called after resize */
  void recreate_render_texture();
  /** Zoom in/out centered at specified screen coordinate */
  void zoom(int x, int y, float scale);

 private:
  SDL_Window *window = nullptr;
  SDL_Renderer *renderer = nullptr;
  SDL_Texture *texture = nullptr;
};

void mandelbrot_application::recreate_render_texture() {
  int width, height;
  SDL_GetWindowSize(window, &width, &height);

  if (texture) SDL_DestroyTexture(texture);
  texture = SDL_CreateTexture(renderer, pixel_format,
                              SDL_TEXTUREACCESS_STREAMING, width, height);
  delete[] pixels;
  pixels = new uint8_t[width * height * 4];
  pitch = width * 4;
  rows = height;
  w = width;
}

mandelbrot_application::mandelbrot_application() {
  SDL_Init(SDL_INIT_VIDEO);

  window = SDL_CreateWindow("Mandelbrot", SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED, 1024, 768,
                            SDL_WINDOW_RESIZABLE);
  if (window == nullptr) throw std::runtime_error("Failed to create window");

  pixel_format = (SDL_PixelFormatEnum)SDL_GetWindowPixelFormat(window);

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  recreate_render_texture();
}

void mandelbrot_application::run() {
  bool keep_running = true;

  std::array<int, 32> rows_completed;
  unsigned int rows_completed_count = 0;

  const int thread_count = 8;
  std::vector<std::thread> threads;
  for (int i = 0; i < thread_count; ++i) {
    threads.push_back(std::thread(worker));
  }

  bool update_surface = false;
  bool restart_render = true;

  bool wait = true;
  while (keep_running) {
    SDL_Event e;

    // Use polling until event queue is empty, then re-render if needed and
    // wait.
    if (wait) {
      if (restart_render) {
        if (rendering) cancel_render();
        start_render();
        restart_render = false;
      }

      if (update_surface) {
        /* TODO: Only copy rendered rows */
        int texture_pitch;
        void *pix = NULL;
        SDL_LockTexture(texture, NULL, &pix, &texture_pitch);
        int width, h;
        SDL_GetWindowSize(window, &width, &h);
        if (rows_completed_count == rows_completed.max_size()) {
          std::cout << "copy all rows" << std::endl;
          memcpy(pix, pixels, width * h * 4);
        } else {
          std::cout << "copy " << rows_completed_count << " rows" << std::endl;
          for (unsigned int i = 0; i < rows_completed_count; ++i) {
            int row = rows_completed[i];
            int pixels_pitch = width * 4;
            memcpy((uint8_t *)pix + texture_pitch * row,
                   pixels + pixels_pitch * row, width * 4);
          }
        }
        rows_completed_count = 0;
        SDL_UnlockTexture(texture);
        SDL_RenderCopy(renderer, texture, NULL, NULL);

        {
          auto text_surface =
              TTF_RenderUTF8_Blended(font, floattypenames[render_float_type],
                                     SDL_Color{255, 255, 255, 0});
          auto text_texture =
              SDL_CreateTextureFromSurface(renderer, text_surface);
          SDL_Rect dest_rect{0, 0, text_surface->w, text_surface->h};
          SDL_RenderCopy(renderer, text_texture, 0, &dest_rect);
          SDL_DestroyTexture(text_texture);
          SDL_FreeSurface(text_surface);
        }

        {
          std::ostringstream ostr;
          ostr << screen_size;
          auto text_surface = TTF_RenderUTF8_Blended(
              font, ostr.str().c_str(), SDL_Color{255, 255, 255, 0});
          auto text_texture =
              SDL_CreateTextureFromSurface(renderer, text_surface);
          SDL_Rect dest_rect{0, 30, text_surface->w, text_surface->h};
          SDL_RenderCopy(renderer, text_texture, 0, &dest_rect);
          SDL_DestroyTexture(text_texture);
          SDL_FreeSurface(text_surface);
        }

        SDL_RenderPresent(renderer);
        update_surface = false;
      }
      SDL_WaitEvent(&e);
      wait = false;
    } else {
      if (!SDL_PollEvent(&e)) {
        wait = true;
        continue;
      }
    }

    switch (e.type) {
      case ROWRENDER_COMPLETE_EVENT:
        if (--jobs_remaining == 0) {
          auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::high_resolution_clock::now() - start)
                              .count();
          std::cout << "render complete in " << duration << " ms" << std::endl;
        }
        {
          int row = e.user.code;
          if (row < rows) {
            if (rows_completed_count < rows_completed.max_size())
              rows_completed[rows_completed_count++] = e.user.code;
            update_surface = true;
          }
        }
        break;
      case SDL_QUIT:
        keep_running = false;
        break;
      case SDL_KEYDOWN:
        if (e.key.keysym.sym == SDLK_RIGHT) {
          center_x += screen_size * flt(0.1);
          std::cout << center_x << std::endl;
          restart_render = true;
        } else if (e.key.keysym.sym == SDLK_LEFT) {
          center_x -= screen_size * flt(0.1);
          restart_render = true;
        } else if (e.key.keysym.sym == SDLK_DOWN) {
          center_y += screen_size * flt(0.1);
          restart_render = true;
        } else if (e.key.keysym.sym == SDLK_UP) {
          center_y -= screen_size * flt(0.1);
          restart_render = true;
        } else if (e.key.keysym.sym == SDLK_RIGHTBRACKET) {
          if (rendering) cancel_render();
          int width, height;
          SDL_GetWindowSize(window, &width, &height);
          zoom(width / 2, height / 2, zoom_factor);
          restart_render = true;
        } else if (e.key.keysym.sym == SDLK_LEFTBRACKET) {
          if (rendering) cancel_render();
          int width, height;
          SDL_GetWindowSize(window, &width, &height);
          zoom(width / 2, height / 2, 1.0 / zoom_factor);
          restart_render = true;
        } else if (e.key.keysym.sym == SDLK_q) {
          keep_running = SDL_FALSE;
        } else if (e.key.keysym.mod == KMOD_LSHIFT ||
                   e.key.keysym.mod == KMOD_RSHIFT) {
          if (e.key.keysym.sym == SDLK_0) {
            user_chosen_float_type = FT_AUTO;
            restart_render = true;
          } else if (e.key.keysym.sym >= SDLK_1 && e.key.keysym.sym <= SDLK_9) {
            int num = e.key.keysym.sym - SDLK_1 + 1;
            if (num < FT_MAX) user_chosen_float_type = FloatType(num);
            restart_render = true;
          }
        } else if (e.key.keysym.mod == KMOD_LCTRL ||
                   e.key.keysym.mod == KMOD_RCTRL) {
          if (e.key.keysym.sym >= SDLK_0 && e.key.keysym.sym <= SDLK_9) {
            int num = e.key.keysym.sym - SDLK_0;
            save(num);
          }
        } else if (e.key.keysym.mod == KMOD_NONE) {
          if (e.key.keysym.sym >= SDLK_0 && e.key.keysym.sym <= SDLK_9) {
            int num = e.key.keysym.sym - SDLK_0;
            std::cout << "loading slot" << num << std::endl;
            load(num);
            restart_render = true;
          }
        }
        break;
      case SDL_MOUSEMOTION:
        break;
      case SDL_MOUSEWHEEL: {
        if (rendering) cancel_render();
        restart_render = true;
        int x, y;
        SDL_GetMouseState(&x, &y);
        zoom(x, y, pow(zoom_factor, int(e.wheel.y)));
        zoom(x, y, pow(zoom_factor, int(e.wheel.y)));
      } break;
      case SDL_WINDOWEVENT:
        if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
          if (rendering) cancel_render();
          recreate_render_texture();
          restart_render = true;
          break;
        }
    }
  }

  running = false;
  if (rendering) cancel_render();
  jobs_left.release(thread_count);
  for (auto &thr : threads) thr.join();
}

void mandelbrot_application::zoom(int x, int y, float scale) {
  int width, height;
  SDL_GetWindowSize(window, &width, &height);
  int ofsx = x - width / 2;
  int ofsy = y - height / 2;
  flt old_pixel_size = screen_size / flt(height);
  screen_size *= flt(scale);
  flt new_pixel_size = screen_size / flt(height);

  flt left = center_x - width * old_pixel_size * flt(0.5);
  flt top = center_y - height * old_pixel_size * flt(0.5);

  center_x = center_x + ofsx * old_pixel_size - ofsx * new_pixel_size;
  center_y = center_y + ofsy * old_pixel_size - ofsy * new_pixel_size;

  flt x1 = width * flt(0.5) - (center_x - left) / new_pixel_size;
  flt y1 = height * flt(0.5) - (center_y - top) / new_pixel_size;

  // Render scaled version of on-screen texture to a temporary texture and copy
  // pixels.
  SDL_Texture *temp_texture = SDL_CreateTexture(
      renderer, pixel_format, SDL_TEXTUREACCESS_TARGET, width, height);
  SDL_SetRenderTarget(renderer, temp_texture);
  SDL_FRect dst{static_cast<float>(x1), static_cast<float>(y1),
                static_cast<float>(width / scale),
                static_cast<float>(height / scale)};
  SDL_RenderCopyF(renderer, texture, 0, &dst);
  SDL_RenderReadPixels(renderer, 0, pixel_format, pixels, pitch);
  SDL_SetRenderTarget(renderer, NULL);
  SDL_DestroyTexture(temp_texture);

  // Update the on-screen texture
  int texture_pitch;
  void *pix = NULL;
  SDL_LockTexture(texture, NULL, &pix, &texture_pitch);
  memcpy(pix, pixels, width * height * 4);
  SDL_UnlockTexture(texture);
}

mandelbrot_application::~mandelbrot_application() {
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  delete[] pixels;
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  TTF_Init();
  font = TTF_OpenFont("/usr/share/fonts/TTF/DejaVuSans-Bold.ttf", 16);
  TTF_SetFontHinting(font, TTF_HINTING_NORMAL);

  try {
    mandelbrot_application app;
    app.run();

    for (int i = 0; i < 10; ++i) {
      // mp_exp_t exp;
      // mpf_init2(saves[i].x.mpf, 256);
      // mpf_set_d(saves[i].x.mpf, 3.14159265);
      // char *str = mpf_get_str(0, &exp, 10, 0, saves[i].x.mpf);
      // std::cout << i << "," << str << "," << exp << std::endl;
      // int exp;
      // std::frexp(saves[i].x, &exp);
      // printf("%d %.36f\n", i, (double)saves[i].x);
      // std::cout << float2string(saves[i].x) << " " <<
      // float2string(saves[i].y) << " " << float2string(saves[i].s) <<
      // std::endl;

      // char str[256];
      // __float128 f = 3.141592653;
      // auto r = quadmath_snprintf(str, 256, "%.20Qe", f);
      // std::cout << "result=" << r << std::endl;
      // std::cout << str << std::endl;
    }
    return EXIT_SUCCESS;
  } catch (const std::exception &exc) {
    std::cerr << exc.what() << std::endl;
    return EXIT_FAILURE;
  }
}
