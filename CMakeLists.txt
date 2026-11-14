cmake_minimum_required(VERSION 3.20)
project(
  PhoneMonitor
  VERSION 0.1.0
  LANGUAGES CXX)

if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "Phone Monitor requires a 64-bit Windows toolchain.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(PhoneTelemetry SHARED src/telemetry.cpp)
set_target_properties(PhoneTelemetry PROPERTIES PREFIX "")
target_link_libraries(PhoneTelemetry PRIVATE advapi32)
add_executable(FPSBridge WIN32 src/fps_bridge.cpp)
option(PHONE_MONITOR_BUILD_PROBE "Build the short-lived Direct3D FPS probe" OFF)
if(PHONE_MONITOR_BUILD_PROBE)
  add_executable(PresentTest WIN32 tests/present_test.cpp)
  target_link_libraries(PresentTest PRIVATE d3d11 dxgi uuid)
endif()

foreach(target PhoneTelemetry FPSBridge PresentTest)
  if(NOT TARGET ${target})
    continue()
  endif()
  target_compile_definitions(${target} PRIVATE UNICODE _UNICODE NOMINMAX
                                               WIN32_LEAN_AND_MEAN)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /WX /utf-8 /permissive-)
    set_property(
      TARGET ${target} PROPERTY MSVC_RUNTIME_LIBRARY
                                "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_link_options(${target} PRIVATE -static -static-libgcc
                        -static-libstdc++ $<$<CONFIG:Release>:-s>)
    if(NOT target STREQUAL "PhoneTelemetry")
      target_link_options(${target} PRIVATE -municode)
    endif()
  endif()
endforeach()
