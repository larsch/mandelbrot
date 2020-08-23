cmake_minimum_required(VERSION 3.18)
project(mandlebrot)
find_package(Threads REQUIRED)
add_executable(mandelbrot mandelbrot.cpp)

target_compile_options(mandelbrot PUBLIC -O3)
target_compile_options(mandelbrot PUBLIC -ffast-math)

target_compile_options(mandelbrot PUBLIC -std=c++20)
target_compile_options(mandelbrot PUBLIC -flto)
target_link_options(mandelbrot PUBLIC -flto)
target_link_libraries(mandelbrot PUBLIC SDL2 Threads::Threads)