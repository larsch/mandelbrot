cmake_minimum_required(VERSION 3.18)
project(mandlebrot CXX)

set(CMAKE_MODULE_PATH "${CMAKE_SYSROOT}/lib/cmake")
if(${CMAKE_CROSSCOMPILING})
    set(SDL2_LIBS mingw32 SDL2main SDL2.dll)
    include(SDL2/sdl2-config)
else()
    include(SDL2/SDL2Targets)
    set(SDL2_LIBS SDL2::SDL2 SDL2_ttf)
endif()

find_library(LIBGMP NAMES gmp)
find_library(LIBGMPXX NAMES gmpxx)
find_library(LIBMPFR NAMES mpfr)

find_package(Threads REQUIRED)

include(CheckTypeSize)
check_type_size("long double" LONG_DOUBLE LANGUAGE CXX)
check_type_size(__float80 FLOAT80 LANGUAGE CXX)
check_type_size(__float128 FLOAT128 LANGUAGE CXX)

message("sizeof(long double) = ${LONG_DOUBLE}")
message("sizeof(__float80) = ${FLOAT80}")
message("sizeof(__float128) = ${FLOAT128}")

add_compile_definitions(HAVE_LONG_DOUBLE=$<BOOL:${HAVE_LONG_DOUBLE}>
HAVE_FLOAT80=$<BOOL:${HAVE_FLOAT80}>
HAVE_FLOAT128=$<BOOL:${HAVE_FLOAT128}>
HAVE_LIBGMP=$<BOOL:${LIBGMP}>
MPFR_WANT_FLOAT128=1)
add_compile_options(-std=c++17 -Wall -Wextra -Warith-conversion)
# add_compile_options(-ffast-math)

option(PROFILE "Enable profiling" FALSE)
option(FLTO "Enable FLTO" TRUE)

if(PROFILE)
add_compile_options(-pg -ggdb)
add_link_options(-pg -ggdb)
endif()

if(FLTO)
add_compile_options(-flto)
add_link_options(-flto)
endif()

add_executable(mandelbrot mandelbrot.cpp)

target_compile_options(mandelbrot PUBLIC "$<$<CONFIG:RELEASE>:-Werror>")
target_compile_options(mandelbrot PUBLIC "$<$<CONFIG:RELEASE>:-O3>")

target_link_libraries(mandelbrot PUBLIC "${SDL2_LIBS}" Threads::Threads)
target_link_libraries(mandelbrot PUBLIC quadmath)

add_executable(benchmark benchmark.cpp)

add_executable(unittest unittest.cpp)

if(LIBGMP)
target_link_libraries(mandelbrot PUBLIC ${LIBGMP} ${LIBGMPXX})
target_link_libraries(benchmark PUBLIC ${LIBGMP} ${LIBGMPXX})
target_link_libraries(unittest PUBLIC ${LIBGMP} ${LIBGMPXX})
endif()
if(LIBMPFR)
target_link_libraries(mandelbrot PUBLIC "${LIBMPFR}")
target_link_libraries(benchmark PUBLIC "${LIBMPFR}")
target_link_libraries(unittest PUBLIC "${LIBMPFR}")
endif()
