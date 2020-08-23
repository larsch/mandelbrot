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

typedef __float128 flt;

flt center_x = -0.60;
flt center_y = 0;
flt screen_size = 2.0;

const int LIMIT = 2048;

uint8_t *pixels;
int w;
flt min_x;
flt min_y;
flt pixel_size;
int pitch;
semaphore jobs_left;
std::atomic_int next_job = 0;
semaphore jobs_done;
SDL_Window *window = nullptr;
int virtual_rows = 0;
int rows = 0;
int row_bits;
bool rendering = false;
std::atomic_bool running = true;
int jobs_remaining = 0;
std::chrono::time_point<std::chrono::high_resolution_clock> start;

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
  std::cout << x << std::endl;
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
      std::cout << i << ": " << h << " " << std::hex << pal[i] << std::dec
                << std::endl;
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
  __float128 x;
  __float128 y;
  __float128 s;
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
      std::cout << "wrote" << std::endl;
    }
  } else {
    std::cout << "failed to create" << std::endl;
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

/**
 * Perform mandelbrot iterations and return the number of iteration required
 * before escape.
 */
template <typename FLT>
int iter(FLT xc, FLT yc) {
  FLT x = xc;
  FLT y = yc;
  if (isinside(xc, yc)) return LIMIT;

  int i;
  for (i = 0; i < LIMIT; ++i) {
    FLT x2 = x * x;
    FLT y2 = y * y;
    if (x2 + y2 > 4.0) {
      break;
    }
    FLT nextx = x2 - y2 + xc;
    y = x * y * 2 + yc;
    x = nextx;
  }
  return i;
}

/**
 * Render a single row of the mandelbrot set
 */
template <typename FLT>
void render_rowx(int row) {
  const FLT scl = pixel_size;
  const FLT minx = min_x;
  const FLT miny = min_y;
  row = maprow(row);
  if (row < rows) {
    FLT yc = miny + row * scl;
    uint32_t *row_pixels = reinterpret_cast<uint32_t *>(pixels + row * pitch);
    for (int col = 0; col < w; ++col) {
      int i = iter(minx + col * scl, yc);
      *row_pixels++ = (i == LIMIT) ? 0x00 : pal[i % 256];
    }
  }

  SDL_Event event;
  event.type = SDL_USEREVENT;
  SDL_PushEvent(&event);
}

int algorithm = 1;
/**
 * Render specified row using selected algorithm.
 */
void render_row(int row) {
  switch (algorithm) {
    case 1:
      render_rowx<float>(row);
      break;
    case 2:
      render_rowx<double>(row);
      break;
    case 3:
      render_rowx<__float80>(row);
      break;
    case 4:
      render_rowx<__float128>(row);
      break;
  }
}

/**
 * Start render
 */
void start_render(SDL_Surface *surface) {
  pixels = reinterpret_cast<uint8_t *>(surface->pixels);
  pixel_size = screen_size / surface->h;
  min_x = center_x - surface->w * pixel_size / 2;
  min_y = center_y - surface->h * pixel_size / 2;
  pitch = surface->pitch;
  w = surface->w;
  rows = surface->h;
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
  printf("cancel...");
  fflush(stdout);
  while (jobs_left.try_acquire()) {
    jobs_done.release(1);
  }
  for (int i = 0; i < virtual_rows; ++i) {
    jobs_done.acquire();
  }
  rendering = false;
  printf("done\n");
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

int main(int argc, char **argv) {
  window = SDL_CreateWindow("Mandelbrot", SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED, 1024, 768,
                            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  SDL_Surface *surface = SDL_GetWindowSurface(window);
  SDL_ShowWindow(window);
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
        start_render(surface);
        SDL_UpdateWindowSurface(window);
        restart_render = false;
      }

      if (update_surface) {
        SDL_UpdateWindowSurface(window);
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
          std::cout << "render complete in " << duration << std::endl;
        }
        update_surface = true;
        break;
      case SDL_QUIT:
        keep_running = SDL_FALSE;
        break;
      case SDL_KEYDOWN:
        std::cout << "key " << e.key.keysym.sym << " " << e.key.keysym.mod
                  << std::endl;
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
          screen_size *= 0.9;
          restart_render = true;
        } else if (e.key.keysym.sym == SDLK_LEFTBRACKET) {
          screen_size /= 0.9;
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
          std::cout << "ctrl" << std::endl;
          if (e.key.keysym.sym >= SDLK_0 && e.key.keysym.sym <= SDLK_9) {
            int num = e.key.keysym.sym - SDLK_0;
            std::cout << "saving slot" << num << std::endl;
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
      case SDL_WINDOWEVENT:
        if (rendering) cancel_render();
        if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
          surface = SDL_GetWindowSurface(window);
          restart_render = true;
          break;
        }
    }
  }

  if (rendering) cancel_render();
  jobs_left.release(thread_count);
  running = false;
  for (auto &thr : threads) thr.join();
  return 0;
}