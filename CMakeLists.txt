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

add_executable(mandelbrot mandelbrot.cpp)

# target_compile_definitions(mandelbrot PUBLIC -DHAVE_LONG_DOUBLE=$<BOOL:${HAVE_LONG_DOUBLE}>)
# target_compile_definitions(mandelbrot PUBLIC -DHAVE_FLOAT80=$<BOOL:${HAVE_FLOAT80}>)
# target_compile_definitions(mandelbrot PUBLIC -DHAVE_FLOAT128=$<BOOL:${HAVE_FLOAT128}>)
# target_compile_definitions(mandelbrot PUBLIC -DHAVE_LIBGMP=$<BOOL:${LIBGMP}>)

# target_compile_options(mandelbrot PUBLIC -std=c++17)
# target_compile_options(mandelbrot PUBLIC -Wall -Wextra)
# target_compile_options(mandelbrot PUBLIC -ffast-math)
target_compile_options(mandelbrot PUBLIC "$<$<CONFIG:RELEASE>:-Werror>")
target_compile_options(mandelbrot PUBLIC "$<$<CONFIG:RELEASE>:-O3>")
target_compile_options(mandelbrot PUBLIC "$<$<CONFIG:RELEASE>:-flto>")
target_link_options(mandelbrot PUBLIC "$<$<CONFIG:RELEASE>:-flto>")

target_link_libraries(mandelbrot PUBLIC "${SDL2_LIBS}" Threads::Threads)
target_link_libraries(mandelbrot PUBLIC quadmath)

add_executable(benchmark benchmark.cpp)
target_compile_options(benchmark PUBLIC "$<$<CONFIG:RELEASE>:-flto>")
target_link_options(benchmark PUBLIC "$<$<CONFIG:RELEASE>:-flto>")

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
