cmake_minimum_required(VERSION 3.20)
project(
  PhoneMonitor
  VERSION 0.2.0
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(PHONE_MONITOR_BUILD_RUNTIME
       "Build the Windows Rainmeter DLL and FPS collector" ${WIN32})
option(PHONE_MONITOR_BUILD_PROBE
       "Build the optional interactive Direct3D probe" OFF)
include(CTest)

function(monitor_target target)
  target_include_directories(${target} PRIVATE "${PROJECT_SOURCE_DIR}/src")
  if(WIN32)
    target_compile_definitions(${target} PRIVATE UNICODE _UNICODE NOMINMAX
                                                 WIN32_LEAN_AND_MEAN)
  endif()
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /WX /utf-8 /permissive-)
    set_property(
      TARGET ${target} PROPERTY MSVC_RUNTIME_LIBRARY
                                "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    if(MINGW)
      target_link_options(${target} PRIVATE -static -static-libgcc
                          -static-libstdc++ $<$<CONFIG:Release>:-s>)
    endif()
  endif()
endfunction()

add_library(
  monitor_core STATIC
  src/telemetry/service.cpp
  src/telemetry/file_fps_source.cpp
  src/fps/config.cpp
  src/fps/csv_reader.cpp
  src/fps/frame_history.cpp
  src/fps/process_filter.cpp
  src/fps/state_protocol.cpp)
monitor_target(monitor_core)

if(PHONE_MONITOR_BUILD_RUNTIME)
  if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(
      FATAL_ERROR
        "Rainmeter runtime requires 64-bit Windows; disable PHONE_MONITOR_BUILD_RUNTIME for core tests."
    )
  endif()
  add_library(
    monitor_windows STATIC
    src/platform/windows/cpu_provider.cpp
    src/platform/windows/nvml_gpu_provider.cpp
    src/platform/windows/text.cpp
    src/platform/windows/bridge_environment.cpp
    src/platform/windows/presentmon_process.cpp
    src/platform/windows/state_publisher.cpp)
  monitor_target(monitor_windows)
  target_link_libraries(monitor_windows PUBLIC monitor_core ole32 oleaut32
                                               wbemuuid)

  add_library(
    monitor_rainmeter STATIC src/rainmeter/host.cpp src/rainmeter/format.cpp
                             src/rainmeter/plugin.cpp)
  monitor_target(monitor_rainmeter)
  target_link_libraries(monitor_rainmeter PUBLIC monitor_windows)

  add_library(PhoneTelemetry SHARED src/rainmeter/exports.cpp)
  monitor_target(PhoneTelemetry)
  target_include_directories(PhoneTelemetry PRIVATE include)
  target_link_libraries(PhoneTelemetry PRIVATE monitor_rainmeter)
  set_target_properties(PhoneTelemetry
                        PROPERTIES PREFIX "" WINDOWS_EXPORT_ALL_SYMBOLS OFF)

  add_executable(FPSBridge WIN32 src/fps/main.cpp src/fps/application.cpp)
  monitor_target(FPSBridge)
  target_link_libraries(FPSBridge PRIVATE monitor_windows)
  if(MINGW)
    target_link_options(FPSBridge PRIVATE -municode)
  endif()

  if(PHONE_MONITOR_BUILD_PROBE)
    add_executable(PresentTest WIN32 tests/present_test.cpp)
    monitor_target(PresentTest)
    target_link_libraries(PresentTest PRIVATE d3d11 dxgi uuid)
    if(MINGW)
      target_link_options(PresentTest PRIVATE -municode)
    endif()
  endif()
endif()

if(BUILD_TESTING)
  add_executable(monitor_tests tests/core_tests.cpp)
  monitor_target(monitor_tests)
  target_link_libraries(monitor_tests PRIVATE monitor_core)
  add_test(NAME monitor_core_contracts COMMAND monitor_tests)
  if(PHONE_MONITOR_BUILD_RUNTIME)
    add_executable(monitor_capture_fixture tests/capture_fixture.cpp)
    monitor_target(monitor_capture_fixture)
    add_executable(monitor_windows_tests tests/windows_tests.cpp)
    monitor_target(monitor_windows_tests)
    target_link_libraries(monitor_windows_tests PRIVATE monitor_windows)
    add_test(NAME monitor_windows_contracts
             COMMAND monitor_windows_tests
                     $<TARGET_FILE:monitor_capture_fixture>)
    set_tests_properties(monitor_windows_contracts PROPERTIES TIMEOUT 30)
  endif()
endif()
