/**
 * @file mandelbrot.cpp
 *
 * Optimized mandelbrot set explorer.
 *
 * Copyright 2020 by Lars Christensen <larsch@belunktum.dk>
 */

#include <SDL2/SDL.h>

#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>

#include "semaphore.hpp"

static const float zoom_factor = 0.75;

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

#if HAVE_FLOAT128
typedef __float128 flt;
#elif HAVE_FLOAT80
typedef __float80 flt;
#else
typedef double flt;
#endif

static flt center_x = -0.60;
static flt center_y = 0;
static flt screen_size = 2.0;

const int LIMIT = 2048;

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
static int algorithm = 0;

#define RGB(r, g, b) (((r) << 16) | ((g) << 8) | ((b)))

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

/**
 * Check if x,y is inside some of the obvious areas of the mandelbrot set. This
 * improves performance greatly when rendering initial screen where the main set
 * is visible.
 */
template <typename FLT>
bool isinside(FLT x, FLT y) {
  FLT absy = abs(double(y));
  if (x > -0.75 && absy < 0.75) {
    // check if x,y is inside the main cardioid
    x -= 0.25;
    FLT c1 = (x * x + y * y);
    FLT a = 0.25;
    FLT c2 = c1 * c1 + 4 * a * x * c1 - 4 * a * a * y * y;
    return c2 < 0;
  } else if (x > -1.25 && absy < 0.25) {
    // check if x,y is inside the left circle
    x += 1;
    return x * x + y * y < 0.0625;
  } else {
    return false;
  }
}

template <typename FLT>
struct iter_result {
  unsigned int iterations;
  FLT x;
  FLT y;
};

/**
 * Perform mandelbrot iterations and return the number of iteration required
 * before escape.
 */
template <typename FLT>
iter_result<FLT> iter(FLT xc, FLT yc) {
  FLT x = xc;
  FLT y = yc;
  if (isinside(xc, yc)) return {LIMIT, 0, 0};

  unsigned int iterations = 0;
  FLT x2 = x * x;
  FLT y2 = y * y;

  while (x2 + y2 < 4.0 && ++iterations < LIMIT) {
    y = x * y * 2 + yc;
    x = x2 - y2 + xc;
    x2 = x * x;
    y2 = y * y;
  }

  for (int j = 0; j < 4; ++j) {
    y = x * y * 2 + yc;
    x = x2 - y2 + xc;
    x2 = x * x;
    y2 = y * y;
  }

  return {iterations, x2, y2};
}

template <typename FLT>
FLT fraction(FLT zx2, FLT zy2) {
  const FLT log2Inverse = 1.0 / log(2.0);
  const FLT logHalflog2Inverse = log(0.5) * log2Inverse;
  return 5.0 - logHalflog2Inverse -
         log(log(double(zx2) + double(zy2))) * log2Inverse;
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
  const FLT scl = pixel_size;
  const FLT minx = min_x;
  const FLT miny = min_y;
  row = maprow(row);
  if (row < rows) {
    FLT yc = miny + row * scl;
    uint32_t *row_pixels = reinterpret_cast<uint32_t *>(pixels + row * pitch);
    LOG("row: " << row << " " << rows << " " << (void *)row_pixels << " " << w);
    for (int col = 0; col < w; ++col) {
      auto result = iter(minx + col * scl, yc);
      if (result.iterations == LIMIT) {
        *row_pixels++ = 0x00;
      } else {
        FLT zx2 = result.x;
        FLT zy2 = result.y;
        FLT sum = result.iterations + fraction(zx2, zy2);
        unsigned int n = (unsigned int)floor(double(sum));
        FLT f2 = sum - n;
        unsigned int n1 = n % 256;
        FLT f1 = 1.0 - f2;
        unsigned int n2 = ((n1 + 1) % 256);
        *row_pixels++ = blend(pal[n1], pal[n2], f1);
      }
    }
  }

  SDL_Event event;
  event.type = SDL_USEREVENT;
  SDL_PushEvent(&event);
}

#if HAVE_FLOAT128
std::ostream &operator<<(std::ostream &os, __float128 f) {
  return os << double(f);
}
#endif

template <typename T>
bool try_render(int row) {
  if (pixel_size > std::numeric_limits<T>::epsilon()) {
    render_rowx<T>(row);
    return true;
  } else {
    return false;
  }
}

/**
 * Render specified row using selected algorithm.
 */
#include <cfloat>
void render_row(int row) {
  switch (algorithm) {
    case 0:
      try_render<float>(row) || try_render<double>(row) ||
#if HAVE_LONG_DOUBLE
          try_render<long double>(row) ||
#endif  // HAVE_LONG_DOUBLE
#if HAVE_FLOAT80
          try_render<__float80>(row) ||
#endif  // HAVE_FLOATT80
#if HAVE_FLOAT128
          try_render<__float128>(row) ||
#endif  // HAVE_FLOATT80
          (render_rowx<flt>(row), true);
      break;
    case 1:
      render_rowx<float>(row);
      break;
    case 2:
      render_rowx<double>(row);
      break;
#if HAVE_FLOAT80
    case 3:
      render_rowx<__float80>(row);
      break;
#endif
#if HAVE_FLOAT128
    case 4:
      render_rowx<__float128>(row);
      break;
#endif
  }
}

/**
 * Start render
 */
void start_render() {
  pixel_size = screen_size / rows;
  min_x = center_x - w * pixel_size / 2;
  min_y = center_y - rows * pixel_size / 2;
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
  while (jobs_left.try_acquire()) {
    jobs_done.release(1);
  }
  for (int i = 0; i < virtual_rows; ++i) {
    jobs_done.acquire();
  }
  rendering = false;
}

/**
 * Rendering worker main function.
 */
void worker() {
  while (running) {
    jobs_left.acquire();
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
  void run();
  void recreate_render_texture();
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
  // SDL_Surface *surface = SDL_GetWindowSurface(window);
  // SDL_Surface *surface = NULL;
  // SDL_ShowWindow(window);
  int keep_running = SDL_TRUE;

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
        LOG("update_surface");
        int texture_pitch;
        void *pix = NULL;
        SDL_LockTexture(texture, NULL, &pix, &texture_pitch);
        int width, h;
        SDL_GetWindowSize(window, &width, &h);
        memcpy(pix, pixels, width * h * 4);
        SDL_UnlockTexture(texture);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
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
      case SDL_USEREVENT:
        if (--jobs_remaining == 0) {
          auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::high_resolution_clock::now() - start)
                              .count();
          std::cout << "render complete in " << duration << " ms" << std::endl;
        }
        update_surface = true;
        break;
      case SDL_QUIT:
        keep_running = SDL_FALSE;
        break;
      case SDL_KEYDOWN:
        if (e.key.keysym.sym == SDLK_RIGHT) {
          center_x += screen_size * 0.1;
          restart_render = true;
        } else if (e.key.keysym.sym == SDLK_LEFT) {
          center_x -= screen_size * 0.1;
          restart_render = true;
        } else if (e.key.keysym.sym == SDLK_DOWN) {
          center_y += screen_size * 0.1;
          restart_render = true;
        } else if (e.key.keysym.sym == SDLK_UP) {
          center_y -= screen_size * 0.1;
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
          if (e.key.keysym.sym == SDLK_1) {
            algorithm = 1;
            restart_render = true;
          } else if (e.key.keysym.sym == SDLK_2) {
            algorithm = 2;
            restart_render = true;
          } else if (e.key.keysym.sym == SDLK_3) {
            algorithm = 3;
            restart_render = true;
          } else if (e.key.keysym.sym == SDLK_4) {
            algorithm = 4;
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

  if (rendering) cancel_render();
  jobs_left.release(thread_count);
  running = false;
  for (auto &thr : threads) thr.join();
}

void mandelbrot_application::zoom(int x, int y, float scale) {
  int width, height;
  SDL_GetWindowSize(window, &width, &height);
  int ofsx = x - width / 2;
  int ofsy = y - height / 2;
  flt old_pixel_size = screen_size / height;
  screen_size *= scale;
  flt new_pixel_size = screen_size / height;

  flt left = center_x - width / 2 * old_pixel_size;
  flt top = center_y - height / 2 * old_pixel_size;

  center_x = center_x + ofsx * old_pixel_size - ofsx * new_pixel_size;
  center_y = center_y + ofsy * old_pixel_size - ofsy * new_pixel_size;

  flt x1 = width / 2 - (center_x - left) / new_pixel_size;
  flt y1 = height / 2 - (center_y - top) / new_pixel_size;

  SDL_Texture *back = SDL_CreateTexture(
      renderer, pixel_format, SDL_TEXTUREACCESS_TARGET, width, height);
  SDL_SetRenderTarget(renderer, back);
  SDL_FRect dst{static_cast<float>(x1), static_cast<float>(y1),
                static_cast<float>(width / scale),
                static_cast<float>(height / scale)};
  SDL_RenderCopy(renderer, texture, 0, 0);
  SDL_RenderCopyF(renderer, texture, 0, &dst);
  SDL_RenderReadPixels(renderer, 0, pixel_format, pixels, pitch);
  SDL_SetRenderTarget(renderer, NULL);
  SDL_DestroyTexture(back);
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
  try {
    mandelbrot_application app;
    app.run();
    return EXIT_SUCCESS;
  } catch (const std::exception &exc) {
    std::cerr << exc.what() << std::endl;
    return EXIT_FAILURE;
  }
}
