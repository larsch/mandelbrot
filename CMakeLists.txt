cmake_minimum_required(VERSION 3.18)
include(CheckTypeSize)

project(mandlebrot)

find_package(Threads REQUIRED)
check_type_size(float FLOAT)
check_type_size(double DOUBLE)
check_type_size("long double" LONG_DOUBLE)
check_type_size(__float80 FLOAT80)
check_type_size(__float128 FLOAT128)
message("sizeof(float) = ${FLOAT}")
message("sizeof(double) = ${DOUBLE}")
message("sizeof(long double) = ${LONG_DOUBLE}")
message("sizeof(__float80) = ${FLOAT80}")
message("sizeof(__float128) = ${FLOAT128}")

add_executable(mandelbrot mandelbrot.cpp)

target_compile_definitions(mandelbrot PUBLIC -DHAVE_LONG_DOUBLE=$<BOOL:${HAVE_LONG_DOUBLE}>)
target_compile_definitions(mandelbrot PUBLIC -DHAVE_FLOAT80=$<BOOL:${HAVE_FLOAT80}>)
target_compile_definitions(mandelbrot PUBLIC -DHAVE_FLOAT128=$<BOOL:${HAVE_FLOAT128}>)

target_compile_options(mandelbrot PUBLIC -O3)
target_compile_options(mandelbrot PUBLIC -Wall -Wextra -Werror)
target_compile_options(mandelbrot PUBLIC -Werror)
target_compile_options(mandelbrot PUBLIC -ffast-math)

target_compile_options(mandelbrot PUBLIC -std=c++17)
target_compile_options(mandelbrot PUBLIC -flto)
target_link_options(mandelbrot PUBLIC -flto)
target_link_libraries(mandelbrot PUBLIC SDL2 Threads::Threads)
