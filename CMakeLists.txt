cmake_minimum_required(VERSION 3.10)

# Get project version from version.sh
execute_process(
  COMMAND ./version.sh -s
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE BEAST_VERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
project(Beast LANGUAGES C CXX VERSION ${BEAST_VERSION})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# --- Build Mode & Sanitizer Options ---
# Standard build types are handled by CMAKE_BUILD_TYPE (Debug, Release, etc.)
# CMake's default flags for Debug (-g) and Release (-O3 -DNDEBUG) are generally used.
# The "quick" mode from the Makefile can be mapped to a Debug build.

option(USE_ASAN "Enable AddressSanitizer" OFF)
option(USE_LSAN "Enable LeakSanitizer" OFF)
option(USE_TSAN "Enable ThreadSanitizer" OFF)
option(USE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

if(USE_ASAN)
  message(STATUS "AddressSanitizer (ASan) enabled")
  add_compile_options(-fsanitize=address)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address")
endif()

if(USE_LSAN)
  message(STATUS "LeakSanitizer (LSan) enabled")
  if(NOT USE_ASAN)
    message(WARNING "LSan is typically used with ASan. Enabling ASan is recommended for LSan to be most effective.")
  endif()
  add_compile_options(-fsanitize=leak)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=leak")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=leak")
endif()

if(USE_TSAN)
  message(STATUS "ThreadSanitizer (TSan) enabled")
  add_compile_options(-fsanitize=thread)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=thread")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=thread")
endif()

if(USE_UBSAN)
  message(STATUS "UndefinedBehaviorSanitizer (UBSan) enabled")
  # Common UBSan checks. Add more as needed e.g. -fsanitize=float-divide-by-zero,etc.
  add_compile_options(-fsanitize=undefined -fsanitize=integer -fsanitize=nullability -fsanitize=bounds)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=undefined -fsanitize=integer -fsanitize=nullability -fsanitize=bounds")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=undefined -fsanitize=integer -fsanitize=nullability -fsanitize=bounds")
endif()

# --- Output Directories ---
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Set build type options
set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose build type: Debug Release RelWithDebInfo MinSizeRel")
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Add subdirectories
add_subdirectory(aidacc)
add_subdirectory(bse)
add_subdirectory(devices)
add_subdirectory(plugins)
add_subdirectory(sfi)
add_subdirectory(tools)
add_subdirectory(tests)
add_subdirectory(ebeast)
add_subdirectory(launchers)
# add_subdirectory(docs) # Might not be needed for CMake
# add_subdirectory(res) # Resource files, handle separately
# add_subdirectory(images) # Image files, handle separately
# add_subirectory(po) # Localization files, handle separately
# add_subdirectory(data) # Data files, handle separately
# add_subdirectory(media) # Media files, handle separately
# add_subdirectory(misc) # Misc files, handle separately
