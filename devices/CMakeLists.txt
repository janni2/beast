# devices/CMakeLists.txt

# Find all .cc files recursively in the current source directory (devices/)
file(GLOB_RECURSE DEVICE_SOURCES
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)

if(NOT DEVICE_SOURCES)
  message(STATUS "No sources found in devices/ directory. Creating an empty bse_devices_objlib.")
  # Create an empty object library if no sources are found to prevent errors with $<TARGET_OBJECTS:>
  # if other parts of the build system expect this target.
  add_library(bse_devices_objlib OBJECT "")
  return()
endif()

# Create an OBJECT library from these sources.
# Object libraries compile sources but don't link them into a library archive/shared object.
# Instead, their object files can be included in other targets (like libbse).
add_library(bse_devices_objlib OBJECT ${DEVICE_SOURCES})

# Apply specific compile options to sources if needed
# Example from Makefile: $>/devices/freeverb/freeverb.o: EXTRA_FLAGS ::= -Wno-unused-function
# Convert path to be relative to CMAKE_CURRENT_SOURCE_DIR for set_source_files_properties
set(FREEVERB_SOURCE_FILE "freeverb/freeverb.cc") # Path relative to devices/
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${FREEVERB_SOURCE_FILE}")
  set_source_files_properties(
    ${FREEVERB_SOURCE_FILE}
    PROPERTIES COMPILE_FLAGS "-Wno-unused-function"
  )
else()
  message(WARNING "Source file for special compile flag not found: ${CMAKE_CURRENT_SOURCE_DIR}/${FREEVERB_SOURCE_FILE}")
endif()

# To make these object files available to bse/CMakeLists.txt:
# 1. In the root CMakeLists.txt, ensure add_subdirectory(devices) is called before add_subdirectory(bse).
# 2. In bse/CMakeLists.txt, add the objects from this library to libbse's sources:
#    target_sources(bse PRIVATE $<TARGET_OBJECTS:bse_devices_objlib>)
#    (Or alternatively, link bse against bse_devices_objlib if it were a static/shared lib,
#     but for object contributions, target_sources is appropriate.)

# Add include directories that might be needed by sources within devices/
# Typically, devices/ might have its own include structure or rely on bse/ includes.
# For now, assume sources use paths relative to devices/ or include paths are set by bse.
target_include_directories(bse_devices_objlib INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> # Allow includes relative to devices/ itself
)

# If devices sources need includes from bse/ or generated headers from bse's build dir:
# This creates a circular dependency if bse also depends on devices.
# It's better if bse provides its include directories as INTERFACE to its dependents.
# For now, assume devices code is self-contained or relies on includes that will be
# provided when linked into libbse.

message(STATUS "devices/CMakeLists.txt: Created bse_devices_objlib object library.")
message(STATUS "  Sources: ${DEVICE_SOURCES}")

# No installation rules here as these objects become part of libbse, which is installed by bse/CMakeLists.txt.
# No executables or standalone libraries defined here.
