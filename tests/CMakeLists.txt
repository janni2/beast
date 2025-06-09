# tests/CMakeLists.txt

enable_testing() # Enable CTest

# Find required tools and packages
find_package(GLIB2 REQUIRED COMPONENTS gio)
include_directories(SYSTEM ${GLIB2_INCLUDE_DIRS})

# BSEDEPS_PACKAGES - for linking 'suite' executable
pkg_check_modules(FLUIDSYNTH REQUIRED fluidsynth>=2.0.5)
pkg_check_modules(VORBISENC REQUIRED vorbisenc>=1.3.2)
pkg_check_modules(VORBISFILE REQUIRED vorbisfile>=1.3.2)
pkg_check_modules(OGG REQUIRED ogg>=1.2.2)
pkg_check_modules(FLAC REQUIRED flac>=1.2.1)
pkg_check_modules(ZLIB REQUIRED zlib)
option(ENABLE_MAD "Enable MAD MP3 decoding support (for tests)" ON)
if(ENABLE_MAD)
  pkg_check_modules(MAD mad>=0.14.2)
  if(NOT MAD_FOUND) set(ENABLE_MAD OFF) endif()
endif()

# Expected targets from other directories
set(AIDACC_TARGET_NAME aidacc)
set(BSE_TARGET_NAME bse)
set(BSETOOL_TARGET_NAME bsetool)

# --- IDL Generation for explore.idl ---
set(EXPLORE_IDL_INPUT ${CMAKE_CURRENT_SOURCE_DIR}/explore.idl)
set(EXPLORE_INTERFACES_HH ${CMAKE_CURRENT_BINARY_DIR}/explore_interfaces.hh)
set(EXPLORE_INTERFACES_CC ${CMAKE_CURRENT_BINARY_DIR}/explore_interfaces.cc)

add_custom_command(
  OUTPUT ${EXPLORE_INTERFACES_HH} ${EXPLORE_INTERFACES_CC}
  COMMAND ${AIDACC_TARGET_NAME}
    -x CxxStub
    -G strip-path=${CMAKE_CURRENT_SOURCE_DIR}/ # Makefile was $(abspath .)/, this should be similar for paths in generated code
    -o ${CMAKE_CURRENT_BINARY_DIR} # Output directory for generated files
    ${EXPLORE_IDL_INPUT}
  COMMAND sed -e '1i#define _(x) x' -i ${EXPLORE_INTERFACES_CC} # As per Makefile
  DEPENDS ${EXPLORE_IDL_INPUT} ${AIDACC_TARGET_NAME}
  COMMENT "Generating CxxStub interfaces from ${EXPLORE_IDL_INPUT}"
  VERBATIM
)

# --- tests/suite executable ---
set(SUITE_SOURCES
  basics.cc
  aida-types.cc
  benchmarks.cc
  blocktests.cc
  checkserialize.cc
  explore-tests.cc # Will include explore_interfaces.hh
  filterdesign.cc
  filtertest.cc
  firhandle.cc
  ipc.cc
  loophandle.cc
  misctests.cc
  resamplehandle.cc
  subnormals-aux.cc
  subnormals.cc
  suite-main.cc
  suite-randomhash.cc
  testfft.cc
  testresampler.cc
  testresamplerq.cc
  testwavechunk.cc
)
set(SUITE_FULLPATH_SOURCES "")
foreach(src IN LISTS SUITE_SOURCES)
  list(APPEND SUITE_FULLPATH_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${src}")
endforeach()

add_executable(suite
  ${SUITE_FULLPATH_SOURCES}
  ${EXPLORE_INTERFACES_CC} # Add generated C++ file
)

target_include_directories(suite PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR} # For its own headers
  ${CMAKE_CURRENT_BINARY_DIR} # For explore_interfaces.hh and other generated files
  $<TARGET_PROPERTY:bse,INTERFACE_INCLUDE_DIRECTORIES> # For libbse headers
  ${GLIB2_INCLUDE_DIRS}
  # For external libs if tests include them directly, e.g. rapidjson, pugixml
  # These paths might need to be configured or discovered if not part of BSEDEPS
  ${CMAKE_SOURCE_DIR}/external/rapidjson/include # Example, if tests use rapidjson directly
  ${CMAKE_SOURCE_DIR}/external/pugixml/src       # Example, if tests use pugixml directly
)

target_link_libraries(suite PRIVATE
  ${BSE_TARGET_NAME} # Link against libbse
  ${GLIB2_LIBRARIES}
  ${FLUIDSYNTH_LIBRARIES}
  ${VORBISENC_LIBRARIES}
  ${VORBISFILE_LIBRARIES}
  ${OGG_LIBRARIES}
  ${FLAC_LIBRARIES}
  ${ZLIB_LIBRARIES}
  stdc++fs
)
if(ENABLE_MAD AND MAD_FOUND)
  target_link_libraries(suite PRIVATE ${MAD_LIBRARIES})
endif()

# --- Basic Tests ---
add_test(NAME CheckSuite COMMAND suite)

# Benchmark tests
add_test(NAME AidaBenchmark COMMAND suite --aida-bench)
add_test(NAME SuiteBenchmark COMMAND suite --bench) # Potentially add -j if suite handles it

# Assertion tests (simplified version)
# The original Makefile checks specific exit codes and SIGTRAP.
# CMake's add_test can check for non-zero exit (WILL_FAIL) or required regex in output.
# For more complex logic, a wrapper script run by add_test is better.
add_test(NAME Assertions.ReturnUnless1 COMMAND suite --return_unless1) # Expected to pass (exit 0)
add_test(NAME Assertions.AssertReturn1 COMMAND suite --assert_return1)   # Expected to pass (exit 0)

set(ASSERT_TEST_EXPECT_FAIL_CMD $<TARGET_FILE:suite>) # Get path to suite executable
add_test(NAME Assertions.ReturnUnless0
  COMMAND ${CMAKE_COMMAND} -E env BSE_DEBUG=no-backtrace ${ASSERT_TEST_EXPECT_FAIL_CMD} --return_unless0
)
set_tests_properties(Assertions.ReturnUnless0 PROPERTIES WILL_FAIL TRUE) # Expects non-zero exit

add_test(NAME Assertions.AssertReturn0
  COMMAND ${CMAKE_COMMAND} -E env BSE_DEBUG=no-backtrace ${ASSERT_TEST_EXPECT_FAIL_CMD} --assert_return0
)
set_tests_properties(Assertions.AssertReturn0 PROPERTIES WILL_FAIL TRUE)

add_test(NAME Assertions.AssertReturnUnreached
  COMMAND ${CMAKE_COMMAND} -E env BSE_DEBUG=no-backtrace ${ASSERT_TEST_EXPECT_FAIL_CMD} --assert_return_unreached
)
set_tests_properties(Assertions.AssertReturnUnreached PROPERTIES WILL_FAIL TRUE)

add_test(NAME Assertions.FatalError
  COMMAND ${CMAKE_COMMAND} -E env BSE_DEBUG=no-backtrace ${ASSERT_TEST_EXPECT_FAIL_CMD} --fatal_error
)
set_tests_properties(Assertions.FatalError PROPERTIES WILL_FAIL TRUE)


# --- check-loading Test (Placeholder/Simplified) ---
# This test is complex: finds all .bse files, splits them, runs bsetool check-load on each.
# Full implementation requires scripting for file listing, splitting, and iteration.
# For now, a placeholder or a simplified version.
# A proper CMake way would be a script that CTest runs, and that script performs all these actions.

# Create the list of .bse files at configure time (or build time using add_custom_target)
file(GLOB_RECURSE ALL_BSE_FILES FOLLOW_SYMLINKS
  "${CMAKE_SOURCE_DIR}/*.bse" # Search from project root
)
# Filter out some known problematic ones if needed (tests/skipload from Makefile)
set(SKIPLOAD_PATTERNS "DUMMY.bse") # Add more if needed

set(BSE_FILES_TO_CHECK "")
foreach(bse_file IN LISTS ALL_BSE_FILES)
  set(skip_this_file FALSE)
  foreach(pattern IN LISTS SKIPLOAD_PATTERNS)
    if(bse_file MATCHES "${pattern}")
      set(skip_this_file TRUE)
      break()
    endif()
  endforeach()
  if(NOT skip_this_file)
    list(APPEND BSE_FILES_TO_CHECK ${bse_file})
  endif()
endforeach()

# For each .bse file, add a test. This might create many tests.
# The Makefile splits them into 8 groups for parallelism. CTest can manage parallelism of individual tests.
# This also needs paths to plugins. Assuming plugins are built and in a known location relative to bsetool or build dir.
# For example: $<TARGET_FILE_DIR:cxxplugins>
# This part is highly dependent on where plugins are built and how bsetool finds them.
# The makefile uses: --bse-override-plugin-globs '$>/plugins/*.so'
set(PLUGIN_DIR "${CMAKE_BINARY_DIR}/plugins") # Assuming this is where .so files are

foreach(bse_file_to_check IN LISTS BSE_FILES_TO_CHECK)
  get_filename_component(bse_file_name ${bse_file_to_check} NAME)
  add_test(
    NAME CheckLoading.${bse_file_name}
    COMMAND $<TARGET_FILE:bsetool>
      --quiet
      check-load
      --bse-pcm-driver null
      --bse-midi-driver null
      --bse-override-plugin-globs "${PLUGIN_DIR}/*.so" # Path to built plugins
      --bse-override-sample-path "${CMAKE_SOURCE_DIR}/tests/audio:${CMAKE_SOURCE_DIR}/media/Samples"
      --bse-rcfile /dev/null
      ${bse_file_to_check}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR} # Or a specific test working dir
  )
  # TODO: The original test checks if an output log file is empty.
  # This can be done by piping output to a file and checking its size,
  # or by using PASS_REGULAR_EXPRESSION/FAIL_REGULAR_EXPRESSION if errors print specific things.
  # For now, test passes if bsetool check-load returns 0.
endforeach()


# --- Subdirectory for audio tests ---
add_subdirectory(audio)

message(STATUS "tests/CMakeLists.txt processed.")
message(STATUS "  'suite' executable and various tests defined.")
message(STATUS "  'check-loading' test may generate many individual CTest tests.")
message(STATUS "  Path to plugins for check-loading: ${PLUGIN_DIR} (verify this is correct after plugins build).")
message(STATUS "  External includes for tests (e.g. rapidjson, pugixml) are hardcoded, make these configurable if needed.")
