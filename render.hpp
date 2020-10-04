#ifndef _render_hpp
#define _render_hpp

#include "floattype.hpp"
#include "float.hpp"

#include <atomic>

extern flt center_x;
extern flt center_y;
extern flt screen_size;
extern uint8_t *pixels;
// extern int w;
// extern flt min_x;
// extern flt min_y;
extern flt pixel_size;
extern int pitch;
// extern semaphore jobs_left;
// extern std::atomic_int next_job = 0;
// extern semaphore jobs_done;
// extern int virtual_rows = 0;
extern int rows;
// extern int row_bits;
// extern bool rendering = false;
extern std::atomic_bool running;
extern int jobs_remaining;
// extern std::chrono::time_point<std::chrono::high_resolution_clock> start;
extern FloatType user_chosen_float_type; /** Type chosen by user */
// extern FloatType render_float_type = FT_AUTO;      /**< Type used for render */

/// Reconfigure the rendering for a new screen size
void render_reconfigure(int width, int height);

/// Initialize rendering engine
void render_init();

/// Stop and clean up rendering engine
void render_stop();

/// Cancel current rendering if any
void cancel_render();

/// Start rendering
void start_render();

/// Copy pixels from rendering buffer
void render_copy_pixels(void* dest, size_t offset, size_t length);

const char* render_get_float_type_name();

#endif  // _render_hpp
