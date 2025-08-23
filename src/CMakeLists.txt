# src/CMakeLists.txt -- CMake build file for GORP, intended for use with GCC.
# For Linux builds, make a build folder and just `cmake ..`
# For Windows builds, use the apt package mingw-w64 and cross-compile from Linux or Ubuntu WSL, using `cmake .. -DCROSS_COMPILE_WINDOWS=1`
# See CMakeLists.txt in the root folder for optional build parameters.

# SPDX-FileType: SOURCE
# SPDX-FileCopyrightText: Copyright 2025 Raine Simmons <gc@gravecat.com>
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.13)

# Automatically generate version strings used in-game and in the Windows resource file, from this specified game version.
set(GORP_VERSION "0.0.0.1")
string(REPLACE "." "," GORP_VERSION_COMMA "${GORP_VERSION}")
execute_process(COMMAND date "+%y%m%d%H%M" OUTPUT_VARIABLE BUILD_TIMESTAMP OUTPUT_STRIP_TRAILING_WHITESPACE)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/cmake/version.hpp @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/resource.rc.in ${CMAKE_CURRENT_BINARY_DIR}/cmake/resource.rc @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/source.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/cmake/source.hpp @ONLY)

# Non-platform-specific stuff.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic -pedantic-errors")
if(WARNINGS_AS_ERRORS)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
endif(WARNINGS_AS_ERRORS)
set(CMAKE_CXX_FLAGS_DEBUG "-g -Og")
set(CMAKE_CXX_FLAGS_RELEASE  "-O2")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-s")
add_definitions(-D__STDC_LIMIT_MACROS)
add_definitions(-D_USE_MATH_DEFINES)
set(GORP_BIN "$<TARGET_FILE:gorp>")

# Check to ensure C++17 is supported.
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-std=c++14" COMPILER_SUPPORTS_CXX14)
check_cxx_compiler_flag("-std=c++17" COMPILER_SUPPORTS_CXX17)
if(COMPILER_SUPPORTS_CXX14)
  add_definitions(-DHAVE_CXX14)
  if(COMPILER_SUPPORTS_CXX17)
    add_definitions(-DHAVE_CXX17)
  else()
    message(FATAL_ERROR "Compiler does not support C++17")
  endif(COMPILER_SUPPORTS_CXX17)
else()
  message(FATAL_ERROR "Compiler does not support C++14")
endif(COMPILER_SUPPORTS_CXX14)

# Windows-specific settings, cross-compiling from Linux.
if(TARGET_WINDOWS)
  if(TARGET_I686)
    set(CMAKE_SYSTEM_PROCESSOR x86)
    set(CMAKE_AR "/usr/bin/x86_64-w64-mingw32-ar")
    set(CMAKE_RANLIB "/usr/bin/i686-w64-mingw32-ranlib")
    set(CMAKE_LINKER "/usr/bin/i686-w64-mingw32-ld")
    set(CMAKE_C_COMPILER i686-w64-mingw32-gcc)
    set(CMAKE_CXX_COMPILER i686-w64-mingw32-g++)
    set(CMAKE_RC_COMPILER i686-w64-mingw32-windres)
  else(TARGET_I686)
    set(CMAKE_SYSTEM_PROCESSOR x86_64)
    set(CMAKE_AR "/usr/bin/x86_64-w64-mingw32-ar")
    set(CMAKE_RANLIB "/usr/bin/x86_64-w64-mingw32-ranlib")
    set(CMAKE_LINKER "/usr/bin/x86_64-w64-mingw32-ld")
    set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
    set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
    set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)
  endif(TARGET_I686)
  set(CMAKE_SYSTEM_NAME Windows)
  set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
  set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
  set(GORP_BIN "$<TARGET_FILE:gorp>.exe")

  # Platform-specific libraries.
  add_definitions(-DWIN32_LEAN_AND_MEAN)
  set(OS_LIBRARIES mingw32 shlwapi)
  if(DEBUG_BUILD)
    set(SFML_LIBRARIES sfml-graphics-d sfml-window-d sfml-audio-d sfml-system-d)
  else()
    set(SFML_LIBRARIES sfml-graphics sfml-window sfml-audio sfml-system)
  endif()

  # Windows resource file, includes application icon.
  enable_language(RC)
  set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
  set(GORP_RC "${CMAKE_CURRENT_BINARY_DIR}/cmake/resource.rc")
endif(TARGET_WINDOWS)

# Linux-specific settings.
if(TARGET_LINUX)
  if(TARGET_I686)
    set(CMAKE_SYSTEM_PROCESSOR x86)
    set(CMAKE_C_FLAGS "-m32")
    set(CMAKE_CXX_FLAGS "-m32")
  else(TARGET_I686)
    set(CMAKE_SYSTEM_PROCESSOR x86_64)
  endif(TARGET_I686)
  set(CMAKE_SYSTEM_NAME Linux)

  # Platform-specific libraries.
  set(OS_LIBRARIES X11 Xrandr Xi Xcursor udev FLAC vorbis vorbisenc vorbisfile ogg)
  if(DEBUG_BUILD)
    set(SFML_LIBRARIES sfml-graphics-s-d sfml-window-s-d sfml-audio-s-d sfml-system-s-d)
  else()
    set(SFML_LIBRARIES sfml-graphics-s sfml-window-s sfml-audio-s sfml-system-s)
  endif()
endif(TARGET_LINUX)

# Source files.
set(GORP_CPPS
  core/core.cpp
  core/game.cpp
  core/guru.cpp
  core/prefs.cpp
  core/terminal/colour-maps.cpp
  core/terminal/terminal.cpp
  core/terminal/window.cpp
  ui/element.cpp
  util/file/binpath.cpp
  util/file/filereader.cpp
  util/file/fileutils.cpp
  util/file/filewriter.cpp
  util/file/yaml.cpp
  util/stringutils.cpp
)

# Binary file. GORP_RC should be blank for non-Windows builds.
add_executable(gorp ${GORP_CPPS} ${GORP_RC})
target_compile_features(gorp PRIVATE cxx_std_17)
if(TARGET_WINDOWS)
  target_link_options(gorp PRIVATE -mwindows)
endif(TARGET_WINDOWS)

# Include directories.
target_include_directories(gorp PRIVATE
  "${CMAKE_SOURCE_DIR}/src"
#  "${CMAKE_SOURCE_DIR}/src/3rdparty"
  "${CMAKE_SOURCE_DIR}/src/3rdparty/sfml/include"
  "${CMAKE_CURRENT_BINARY_DIR}"
)

# Platform-specific library directories.
if(TARGET_WINDOWS)
  target_link_directories(gorp PRIVATE "${CMAKE_SOURCE_DIR}/windows/lib")
endif(TARGET_WINDOWS)
if(TARGET_LINUX)
  target_link_directories(gorp PRIVATE "${CMAKE_SOURCE_DIR}/linux/lib")
endif(TARGET_LINUX)

# Link libraries. Platform-specific stuff should be set in the main platform-specific section near the top.
target_link_libraries(gorp
  ${SFML_LIBRARIES}
  ${OS_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  fantasyname
  rapidyaml
  sam
  sfxr
)

# Post-build, make a 'bin' folder and copy the binary file in there.
add_custom_command(TARGET gorp POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/bin"
  COMMAND ${CMAKE_COMMAND} -E copy "${GORP_BIN}" "${CMAKE_BINARY_DIR}/bin"
)

# Build some third-party code as separate binaries to be linked in.
add_subdirectory(3rdparty/fantasyname)
add_subdirectory(3rdparty/rapidyaml)
add_subdirectory(3rdparty/sam)
add_subdirectory(3rdparty/sfxr)
