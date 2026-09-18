# File: CMakeLists.txt
# Purpose: Configure the portable engine core, tests, benchmark, sample, and optional SDL3 frontend.
cmake_minimum_required(VERSION 3.25)

project(forge2d_engine VERSION 1.0.0 LANGUAGES C CXX)

include(CTest)
include(GNUInstallDirs)

option(FORGE2D_BUILD_SDL "Build the SDL3 interactive sample" OFF)
option(FORGE2D_BUILD_TESTS "Build deterministic tests" ON)
option(FORGE2D_BUILD_BENCHMARKS "Build performance regression checks" ON)
option(FORGE2D_BUILD_FUZZ "Build the libFuzzer scene-parser target" OFF)
option(FORGE2D_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(FORGE2D_ENABLE_COVERAGE "Enable GCC/Clang coverage instrumentation" OFF)
option(FORGE2D_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

function(forge2d_set_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /permissive- /sdl)
    if(FORGE2D_WARNINGS_AS_ERRORS)
      target_compile_options(${target} PRIVATE /WX)
    endif()
  else()
    target_compile_options(${target} PRIVATE
      -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow
      $<$<COMPILE_LANGUAGE:CXX>:-Wnon-virtual-dtor>
      $<$<COMPILE_LANGUAGE:CXX>:-Wold-style-cast>
      $<$<COMPILE_LANGUAGE:CXX>:-Woverloaded-virtual>)
    if(FORGE2D_WARNINGS_AS_ERRORS)
      target_compile_options(${target} PRIVATE -Werror)
    endif()
  endif()
endfunction()

function(forge2d_enable_sanitizers target)
  if(FORGE2D_ENABLE_SANITIZERS AND NOT MSVC)
    target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(${target} PRIVATE -fsanitize=address,undefined)
  endif()
endfunction()

function(forge2d_enable_coverage target)
  if(FORGE2D_ENABLE_COVERAGE AND NOT MSVC)
    target_compile_options(${target} PRIVATE --coverage -O0 -g)
    target_link_options(${target} PRIVATE --coverage)
  endif()
endfunction()

add_library(forge2d_platform_c STATIC platform/arena.c)
target_include_directories(forge2d_platform_c
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/platform>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/forge2d/platform>)
forge2d_set_warnings(forge2d_platform_c)
forge2d_enable_sanitizers(forge2d_platform_c)
forge2d_enable_coverage(forge2d_platform_c)

add_library(forge2d_core STATIC
  src/collision.cpp
  src/pathfinding.cpp
  src/replay.cpp
  src/scene.cpp
  src/software_renderer.cpp
  src/world.cpp)
add_library(Forge2D::Core ALIAS forge2d_core)
target_include_directories(forge2d_core
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(forge2d_core PUBLIC forge2d_platform_c)
target_compile_definitions(forge2d_core PUBLIC FORGE2D_VERSION="${PROJECT_VERSION}")
forge2d_set_warnings(forge2d_core)
forge2d_enable_sanitizers(forge2d_core)
forge2d_enable_coverage(forge2d_core)

add_executable(forge2d_arena apps/forge2d_arena.cpp)
target_link_libraries(forge2d_arena PRIVATE Forge2D::Core)
forge2d_set_warnings(forge2d_arena)
forge2d_enable_sanitizers(forge2d_arena)
forge2d_enable_coverage(forge2d_arena)

if(FORGE2D_BUILD_SDL)
  include(FetchContent)
  set(SDL_SHARED OFF CACHE BOOL "" FORCE)
  set(SDL_STATIC ON CACHE BOOL "" FORCE)
  set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    SDL3
    URL https://github.com/libsdl-org/SDL/releases/download/release-3.4.16/SDL3-3.4.16.tar.gz
    URL_HASH SHA256=7322236cd12090c3eb40b9728be4d49c76f66ad17d04369584d4ecad5cf77c68
    EXCLUDE_FROM_ALL)
  FetchContent_MakeAvailable(SDL3)

  add_executable(forge2d_arena_sdl apps/forge2d_arena_sdl.cpp src/sdl_renderer.cpp)
  target_link_libraries(forge2d_arena_sdl PRIVATE Forge2D::Core SDL3::SDL3-static)
  forge2d_set_warnings(forge2d_arena_sdl)
  forge2d_enable_sanitizers(forge2d_arena_sdl)
  forge2d_enable_coverage(forge2d_arena_sdl)
endif()

if(FORGE2D_BUILD_TESTS)
  add_executable(forge2d_tests tests/test_main.cpp)
  target_link_libraries(forge2d_tests PRIVATE Forge2D::Core)
  forge2d_set_warnings(forge2d_tests)
  forge2d_enable_sanitizers(forge2d_tests)
  forge2d_enable_coverage(forge2d_tests)
  add_test(NAME forge2d_tests COMMAND forge2d_tests)
endif()

if(FORGE2D_BUILD_BENCHMARKS)
  add_executable(forge2d_benchmark benchmarks/benchmark.cpp)
  target_link_libraries(forge2d_benchmark PRIVATE Forge2D::Core)
  forge2d_set_warnings(forge2d_benchmark)
  forge2d_enable_sanitizers(forge2d_benchmark)
  forge2d_enable_coverage(forge2d_benchmark)
endif()

if(FORGE2D_BUILD_FUZZ)
  if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "FORGE2D_BUILD_FUZZ requires Clang")
  endif()
  add_executable(forge2d_scene_fuzz fuzz/scene_parser_fuzz.cpp)
  target_link_libraries(forge2d_scene_fuzz PRIVATE Forge2D::Core)
  target_compile_options(forge2d_scene_fuzz PRIVATE
    -fsanitize=fuzzer,address,undefined -fno-omit-frame-pointer)
  target_link_options(forge2d_scene_fuzz PRIVATE -fsanitize=fuzzer,address,undefined)
  forge2d_set_warnings(forge2d_scene_fuzz)
endif()

install(TARGETS forge2d_core forge2d_platform_c
  EXPORT Forge2DTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES platform/arena.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/forge2d/platform)
install(EXPORT Forge2DTargets
  NAMESPACE Forge2D::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Forge2D)

set(CPACK_PACKAGE_NAME "forge2d-engine")
set(CPACK_PACKAGE_VENDOR "Jason Stys")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_GENERATOR "ZIP;TGZ")
include(CPack)
