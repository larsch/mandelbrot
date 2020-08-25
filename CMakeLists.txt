cmake_minimum_required(VERSION 3.18)
include(CheckTypeSize)

project(mandlebrot)

find_package(Threads REQUIRED)
check_type_size(__float128 FLT128)
check_type_size(__float80 FLT80)
add_executable(mandelbrot mandelbrot.cpp)
target_compile_definitions(mandelbrot PUBLIC -DHAVE_FLT128=$<BOOL:HAVE_FLT128>)
target_compile_definitions(mandelbrot PUBLIC -DHAVE_FLT80=$<BOOL:HAVE_FLT80>)

target_compile_options(mandelbrot PUBLIC -O3)
target_compile_options(mandelbrot PUBLIC -ffast-math)

target_compile_options(mandelbrot PUBLIC -std=c++17)
target_compile_options(mandelbrot PUBLIC -flto)
target_link_options(mandelbrot PUBLIC -flto)
target_link_libraries(mandelbrot PUBLIC SDL2 Threads::Threads)
