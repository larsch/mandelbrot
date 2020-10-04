/**
 * @file application.cpp
 *
 * Optimized mandelbrot set explorer.
 *
 * Copyright 2020 by Lars Christensen <larsch@belunktum.dk>
 */

#include "application.hpp"
#include "render.hpp"
#include "strop.hpp"

#include <sstream>
#include <chrono>

/// On screen pixel format
static SDL_PixelFormatEnum pixel_format = SDL_PIXELFORMAT_ARGB8888;

/// Zoom in/out factor
static const float zoom_factor = 0.9;

/// Rendering starting time
static std::chrono::time_point<std::chrono::high_resolution_clock> start;

void mandelbrot_application::recreate_render_texture() {
  int width, height;
  SDL_GetWindowSize(window, &width, &height);

  if (texture) SDL_DestroyTexture(texture);
  texture = SDL_CreateTexture(renderer, pixel_format,
                              SDL_TEXTUREACCESS_STREAMING, width, height);
  render_reconfigure(width, height);
}

mandelbrot_application::mandelbrot_application() 
  : font(nullptr) {
  SDL_Init(SDL_INIT_VIDEO);

  TTF_Init();
  font = TTF_OpenFont("/usr/share/fonts/TTF/DejaVuSans-Bold.ttf", 16);
  TTF_SetFontHinting(font, TTF_HINTING_NORMAL);

  window = SDL_CreateWindow("Mandelbrot", SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED, 1024, 768,
                            SDL_WINDOW_RESIZABLE);
  if (window == nullptr) throw std::runtime_error("Failed to create window");

  pixel_format = (SDL_PixelFormatEnum)SDL_GetWindowPixelFormat(window);

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  recreate_render_texture();
}

void mandelbrot_application::render_text(int x, int y, const char *str) {
  auto text_surface =
      TTF_RenderUTF8_Blended(font, str, SDL_Color{255, 255, 255, 0});
  auto text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
  SDL_Rect dest_rect{x, y, text_surface->w, text_surface->h};
  SDL_RenderCopy(renderer, text_texture, 0, &dest_rect);
  SDL_DestroyTexture(text_texture);
  SDL_FreeSurface(text_surface);
}

#define RENDER_TEXT(x, y, strseq)          \
  {                                        \
    std::ostringstream ostr;               \
    ostr << strseq;                        \
    render_text(x, y, ostr.str().c_str()); \
  }

static const char *help[] = {
    "h, ?, F1: toggle help display",
    "i: toggle information display",
    "shift+<N>: use fixed precision",
    "shift+0: use dynamic precision (default)",
};

void mandelbrot_application::render_help() {
  int lines = sizeof(help) / sizeof(help[0]);
  int width, height;
  SDL_GetWindowSize(window, &width, &height);
  for (int i = 0; i < lines; ++i) {
    int y = height - 20 - 20 * (lines - i);
    render_text(10, y, help[i]);
  }
}

void mandelbrot_application::run() {
  bool keep_running = true;
  bool show_help = false;
  bool show_information = false;

  std::array<int, 32> rows_completed;
  unsigned int rows_completed_count = 0;

  render_init();

  bool update_surface = false;
  bool restart_render = true;

  bool wait = true;
  while (keep_running) {
    SDL_Event e;

    // Use polling until event queue is empty, then re-render if needed and
    // wait.
    if (wait) {
      if (restart_render) {
        cancel_render();
        start_render();
        start = std::chrono::high_resolution_clock::now();
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
          render_copy_pixels(pix, 0, width * h * 4);
        } else {
          for (unsigned int i = 0; i < rows_completed_count; ++i) {
            int row = rows_completed[i];
            int pixels_pitch = width * 4;
            render_copy_pixels((uint8_t *)pix + texture_pitch * row,
                   pixels_pitch * row, width * 4);
          }
        }
        rows_completed_count = 0;
        SDL_UnlockTexture(texture);
        SDL_RenderCopy(renderer, texture, NULL, NULL);

        if (show_information) {
          render_text(10, 10, render_get_float_type_name());
          RENDER_TEXT(10, 30, pixel_size);
          RENDER_TEXT(10, 50, jobs_remaining);
        }

        if (show_help) render_help();

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
      case SDL_TEXTINPUT:
        if (strcmp(e.text.text, "?") == 0) {
          show_help = !show_help;
          update_surface = true;
        }
        break;
      case SDL_KEYDOWN:
        if (e.key.keysym.sym == SDLK_h || e.key.keysym.sym == SDLK_QUESTION ||
            e.key.keysym.sym == SDLK_F1) {
          show_help = !show_help;
          update_surface = true;
        } else if (e.key.keysym.sym == SDLK_i) {
          show_information = !show_information;
          update_surface = true;
        } else if (e.key.keysym.sym == SDLK_RIGHT) {
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
          cancel_render();
          int width, height;
          SDL_GetWindowSize(window, &width, &height);
          zoom(width / 2, height / 2, zoom_factor);
          restart_render = true;
        } else if (e.key.keysym.sym == SDLK_LEFTBRACKET) {
          cancel_render();
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
            // int num = e.key.keysym.sym - SDLK_0;
            // save(num);
          }
        } else if (e.key.keysym.mod == KMOD_NONE) {
          if (e.key.keysym.sym >= SDLK_0 && e.key.keysym.sym <= SDLK_9) {
            // int num = e.key.keysym.sym - SDLK_0;
            // std::cout << "loading slot" << num << std::endl;
            // // load(num);
            // restart_render = true;
          }
        }
        break;
      case SDL_MOUSEMOTION:
        break;
      case SDL_MOUSEWHEEL: {
        cancel_render();
        restart_render = true;
        int x, y;
        SDL_GetMouseState(&x, &y);
        zoom(x, y, pow(zoom_factor, int(e.wheel.y)));
      } break;
      case SDL_WINDOWEVENT:
        if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
          cancel_render();
          recreate_render_texture();
          restart_render = true;
          break;
        }
    }
  }

  running = false;
  cancel_render();
  render_stop();
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
  if (scale > 1.0) SDL_RenderCopy(renderer, texture, 0, 0);
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
  TTF_CloseFont(font);
  TTF_Quit();
  SDL_Quit();
}
