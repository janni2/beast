# tools/CMakeLists.txt

# Find required packages
find_package(GLIB2 REQUIRED COMPONENTS gio)
include_directories(SYSTEM ${GLIB2_INCLUDE_DIRS})

# BSEDEPS_PACKAGES - these are linked by bsetool, similar to libbse and BeastSoundEngine
# It's good practice for each CMakeLists.txt that needs them to find them,
# or to have them found at the root and provided as imported targets or variables.
# For now, repeat the find logic here for clarity, assuming this CMakeLists could be built standalone.
pkg_check_modules(FLUIDSYNTH REQUIRED fluidsynth>=2.0.5)
pkg_check_modules(VORBISENC REQUIRED vorbisenc>=1.3.2)
pkg_check_modules(VORBISFILE REQUIRED vorbisfile>=1.3.2)
pkg_check_modules(OGG REQUIRED ogg>=1.2.2)
pkg_check_modules(FLAC REQUIRED flac>=1.2.1)
pkg_check_modules(ZLIB REQUIRED zlib)

option(ENABLE_MAD "Enable MAD MP3 decoding support (for bsetool)" ON) # Inherit or set locally
if(ENABLE_MAD)
  pkg_check_modules(MAD mad>=0.14.2)
  if(NOT MAD_FOUND)
    message(WARNING "ENABLE_MAD is ON for bsetool but libmad not found. MAD features in bsetool may be disabled or build may fail.")
    set(ENABLE_MAD OFF) # Update local scope
  endif()
endif()

# libbse target is expected to be available from bse/CMakeLists.txt
set(BSE_TARGET_NAME bse)

# --- bsetool executable ---
set(BSETOOL_SOURCES
  bsetool.cc
  magictest.cc
  bsefcompare.cc
  bsefextract.cc
)

# Prepend CMAKE_CURRENT_SOURCE_DIR to source files
set(BSETOOL_FULLPATH_SOURCES "")
foreach(src IN LISTS BSETOOL_SOURCES)
  list(APPEND BSETOOL_FULLPATH_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${src}")
endforeach()

add_executable(bsetool ${BSETOOL_FULLPATH_SOURCES})

# Includes:
# - $(bse/libbse.deps) are headers generated in bse's build dir.
#   These are available via bse's INTERFACE_INCLUDE_DIRECTORIES.
# - GLIB_CFLAGS are from GLIB2_INCLUDE_DIRS.
# - -I$> (plugins build dir) is not relevant here, tools has its own.
target_include_directories(bsetool PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR} # For its own headers, if any
  $<TARGET_PROPERTY:bse,INTERFACE_INCLUDE_DIRECTORIES> # For headers from libbse and its generated deps
  ${GLIB2_INCLUDE_DIRS}
  # Add include dirs for BSEDEPS_PACKAGES if they are not transitively provided by libbse
  ${FLUIDSYNTH_INCLUDE_DIRS}
  ${VORBISENC_INCLUDE_DIRS}
  ${VORBISFILE_INCLUDE_DIRS}
  ${OGG_INCLUDE_DIRS}
  ${FLAC_INCLUDE_DIRS}
  ${ZLIB_INCLUDE_DIRS}
)
if(ENABLE_MAD AND MAD_FOUND)
  target_include_directories(bsetool PRIVATE ${MAD_INCLUDE_DIRS})
endif()

# Linking:
target_link_libraries(bsetool PRIVATE
  ${BSE_TARGET_NAME} # Link against libbse
  ${GLIB2_LIBRARIES}
  ${FLUIDSYNTH_LIBRARIES}
  ${VORBISENC_LIBRARIES}
  ${VORBISFILE_LIBRARIES}
  ${OGG_LIBRARIES}
  ${FLAC_LIBRARIES}
  ${ZLIB_LIBRARIES}
  stdc++fs # bse links this, if bsetool uses c++17 fs and bse doesn't expose it via interface
)
if(ENABLE_MAD AND MAD_FOUND)
  target_link_libraries(bsetool PRIVATE ${MAD_LIBRARIES})
endif()

# RPATH to find libbse if bsetool were installed.
# For build tree usage, CMake usually sets this up.
# If installed, e.g. bsetool to bin/ and libbse to lib/
set_target_properties(bsetool PROPERTIES
  INSTALL_RPATH "$ORIGIN/../lib" # Assuming relative path from bin to lib
)

# No installation rule for bsetool in the provided Makefile.mk.
# If it needs to be installed, add:
# install(TARGETS bsetool DESTINATION ${CMAKE_INSTALL_BINDIR})

message(STATUS "tools/CMakeLists.txt processed for bsetool.")
message(STATUS "  bsetool executable defined.")
message(STATUS "  Note: Installation for bsetool is not specified in its Makefile.mk snippet.")
message(STATUS "  It will be built in the build directory: ${CMAKE_CURRENT_BINARY_DIR}/bsetool")
