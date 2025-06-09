# SFI (SFIDL Tool and Test) CMakeLists.txt

# Find required packages
find_package(GLIB2 REQUIRED COMPONENTS gio) # For glib includes and libs
include_directories(SYSTEM ${GLIB2_INCLUDE_DIRS})

# --- sfidl executable ---
set(SFIDL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/sfidl.cc)
set(SFIDL_EXECUTABLE_NAME sfidl) # Output name
set(SFIDL_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/${SFIDL_EXECUTABLE_NAME})

add_executable(${SFIDL_EXECUTABLE_NAME} ${SFIDL_SOURCES})

target_compile_definitions(${SFIDL_EXECUTABLE_NAME} PRIVATE
  -DG_LOG_DOMAIN="SFI"
  -DG_DISABLE_CONST_RETURNS
  -DPARANOID
  -DSFIDL_INTERNALS
)
# Makefile had -O0 for sfidl.cc.FLAGS, apply if needed for debug, or let CMake manage optimization.
# For consistency or debugging:
target_compile_options(${SFIDL_EXECUTABLE_NAME} PRIVATE -O0)

target_link_libraries(${SFIDL_EXECUTABLE_NAME} PRIVATE ${GLIB2_LIBRARIES})

# Make sfidl executable available to other CMakeLists.txt files (e.g., bse)
# This creates an alias target `User::sfidl` that can be used by bse.
# However, direct path usage is also common for tools built within the same project.
# For now, bse/CMakeLists.txt uses a variable SFIDL_EXECUTABLE pointing to the path.
# If we want an imported target, sfi/CMakeLists.txt would need to install the target
# and export it, or use add_subdirectory from the root and direct target dependency.
# For a tool used during build, path is fine.

# --- testsfidl executable ---
set(TESTSFIDL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/testsfidl.cc)
set(TESTSFIDL_EXECUTABLE_NAME testsfidl)
set(TESTSFIDL_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/${TESTSFIDL_EXECUTABLE_NAME})

add_executable(${TESTSFIDL_EXECUTABLE_NAME} ${TESTSFIDL_SOURCES})

# testsfidl includes -I$> which is CMAKE_CURRENT_BINARY_DIR (for generated headers if any from sfidl itself - none here)
# and GLIB_CFLAGS (already handled by find_package and include_directories)
target_include_directories(${TESTSFIDL_EXECUTABLE_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
# Makefile had -O0 for testsfidl.cc.FLAGS
target_compile_options(${TESTSFIDL_EXECUTABLE_NAME} PRIVATE -O0)

target_link_libraries(${TESTSFIDL_EXECUTABLE_NAME} PRIVATE ${GLIB2_LIBRARIES})

# --- Test ---
# sfi-check: FORCE | $(sfi/testsfidl)
# 	$(QECHO) RUN $@
#	$Q $(sfi/testsfidl)
add_test(NAME sfi-check
  COMMAND ${TESTSFIDL_OUTPUT_PATH}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Installation - sfidl tool is usually not installed, it's a build tool.
# testsfidl is also a test, not typically installed.
# If sfidl were to be installed:
# install(TARGETS ${SFIDL_EXECUTABLE_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})

message(STATUS "SFI CMakeLists.txt processed.")
message(STATUS "  SFIDL executable will be at: ${SFIDL_OUTPUT_PATH}")
message(STATUS "  TESTSFIDL executable will be at: ${TESTSFIDL_OUTPUT_PATH}")

# If sfidl had public headers or generated IDL support files that other components
# (like bse when processing IDLs) would need to include, this is where you'd
# install them and potentially create a CMake export file.
# Based on sfi/Makefile.mk, it doesn't seem to produce such files for export.
# The $(sfi/sfidl.includes) variable remains a bit of a mystery, possibly
# an empty variable or set globally for project-specific IDL search paths.
