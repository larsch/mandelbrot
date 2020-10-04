#ifndef _application_hpp
#define _application_hpp

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define ROWRENDER_COMPLETE_EVENT SDL_USEREVENT

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
  /** Render text on screen */
  void render_text(int x, int y, const char *str);
  /** Render help text */
  void render_help();

 private:
  SDL_Window *window = nullptr;
  SDL_Renderer *renderer = nullptr;
  SDL_Texture *texture = nullptr;
  TTF_Font *font;
};

#endif // _application_hpp
