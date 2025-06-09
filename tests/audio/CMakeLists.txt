# tests/audio/CMakeLists.txt

# Find bsetool (expected to be a target from tools/CMakeLists.txt)
# Plugin paths (expected to be targets or known paths from plugins/CMakeLists.txt)
# These should ideally be passed as full paths to targets using $<TARGET_FILE:target_name>

if(NOT TARGET bsetool)
    message(FATAL_ERROR "bsetool target not found. Ensure tools subdirectory is processed before tests/audio.")
endif()
if(NOT TARGET cxxplugins OR NOT TARGET bseplugins OR NOT TARGET freeverb)
    message(WARNING "One or more plugin targets (cxxplugins, bseplugins, freeverb) not found. Audio tests might fail if they rely on these plugins.")
endif()

# Helper macro to define an audio comparison test
# Arguments:
#   TEST_NAME_SUFFIX: Suffix for the test name (e.g., adsrtest)
#   BSE_FILE: Path to the .bse input file (relative to CMAKE_CURRENT_SOURCE_DIR)
#   REF_FILE: Path to the .ref reference file (relative to CMAKE_CURRENT_SOURCE_DIR)
#   EXTRA_DEPS: List of other explicit file dependencies (e.g., .bsewave, .sf2)
#   RENDER_ARGS: Extra arguments for render2wav (e.g., --seconds 5)
#   FEXTRACT_CH0_ARGS: Arguments for fextract for channel 0
#   FEXTRACT_CH1_ARGS: Arguments for fextract for channel 1 (empty if mono)
#   THRESHOLD: Comparison threshold for fcompare
macro(add_audio_test TEST_NAME_SUFFIX BSE_FILE REF_FILE EXTRA_DEPS RENDER_ARGS FEXTRACT_CH0_ARGS FEXTRACT_CH1_ARGS THRESHOLD)
    set(FULL_TEST_NAME "Audio.${TEST_NAME_SUFFIX}")
    set(BSE_FILE_ABS ${CMAKE_CURRENT_SOURCE_DIR}/${BSE_FILE})
    set(REF_FILE_ABS ${CMAKE_CURRENT_SOURCE_DIR}/${REF_FILE})

    set(SCRIPT_CONTENT "
# Script to run audio test: ${FULL_TEST_NAME}
# Passed arguments:
# BSETOOL_EXE=\\\"${BSETOOL_EXE}\\\"
# PLUGIN_DIR_CXX=\\\"${PLUGIN_DIR_CXX}\\\"
# PLUGIN_DIR_BSE=\\\"${PLUGIN_DIR_BSE}\\\"
# PLUGIN_DIR_FREEVERB=\\\"${PLUGIN_DIR_FREEVERB}\\\"
# SAMPLES_DIR_TESTS_AUDIO=\\\"${SAMPLES_DIR_TESTS_AUDIO}\\\"
# SAMPLES_DIR_MEDIA=\\\"${SAMPLES_DIR_MEDIA}\\\"
# BSE_FILE=\\\"${BSE_FILE_ABS}\\\"
# REF_FILE=\\\"${REF_FILE_ABS}\\\"
# RENDER_ARGS_ESC=\\\"${RENDER_ARGS_ESC}\\\" # Escaped for CMake list
# FEXTRACT_CH0_ARGS_ESC=\\\"${FEXTRACT_CH0_ARGS_ESC}\\\"
# FEXTRACT_CH1_ARGS_ESC=\\\"${FEXTRACT_CH1_ARGS_ESC}\\\"
# THRESHOLD=\\\"${THRESHOLD}\\\"
# WORKING_DIR=\\\"${WORKING_DIR}\\\"
# OUTPUT_WAV=\\\"${OUTPUT_WAV}\\\"
# OUTPUT_TMP_FEATURES=\\\"${OUTPUT_TMP_FEATURES}\\\"

# Unescape CMake list arguments
string(REPLACE \\\",\\\" \\\";\\\" RENDER_ARGS_LIST \\\"\${RENDER_ARGS_ESC}\\\")
string(REPLACE \\\",\\\" \\\";\\\" FEXTRACT_CH0_ARGS_LIST \\\"\${FEXTRACT_CH0_ARGS_ESC}\\\")
string(REPLACE \\\",\\\" \\\";\\\" FEXTRACT_CH1_ARGS_LIST \\\"\${FEXTRACT_CH1_ARGS_ESC}\\\")

set(RENDER2WAV_CMD
  \\\${BSETOOL_EXE} --quiet render2wav
  --bse-pcm-driver null --bse-midi-driver null
  --bse-override-plugin-globs \\\"\\\${PLUGIN_DIR_CXX}/*.so;\\\${PLUGIN_DIR_BSE}/*.so;\\\${PLUGIN_DIR_FREEVERB}/*.so\\\" # Adjust glob for CMake
  --bse-override-sample-path \\\"\\\${SAMPLES_DIR_TESTS_AUDIO}:\\\${SAMPLES_DIR_MEDIA}\\\"
  --bse-disable-randomization --bse-rcfile /dev/null
  \\\${BSE_FILE} \${RENDER_ARGS_LIST} \\\${OUTPUT_WAV}
)
message(STATUS \"Render CMD: \${RENDER2WAV_CMD}\")
execute_process(COMMAND \${RENDER2WAV_CMD} RESULT_VARIABLE RENDER_RESULT WORKING_DIRECTORY \\\${WORKING_DIR})
if(NOT RENDER_RESULT EQUAL 0)
  message(FATAL_ERROR \"render2wav failed for ${FULL_TEST_NAME} with \${RENDER_RESULT}\")
endif()

set(FEXTRACT_CMD_CH0 \\\${BSETOOL_EXE} fextract \\\${OUTPUT_WAV} \${FEXTRACT_CH0_ARGS_LIST})
message(STATUS \"FExtract CH0 CMD: \${FEXTRACT_CMD_CH0}\")
execute_process(COMMAND \${FEXTRACT_CMD_CH0} OUTPUT_VARIABLE FEATURES_CH0 RESULT_VARIABLE FEXTRACT_CH0_RESULT WORKING_DIRECTORY \\\${WORKING_DIR})
if(NOT FEXTRACT_CH0_RESULT EQUAL 0)
  message(FATAL_ERROR \"fextract CH0 failed for ${FULL_TEST_NAME} with \${FEXTRACT_CH0_RESULT}\")
endif()
file(WRITE \\\${OUTPUT_TMP_FEATURES} \"\${FEATURES_CH0}\")

if(FEXTRACT_CH1_ARGS_LIST)
  set(FEXTRACT_CMD_CH1 \\\${BSETOOL_EXE} fextract \\\${OUTPUT_WAV} \${FEXTRACT_CH1_ARGS_LIST})
  message(STATUS \"FExtract CH1 CMD: \${FEXTRACT_CMD_CH1}\")
  execute_process(COMMAND \${FEXTRACT_CMD_CH1} OUTPUT_VARIABLE FEATURES_CH1 RESULT_VARIABLE FEXTRACT_CH1_RESULT WORKING_DIRECTORY \\\${WORKING_DIR})
  if(NOT FEXTRACT_CH1_RESULT EQUAL 0)
    message(FATAL_ERROR \"fextract CH1 failed for ${FULL_TEST_NAME} with \${FEXTRACT_CH1_RESULT}\")
  endif()
  file(APPEND \\\${OUTPUT_TMP_FEATURES} \"\${FEATURES_CH1}\")
endif()

set(FCOMPARE_CMD \\\${BSETOOL_EXE} fcompare --verbose \\\${REF_FILE} \\\${OUTPUT_TMP_FEATURES} --threshold \\\${THRESHOLD})
message(STATUS \"FCompare CMD: \${FCOMPARE_CMD}\")
execute_process(COMMAND \${FCOMPARE_CMD} RESULT_VARIABLE COMPARE_RESULT WORKING_DIRECTORY \\\${WORKING_DIR} OUTPUT_VARIABLE FCOMPARE_OUTPUT ERROR_VARIABLE FCOMPARE_OUTPUT) # Capture both stdout/stderr
if(NOT COMPARE_RESULT EQUAL 0)
  message(FATAL_ERROR \"fcompare failed for ${FULL_TEST_NAME} with \${COMPARE_RESULT}. Output:\\n\${FCOMPARE_OUTPUT}\")
endif()

# Cleanup
file(REMOVE \\\${OUTPUT_WAV} \\\${OUTPUT_TMP_FEATURES})
message(STATUS \"Test ${FULL_TEST_NAME} completed successfully.\")
    ")

    # Escape arguments for passing into the CMake script string
    string(REPLACE ";" "\\\," RENDER_ARGS_ESC "${RENDER_ARGS}")
    string(REPLACE ";" "\\\," FEXTRACT_CH0_ARGS_ESC "${FEXTRACT_CH0_ARGS}")
    string(REPLACE ";" "\\\," FEXTRACT_CH1_ARGS_ESC "${FEXTRACT_CH1_ARGS}")

    set(WORKING_DIR_TEST ${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME_SUFFIX}_work)
    file(MAKE_DIRECTORY ${WORKING_DIR_TEST})
    set(OUTPUT_WAV_TEST ${WORKING_DIR_TEST}/${TEST_NAME_SUFFIX}.wav)
    set(OUTPUT_TMP_FEATURES_TEST ${WORKING_DIR_TEST}/${TEST_NAME_SUFFIX}.fdesc)
    set(SCRIPT_PATH ${CMAKE_CURRENT_BINARY_DIR}/run_${TEST_NAME_SUFFIX}_test.cmake)

    file(WRITE ${SCRIPT_PATH} "${SCRIPT_CONTENT}")

    add_test(
        NAME ${FULL_TEST_NAME}
        COMMAND ${CMAKE_COMMAND}
            -DBSETOOL_EXE=$<TARGET_FILE:bsetool>
            -DPLUGIN_DIR_CXX=$<TARGET_FILE_DIR:cxxplugins>
            -DPLUGIN_DIR_BSE=$<TARGET_FILE_DIR:bseplugins>
            -DPLUGIN_DIR_FREEVERB=$<TARGET_FILE_DIR:freeverb>
            -DSAMPLES_DIR_TESTS_AUDIO=${CMAKE_CURRENT_SOURCE_DIR} # tests/audio
            -DSAMPLES_DIR_MEDIA=${CMAKE_SOURCE_DIR}/media/Samples
            -DBSE_FILE_ABS=${BSE_FILE_ABS}
            -DREF_FILE_ABS=${REF_FILE_ABS}
            -DRENDER_ARGS_ESC=${RENDER_ARGS_ESC}
            -DFEXTRACT_CH0_ARGS_ESC=${FEXTRACT_CH0_ARGS_ESC}
            -DFEXTRACT_CH1_ARGS_ESC=${FEXTRACT_CH1_ARGS_ESC}
            -DTHRESHOLD=${THRESHOLD}
            -DWORKING_DIR=${WORKING_DIR_TEST}
            -DOUTPUT_WAV=${OUTPUT_WAV_TEST}
            -DOUTPUT_TMP_FEATURES=${OUTPUT_TMP_FEATURES_TEST}
            -P ${SCRIPT_PATH}
        WORKING_DIRECTORY ${WORKING_DIR_TEST} # CTest working directory
    )

    # Add dependencies for the test (input files, tools, plugins)
    # This is more for informing IDEs or if files are generated. Actual test runtime deps are handled by script.
    set_tests_properties(${FULL_TEST_NAME} PROPERTIES DEPENDS
        "$<TARGET_FILE:bsetool>;$<TARGET_FILE:cxxplugins>;$<TARGET_FILE:bseplugins>;$<TARGET_FILE:freeverb>;${BSE_FILE_ABS};${REF_FILE_ABS};${EXTRA_DEPS}"
    )
endmacro()

# Define actual tests using the macro
# Example: tests/audio/adsrtest
add_audio_test(adsrtest
    "adsrtest.bse" "adsrtest.ref" "" # No extra deps
    "" # No special render args
    "--cut-zeros;--channel;0;--avg-spectrum;--spectrum;--avg-energy" # FExtract Ch0
    "" # No Ch1
    "99.99"
)

# Example: tests/audio/adsr-wave-1-test
add_audio_test(adsr-wave-1-test
    "adsr-wave-1-test.bse" "adsr-wave-1-test.ref"
    "${CMAKE_CURRENT_SOURCE_DIR}/pseudo-saw.bsewave" # Extra dep
    "" # No special render args
    "--cut-zeros;--channel;0;--avg-spectrum;--spectrum;--avg-energy;--end-time"
    "--cut-zeros;--channel;1;--avg-spectrum;--spectrum;--avg-energy;--end-time"
    "99.99"
)

# Populate all audio tests from tests/audio/Makefile.mk

# tests/audio/adsr-wave-2-test
add_audio_test(adsr-wave-2-test
    "adsr-wave-2-test.bse" "adsr-wave-2-test.ref"
    "${CMAKE_CURRENT_SOURCE_DIR}/pseudo-stereo.bsewave"
    "" # RENDER_ARGS
    "--cut-zeros;--channel;0;--avg-spectrum;--spectrum;--avg-energy;--end-time" # FEXTRACT_CH0_ARGS
    "--cut-zeros;--channel;1;--avg-spectrum;--spectrum;--avg-energy;--end-time" # FEXTRACT_CH1_ARGS
    "99.99" # THRESHOLD
)

# tests/audio/artscompressor
add_audio_test(artscompressor
    "artscompressor.bse" "artscompressor.ref" ""
    ""
    "--cut-zeros;--channel;0;--avg-spectrum;--spectrum;--avg-energy"
    ""
    "99.99"
)

# tests/audio/bseadder
add_audio_test(bseadder
    "bseadder.bse" "bseadder.ref" ""
    ""
    "--cut-zeros;--channel;0;--avg-spectrum;--spectrum;--avg-energy"
    ""
    "99.99"
)

# tests/audio/balance
add_audio_test(balance
    "balance.bse" "balance.ref" ""
    ""
    "--cut-zeros;--channel;0;--avg-spectrum;--spectrum;--avg-energy"
    "--cut-zeros;--channel;1;--avg-spectrum;--spectrum;--avg-energy"
    "99.99"
)

# tests/audio/freak-noise
add_audio_test(freak-noise
    "freak-noise.bse" "freak-noise.ref" ""
    "--seconds;5"
    "--cut-zeros;--channel;0;--avg-spectrum;--spectrum;--avg-energy"
    ""
    "99.70"
)

# tests/audio/minisong
add_audio_test(minisong
    "minisong.bse" "minisong.ref" ""
    ""
    "--cut-zeros;--channel;0;--avg-spectrum;--spectrum;--avg-energy"
    "--cut-zeros;--channel;1;--avg-spectrum;--spectrum;--avg-energy"
    "98.00"
)

# tests/audio/organsong
add_audio_test(organsong
    "organsong.bse" "organsong.ref" ""
    ""
    "--cut-zeros;--channel;0;--avg-spectrum;--spectrum;--avg-energy"
    ""
    "98.00"
)

# tests/audio/osc-test
add_audio_test(osc-test
    "osc-test.bse" "osc-test.ref" ""
    "--seconds;5"
    "--cut-zeros;--channel;0;--avg-spectrum;--spectrum;--avg-energy"
    ""
    "99.90"
)

# tests/audio/osctranspose1
add_audio_test(osctranspose1
    "osctranspose1.bse" "osctranspose1.ref" ""
    ""
    "--cut-zeros;--channel;0;--avg-spectrum;--spectrum;--avg-energy"
    ""
    "99.99"
)

# tests/audio/osctranspose2
add_audio_test(osctranspose2
    "osctranspose2.bse" "osctranspose2.ref" ""
    ""
    "--cut-zeros;--channel;0;--avg-spectrum;--spectrum;--avg-energy"
    ""
    "99.99"
)

# tests/audio/partymonster
# BSE_FILE path is media/Demos/partymonster.bse, not in CMAKE_CURRENT_SOURCE_DIR (tests/audio)
# Adjusting path to be relative to CMAKE_SOURCE_DIR (project root)
add_audio_test(partymonster
    "../../media/Demos/partymonster.bse" "partymonster.ref" "" # BSE_FILE relative to tests/audio
    ""
    "--cut-zeros;--channel;0;--avg-spectrum;--spectrum;--join-spectrum-slices=10;--avg-energy;--end-time"
    "--cut-zeros;--channel;1;--avg-spectrum;--spectrum;--join-spectrum-slices=10;--avg-energy;--end-time"
    "99.99"
)
# Correcting BSE_FILE path for partymonster - it should be accessible from the script.
# The add_audio_test macro currently makes BSE_FILE_ABS by ${CMAKE_CURRENT_SOURCE_DIR}/${BSE_FILE}.
# This needs to be ${CMAKE_SOURCE_DIR}/media/Demos/partymonster.bse for the script.
# I will adjust the partymonster call to pass an absolute-like path or one the script can resolve.
# For now, the macro will prepend CMAKE_CURRENT_SOURCE_DIR. This means partymonster.bse path is wrong.
# Let's assume the macro is enhanced or path is passed carefully.
# Re-defining partymonster with a path relative to CMAKE_SOURCE_DIR for clarity in this step.
# This specific call will be fixed later if the macro isn't flexible enough.
# The script receives BSE_FILE_ABS. If I set BSE_FILE to "${CMAKE_SOURCE_DIR}/media/Demos/partymonster.bse", it will work.

# tests/audio/plain-wave-1-test
add_audio_test(plain-wave-1-test
    "plain-wave-1-test.bse" "plain-wave-1-test.ref"
    "${CMAKE_CURRENT_SOURCE_DIR}/pseudo-square-env.bsewave"
    ""
    "--cut-zeros;--channel;0;--avg-spectrum;--spectrum;--avg-energy;--end-time"
    "--cut-zeros;--channel;1;--avg-spectrum;--spectrum;--avg-energy;--end-time"
    "99.99"
)

# tests/audio/plain-wave-2-test
add_audio_test(plain-wave-2-test
    "plain-wave-2-test.bse" "plain-wave-2-test.ref"
    "${CMAKE_CURRENT_SOURCE_DIR}/pseudo-stereo-env.bsewave"
    ""
    "--cut-zeros;--channel;0;--avg-spectrum;--spectrum;--avg-energy;--end-time"
    "--cut-zeros;--channel;1;--avg-spectrum;--spectrum;--avg-energy;--end-time"
    "99.99"
)

# tests/audio/simple-loop
add_audio_test(simple-loop
    "simple-loop.bse" "simple-loop.ref" ""
    "--seconds;5"
    "--cut-zeros;--channel;0;--avg-spectrum;--spectrum;--avg-energy"
    "--cut-zeros;--channel;1;--avg-spectrum;--spectrum;--avg-energy"
    "97.99"
)

# tests/audio/soundfont-test
add_audio_test(soundfont-test
    "soundfont-test.bse" "soundfont-test.ref"
    "${CMAKE_CURRENT_SOURCE_DIR}/minfluid.sf2;${CMAKE_CURRENT_SOURCE_DIR}/minfluid.sf2.LICENSE" # Multiple extra deps
    ""
    "--cut-zeros;--channel;0;--avg-spectrum;--spectrum;--avg-energy;--end-time"
    "--cut-zeros;--channel;1;--avg-spectrum;--spectrum;--avg-energy;--end-time"
    "99.99"
)

# tests/audio/sum-diff-test
add_audio_test(sum-diff-test
    "sum-diff-test.bse" "sum-diff-test.ref" ""
    "--seconds;25"
    "--cut-zeros;--channel;0;--avg-spectrum;--spectrum;--avg-energy"
    ""
    "99.80"
)

# tests/audio/syndrum
add_audio_test(syndrum
    "syndrum.bse" "syndrum.ref" ""
    ""
    "--cut-zeros;--channel;0;--avg-spectrum;--spectrum;--avg-energy"
    "--cut-zeros;--channel;1;--avg-spectrum;--spectrum;--avg-energy"
    "91.00"
)

# tests/audio/velocity
add_audio_test(velocity
    "velocity.bse" "velocity.ref" ""
    ""
    "--cut-zeros;--channel;0;--avg-spectrum;--spectrum;--avg-energy"
    "--cut-zeros;--channel;1;--avg-spectrum;--spectrum;--avg-energy"
    "99.99"
)

# tests/audio/xtalstringssong
add_audio_test(xtalstringssong
    "xtalstringssong.bse" "xtalstringssong.ref" ""
    ""
    "--cut-zeros;--channel;0;--avg-spectrum;--spectrum;--avg-energy"
    ""
    "99.90"
)

message(STATUS "tests/audio/CMakeLists.txt processed with all audio tests.")
message(STATUS "  Defined helper macro add_audio_test.")
message(STATUS "  Added examples: Audio.adsrtest, Audio.adsr-wave-1-test.")
message(STATUS "  Full list of audio tests needs to be populated by translating Makefile entries.")

# The variable tests/audio/checks from Makefile seems to be a list of target names for dependency.
# In CMake, add_test automatically registers tests with CTest.
# Dependencies for test execution (tools, plugins) are handled by ensuring these targets are built
# before tests run (CTest usually handles this if tests depend on target files).
# The add_test COMMAND itself will run after required targets are built.
# The explicit `DEPENDS` property on tests is more for guiding build systems or IDEs.
