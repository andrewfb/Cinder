cmake_minimum_required(VERSION 3.10)
project(CinderBenchmarks)

# Force Release build for accurate benchmarking
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

message(STATUS "Benchmark Build Type: ${CMAKE_BUILD_TYPE}")

# Find Cinder (adjust path as needed)
get_filename_component(CINDER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set(CINDER_INCLUDE_DIR "${CINDER_PATH}/include")
set(CINDER_LIB_DIR "${CINDER_PATH}/lib/macosx/${CMAKE_BUILD_TYPE}")

# Platform-specific library paths
if(APPLE)
    set(CINDER_LIB_DIR "${CINDER_PATH}/lib/macosx/${CMAKE_BUILD_TYPE}")
elseif(WIN32)
    set(CINDER_LIB_DIR "${CINDER_PATH}/lib/msw/x64/${CMAKE_BUILD_TYPE}")
else()
    set(CINDER_LIB_DIR "${CINDER_PATH}/lib/linux/${CMAKE_SYSTEM_PROCESSOR}/${CMAKE_BUILD_TYPE}")
endif()

message(STATUS "Cinder library directory: ${CINDER_LIB_DIR}")

# Create benchmark executable
add_executable(CinderBenchmarks
    src/BenchmarkMain.cpp
    src/QuadraticSolverBenchmark.cpp
    src/BezierUtilitiesBenchmark.cpp
    src/Path2dBenchmark.cpp
)

# Create quartic test executable
add_executable(QuarticTest
    src/QuarticTest.cpp
)

add_executable(QuarticDebug
    src/QuarticDebug.cpp
)

add_executable(QuarticTrace
    src/QuarticTrace.cpp
)

add_executable(CubicDebug
    src/CubicDebug.cpp
)

add_executable(QuarticDebug2
    src/QuarticDebug2.cpp
)

add_executable(CubicSolverComparison
    src/CubicSolverComparison.cpp
)

add_executable(OffsetCurveTest
    src/OffsetCurveTest.cpp
)

add_executable(OffsetCuspTest
    src/OffsetCuspTest.cpp
)

add_executable(MiterJoinTest
    src/MiterJoinTest.cpp
)

# Include directories
target_include_directories(CinderBenchmarks PRIVATE
    ${CINDER_INCLUDE_DIR}
)

target_include_directories(QuarticTest PRIVATE
    ${CINDER_INCLUDE_DIR}
)

target_include_directories(QuarticDebug PRIVATE
    ${CINDER_INCLUDE_DIR}
)

target_include_directories(QuarticTrace PRIVATE
    ${CINDER_INCLUDE_DIR}
)

target_include_directories(CubicDebug PRIVATE
    ${CINDER_INCLUDE_DIR}
)

target_include_directories(QuarticDebug2 PRIVATE
    ${CINDER_INCLUDE_DIR}
)

target_include_directories(CubicSolverComparison PRIVATE
    ${CINDER_INCLUDE_DIR}
)

target_include_directories(OffsetCurveTest PRIVATE
    ${CINDER_INCLUDE_DIR}
)

target_include_directories(OffsetCuspTest PRIVATE
    ${CINDER_INCLUDE_DIR}
)

target_include_directories(MiterJoinTest PRIVATE
    ${CINDER_INCLUDE_DIR}
)

# Link against Cinder
target_link_libraries(CinderBenchmarks PRIVATE
    ${CINDER_LIB_DIR}/libcinder.a
)

target_link_libraries(QuarticTest PRIVATE
    ${CINDER_LIB_DIR}/libcinder.a
)

target_link_libraries(QuarticDebug PRIVATE
    ${CINDER_LIB_DIR}/libcinder.a
)

target_link_libraries(QuarticTrace PRIVATE
    ${CINDER_LIB_DIR}/libcinder.a
)

target_link_libraries(CubicDebug PRIVATE
    ${CINDER_LIB_DIR}/libcinder.a
)

target_link_libraries(QuarticDebug2 PRIVATE
    ${CINDER_LIB_DIR}/libcinder.a
)

target_link_libraries(CubicSolverComparison PRIVATE
    ${CINDER_LIB_DIR}/libcinder.a
)

target_link_libraries(OffsetCurveTest PRIVATE
    ${CINDER_LIB_DIR}/libcinder.a
)

target_link_libraries(OffsetCuspTest PRIVATE
    ${CINDER_LIB_DIR}/libcinder.a
)

target_link_libraries(MiterJoinTest PRIVATE
    ${CINDER_LIB_DIR}/libcinder.a
)

# Platform-specific settings
if(APPLE)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(OPENGL_LIBRARY OpenGL)
    find_library(COREAUDIO_LIBRARY CoreAudio)
    find_library(AUDIOTOOLBOX_LIBRARY AudioToolbox)
    find_library(COREVIDEO_LIBRARY CoreVideo)
    find_library(COREMEDIA_LIBRARY CoreMedia)
    find_library(AVFOUNDATION_LIBRARY AVFoundation)
    find_library(ACCELERATE_LIBRARY Accelerate)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    find_library(IOSURFACE_LIBRARY IOSurface)

    target_link_libraries(CinderBenchmarks PRIVATE
        ${COCOA_LIBRARY}
        ${OPENGL_LIBRARY}
        ${COREAUDIO_LIBRARY}
        ${AUDIOTOOLBOX_LIBRARY}
        ${COREVIDEO_LIBRARY}
        ${COREMEDIA_LIBRARY}
        ${AVFOUNDATION_LIBRARY}
        ${ACCELERATE_LIBRARY}
        ${IOKIT_LIBRARY}
        ${COREFOUNDATION_LIBRARY}
        ${IOSURFACE_LIBRARY}
    )

    target_link_libraries(QuarticTest PRIVATE
        ${COCOA_LIBRARY}
        ${OPENGL_LIBRARY}
        ${COREAUDIO_LIBRARY}
        ${AUDIOTOOLBOX_LIBRARY}
        ${COREVIDEO_LIBRARY}
        ${COREMEDIA_LIBRARY}
        ${AVFOUNDATION_LIBRARY}
        ${ACCELERATE_LIBRARY}
        ${IOKIT_LIBRARY}
        ${COREFOUNDATION_LIBRARY}
        ${IOSURFACE_LIBRARY}
    )

    target_link_libraries(QuarticDebug PRIVATE
        ${COCOA_LIBRARY}
        ${OPENGL_LIBRARY}
        ${COREAUDIO_LIBRARY}
        ${AUDIOTOOLBOX_LIBRARY}
        ${COREVIDEO_LIBRARY}
        ${COREMEDIA_LIBRARY}
        ${AVFOUNDATION_LIBRARY}
        ${ACCELERATE_LIBRARY}
        ${IOKIT_LIBRARY}
        ${COREFOUNDATION_LIBRARY}
        ${IOSURFACE_LIBRARY}
    )

    target_link_libraries(QuarticTrace PRIVATE
        ${COCOA_LIBRARY}
        ${OPENGL_LIBRARY}
        ${COREAUDIO_LIBRARY}
        ${AUDIOTOOLBOX_LIBRARY}
        ${COREVIDEO_LIBRARY}
        ${COREMEDIA_LIBRARY}
        ${AVFOUNDATION_LIBRARY}
        ${ACCELERATE_LIBRARY}
        ${IOKIT_LIBRARY}
        ${COREFOUNDATION_LIBRARY}
        ${IOSURFACE_LIBRARY}
    )

    target_link_libraries(CubicDebug PRIVATE
        ${COCOA_LIBRARY}
        ${OPENGL_LIBRARY}
        ${COREAUDIO_LIBRARY}
        ${AUDIOTOOLBOX_LIBRARY}
        ${COREVIDEO_LIBRARY}
        ${COREMEDIA_LIBRARY}
        ${AVFOUNDATION_LIBRARY}
        ${ACCELERATE_LIBRARY}
        ${IOKIT_LIBRARY}
        ${COREFOUNDATION_LIBRARY}
        ${IOSURFACE_LIBRARY}
    )

    target_link_libraries(QuarticDebug2 PRIVATE
        ${COCOA_LIBRARY}
        ${OPENGL_LIBRARY}
        ${COREAUDIO_LIBRARY}
        ${AUDIOTOOLBOX_LIBRARY}
        ${COREVIDEO_LIBRARY}
        ${COREMEDIA_LIBRARY}
        ${AVFOUNDATION_LIBRARY}
        ${ACCELERATE_LIBRARY}
        ${IOKIT_LIBRARY}
        ${COREFOUNDATION_LIBRARY}
        ${IOSURFACE_LIBRARY}
    )

    target_link_libraries(CubicSolverComparison PRIVATE
        ${COCOA_LIBRARY}
        ${OPENGL_LIBRARY}
        ${COREAUDIO_LIBRARY}
        ${AUDIOTOOLBOX_LIBRARY}
        ${COREVIDEO_LIBRARY}
        ${COREMEDIA_LIBRARY}
        ${AVFOUNDATION_LIBRARY}
        ${ACCELERATE_LIBRARY}
        ${IOKIT_LIBRARY}
        ${COREFOUNDATION_LIBRARY}
        ${IOSURFACE_LIBRARY}
    )

    target_link_libraries(OffsetCurveTest PRIVATE
        ${COCOA_LIBRARY}
        ${OPENGL_LIBRARY}
        ${COREAUDIO_LIBRARY}
        ${AUDIOTOOLBOX_LIBRARY}
        ${COREVIDEO_LIBRARY}
        ${COREMEDIA_LIBRARY}
        ${AVFOUNDATION_LIBRARY}
        ${ACCELERATE_LIBRARY}
        ${IOKIT_LIBRARY}
        ${COREFOUNDATION_LIBRARY}
        ${IOSURFACE_LIBRARY}
    )

    target_link_libraries(OffsetCuspTest PRIVATE
        ${COCOA_LIBRARY}
        ${OPENGL_LIBRARY}
        ${COREAUDIO_LIBRARY}
        ${AUDIOTOOLBOX_LIBRARY}
        ${COREVIDEO_LIBRARY}
        ${COREMEDIA_LIBRARY}
        ${AVFOUNDATION_LIBRARY}
        ${ACCELERATE_LIBRARY}
        ${IOKIT_LIBRARY}
        ${COREFOUNDATION_LIBRARY}
        ${IOSURFACE_LIBRARY}
    )

    target_link_libraries(MiterJoinTest PRIVATE
        ${COCOA_LIBRARY}
        ${OPENGL_LIBRARY}
        ${COREAUDIO_LIBRARY}
        ${AUDIOTOOLBOX_LIBRARY}
        ${COREVIDEO_LIBRARY}
        ${COREMEDIA_LIBRARY}
        ${AVFOUNDATION_LIBRARY}
        ${ACCELERATE_LIBRARY}
        ${IOKIT_LIBRARY}
        ${COREFOUNDATION_LIBRARY}
        ${IOSURFACE_LIBRARY}
    )
elseif(UNIX AND NOT APPLE)
    find_package(OpenGL REQUIRED)
    target_link_libraries(CinderBenchmarks PRIVATE
        OpenGL::GL
        pthread
        dl
    )
    target_link_libraries(QuarticTest PRIVATE
        OpenGL::GL
        pthread
        dl
    )
    target_link_libraries(QuarticDebug PRIVATE
        OpenGL::GL
        pthread
        dl
    )
    target_link_libraries(QuarticTrace PRIVATE
        OpenGL::GL
        pthread
        dl
    )
    target_link_libraries(CubicDebug PRIVATE
        OpenGL::GL
        pthread
        dl
    )
    target_link_libraries(QuarticDebug2 PRIVATE
        OpenGL::GL
        pthread
        dl
    )
    target_link_libraries(CubicSolverComparison PRIVATE
        OpenGL::GL
        pthread
        dl
    )
    target_link_libraries(OffsetCurveTest PRIVATE
        OpenGL::GL
        pthread
        dl
    )
    target_link_libraries(OffsetCuspTest PRIVATE
        OpenGL::GL
        pthread
        dl
    )
    target_link_libraries(MiterJoinTest PRIVATE
        OpenGL::GL
        pthread
        dl
    )
elseif(WIN32)
    target_link_libraries(CinderBenchmarks PRIVATE
        opengl32
    )
    target_link_libraries(QuarticTest PRIVATE
        opengl32
    )
    target_link_libraries(QuarticDebug PRIVATE
        opengl32
    )
    target_link_libraries(QuarticTrace PRIVATE
        opengl32
    )
    target_link_libraries(CubicDebug PRIVATE
        opengl32
    )
    target_link_libraries(QuarticDebug2 PRIVATE
        opengl32
    )
    target_link_libraries(CubicSolverComparison PRIVATE
        opengl32
    )
    target_link_libraries(OffsetCurveTest PRIVATE
        opengl32
    )
    target_link_libraries(OffsetCuspTest PRIVATE
        opengl32
    )
    target_link_libraries(MiterJoinTest PRIVATE
        opengl32
    )
endif()

# C++ standard
set_target_properties(CinderBenchmarks PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
)

set_target_properties(QuarticTest PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
)

set_target_properties(QuarticDebug PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
)

set_target_properties(QuarticTrace PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
)

set_target_properties(CubicDebug PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
)

set_target_properties(QuarticDebug2 PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
)

set_target_properties(CubicSolverComparison PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
)

set_target_properties(OffsetCurveTest PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
)

set_target_properties(OffsetCuspTest PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
)

set_target_properties(MiterJoinTest PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
)

# Optimization flags for Release
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        target_compile_options(CinderBenchmarks PRIVATE /O2 /Ob2)
    elseif(APPLE)
        target_compile_options(CinderBenchmarks PRIVATE -O3)
    else()
        target_compile_options(CinderBenchmarks PRIVATE -O3 -march=native)
    endif()
endif()

message(STATUS "Benchmark executable will be built in: ${CMAKE_CURRENT_BINARY_DIR}")

add_executable(ClosedPathTest
    src/ClosedPathTest.cpp
)

target_include_directories(ClosedPathTest PRIVATE
    ${CINDER_INCLUDE_DIR}
)

target_link_libraries(ClosedPathTest PRIVATE
    ${CINDER_LIB_DIR}/libcinder.a
)

if(APPLE)
    target_link_libraries(ClosedPathTest PRIVATE
        ${COCOA_LIBRARY}
        ${OPENGL_LIBRARY}
        ${COREAUDIO_LIBRARY}
        ${AUDIOTOOLBOX_LIBRARY}
        ${COREVIDEO_LIBRARY}
        ${COREMEDIA_LIBRARY}
        ${AVFOUNDATION_LIBRARY}
        ${ACCELERATE_LIBRARY}
        ${IOKIT_LIBRARY}
        ${COREFOUNDATION_LIBRARY}
        ${IOSURFACE_LIBRARY}
    )
elseif(UNIX AND NOT APPLE)
    target_link_libraries(ClosedPathTest PRIVATE
        OpenGL::GL
        pthread
        dl
    )
elseif(WIN32)
    target_link_libraries(ClosedPathTest PRIVATE
        opengl32
    )
endif()

set_target_properties(ClosedPathTest PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
)

add_executable(OffsetDistanceTest
    src/OffsetDistanceTest.cpp
)

add_executable(QuadraticConversionTest
    src/QuadraticConversionTest.cpp
)

target_include_directories(OffsetDistanceTest PRIVATE
    ${CINDER_INCLUDE_DIR}
)

target_include_directories(QuadraticConversionTest PRIVATE
    ${CINDER_INCLUDE_DIR}
)

target_link_libraries(OffsetDistanceTest PRIVATE
    ${CINDER_LIB_DIR}/libcinder.a
)

target_link_libraries(QuadraticConversionTest PRIVATE
    ${CINDER_LIB_DIR}/libcinder.a
)

if(APPLE)
    target_link_libraries(OffsetDistanceTest PRIVATE
        ${COCOA_LIBRARY}
        ${OPENGL_LIBRARY}
        ${COREAUDIO_LIBRARY}
        ${AUDIOTOOLBOX_LIBRARY}
        ${COREVIDEO_LIBRARY}
        ${COREMEDIA_LIBRARY}
        ${AVFOUNDATION_LIBRARY}
        ${ACCELERATE_LIBRARY}
        ${IOKIT_LIBRARY}
        ${COREFOUNDATION_LIBRARY}
        ${IOSURFACE_LIBRARY}
    )
    target_link_libraries(QuadraticConversionTest PRIVATE
        ${COCOA_LIBRARY}
        ${OPENGL_LIBRARY}
        ${COREAUDIO_LIBRARY}
        ${AUDIOTOOLBOX_LIBRARY}
        ${COREVIDEO_LIBRARY}
        ${COREMEDIA_LIBRARY}
        ${AVFOUNDATION_LIBRARY}
        ${ACCELERATE_LIBRARY}
        ${IOKIT_LIBRARY}
        ${COREFOUNDATION_LIBRARY}
        ${IOSURFACE_LIBRARY}
    )
elseif(UNIX AND NOT APPLE)
    target_link_libraries(OffsetDistanceTest PRIVATE
        OpenGL::GL
        pthread
        dl
    )
    target_link_libraries(QuadraticConversionTest PRIVATE
        OpenGL::GL
        pthread
        dl
    )
elseif(WIN32)
    target_link_libraries(OffsetDistanceTest PRIVATE
        opengl32
    )
    target_link_libraries(QuadraticConversionTest PRIVATE
        opengl32
    )
endif()

set_target_properties(OffsetDistanceTest PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
)

set_target_properties(QuadraticConversionTest PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
)
