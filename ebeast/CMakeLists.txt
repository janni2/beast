# ebeast/CMakeLists.txt

# --- Find Prerequisites ---
find_program(NPM_EXECUTABLE npm REQUIRED)
find_program(NODE_EXECUTABLE node) # Optional, if scripts need to be run directly with node
find_program(IMAGEMAGICK_CONVERT_EXECUTABLE convert) # For image manipulations
find_program(PANDOC_EXECUTABLE pandoc) # For Vue docs generation (later step)

# BeastSoundEngine executable is needed for bseapi_jsonipc.js generation
# Assumed to be an available target: BeastSoundEngine
if(NOT TARGET BeastSoundEngine)
    message(FATAL_ERROR "ebeast/CMakeLists.txt requires the BeastSoundEngine target to be defined.")
endif()

# Output directories in the build tree
set(EBEAST_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(APP_BUILD_DIR ${EBEAST_BUILD_DIR}/app)
set(ELECTRON_BUILD_DIR ${EBEAST_BUILD_DIR}/electron)
set(EBEAST_ASSETS_DIR ${APP_BUILD_DIR}/assets)
set(EBEAST_B_DIR ${APP_BUILD_DIR}/b) # For Vue components output

file(MAKE_DIRECTORY ${APP_BUILD_DIR} ${ELECTRON_BUILD_DIR} ${EBEAST_ASSETS_DIR} ${EBEAST_B_DIR})

# --- NPM Installation ---
set(NPM_DONE_STAMP ${EBEAST_BUILD_DIR}/node_modules/npm.done)
set(PACKAGE_JSON_IN ${CMAKE_CURRENT_SOURCE_DIR}/package.json.in)
set(PACKAGE_JSON_OUT ${EBEAST_BUILD_DIR}/package.json)
set(FIX_SCSS_DIFF ${CMAKE_CURRENT_SOURCE_DIR}/fix-scss.diff)

# 1. Copy package.json.in to package.json in build dir
add_custom_command(
  OUTPUT ${PACKAGE_JSON_OUT}
  COMMAND ${CMAKE_COMMAND} -E copy ${PACKAGE_JSON_IN} ${PACKAGE_JSON_OUT}
  DEPENDS ${PACKAGE_JSON_IN}
  COMMENT "Copying package.json.in to build directory"
)

# 2. Run npm install and patch
# The find/sed command to anonymize paths is complex to replicate directly and portably.
# For now, skipping that specific sed command. If it's critical, it might need a helper script.
add_custom_command(
  OUTPUT ${NPM_DONE_STAMP}
  COMMAND ${NPM_EXECUTABLE} --prefer-offline install # TODO: Add --progress=false if PARALLEL_MAKE equivalent is set
  COMMAND patch -p0 < ${FIX_SCSS_DIFF} # Apply patch
  WORKING_DIRECTORY ${EBEAST_BUILD_DIR}
  DEPENDS ${PACKAGE_JSON_OUT} ${FIX_SCSS_DIFF}
  COMMENT "Running npm install and applying patches in ${EBEAST_BUILD_DIR}"
  VERBATIM # Important for commands like patch that might have shell specific syntax
)
# Node modules bin directory
set(NODE_MODULES_BIN_DIR ${EBEAST_BUILD_DIR}/node_modules/.bin)

# --- Copy essential JS files (pre-rollup) ---
set(EBEAST_COPY_TOOL_TARGETS "") # For files copied to EBEAST_BUILD_DIR
list(APPEND EBEAST_COPY_TOOL_TARGETS "${EBEAST_BUILD_DIR}/chromatic-sass2.js")
list(APPEND EBEAST_COPY_TOOL_TARGETS "${EBEAST_BUILD_DIR}/vue.rollup.js")
list(APPEND EBEAST_COPY_TOOL_TARGETS "${EBEAST_BUILD_DIR}/markdown-it.esm0.js")
list(APPEND EBEAST_COPY_TOOL_TARGETS "${EBEAST_BUILD_DIR}/markdown-it.rollup.js")
list(APPEND EBEAST_COPY_TOOL_TARGETS "${EBEAST_BUILD_DIR}/.eslintrc.js")

foreach(target_path IN LISTS EBEAST_COPY_TOOL_TARGETS)
    get_filename_component(filename ${target_path} NAME)
    set(source_path "${CMAKE_CURRENT_SOURCE_DIR}/${filename}")
    add_custom_command(
        OUTPUT ${target_path}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${source_path} ${target_path}
        DEPENDS ${source_path}
        COMMENT "Copying ${filename} to ${EBEAST_BUILD_DIR}"
    )
endforeach()

set(APP_COPY_CONTENT_TARGETS "") # For files copied to APP_BUILD_DIR
list(APPEND APP_COPY_CONTENT_TARGETS "${APP_BUILD_DIR}/main.js")
list(APPEND APP_COPY_CONTENT_TARGETS "${APP_BUILD_DIR}/jsbse.js")
list(APPEND APP_COPY_CONTENT_TARGETS "${APP_BUILD_DIR}/util.js")
# ebeast/b/js.inputs -> copy to app/b/
file(GLOB EBEAST_B_JS_INPUTS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/b/*.js")
foreach(js_file_rel IN LISTS EBEAST_B_JS_INPUTS) # js_file_rel is like "b/filename.js"
    set(js_file_source "${CMAKE_CURRENT_SOURCE_DIR}/${js_file_rel}")
    set(js_file_dest "${APP_BUILD_DIR}/${js_file_rel}")
    add_custom_command(
        OUTPUT ${js_file_dest}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${js_file_source} ${js_file_dest}
        DEPENDS ${js_file_source}
        COMMENT "Copying ${js_file_rel} to ${APP_BUILD_DIR}"
    )
    list(APPEND APP_COPY_CONTENT_TARGETS ${js_file_dest})
endforeach()

# --- Generate bseapi_jsonipc.js ---
set(BSEAPI_JSONIPC_JS ${APP_BUILD_DIR}/bseapi_jsonipc.js)
set(JSONIPC_HEAD_JS ${CMAKE_SOURCE_DIR}/../jsonipc/head.js) # Path from root

add_custom_command(
  OUTPUT ${BSEAPI_JSONIPC_JS}
  COMMAND ${CMAKE_COMMAND} -E touch ${BSEAPI_JSONIPC_JS}.tmp # Create temp file
  COMMAND ${CMAKE_COMMAND} -E cat ${JSONIPC_HEAD_JS} > ${BSEAPI_JSONIPC_JS}.tmp # cat head.js
  COMMAND $<TARGET_FILE:BeastSoundEngine> --js-bseapi >> ${BSEAPI_JSONIPC_JS}.tmp # Append output
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${BSEAPI_JSONIPC_JS}.tmp ${BSEAPI_JSONIPC_JS}
  COMMAND ${CMAKE_COMMAND} -E remove ${BSEAPI_JSONIPC_JS}.tmp
  DEPENDS ${JSONIPC_HEAD_JS} BeastSoundEngine # Depends on head.js and the BeastSoundEngine executable
  COMMENT "Generating ${BSEAPI_JSONIPC_JS}"
  VERBATIM
)

# --- Rollup for Vue Components ---
# ebeast/targets.vuebundles.mjs ::= $(patsubst %, $>/app/b/%.mjs, $(ebeast/b/vue.stems))
# ebeast/targets.vuebundles.css ::= $(patsubst %, $>/app/b/%.css, $(ebeast/b/vue.stems))
# $>/app/b/%.mjs: ebeast/b/%.vue $(ebeast/copy.tool.targets) | $>/app/b/ $>/ebeast/b/ $>/ebeast/node_modules/npm.done
#   cd $>/ebeast/ && $(abspath $(NODE_MODULES.bin)/rollup) -c ./vue.rollup.js -i $(abspath $<) -o $(abspath $>/ebeast/b/$(@F))
#   mv $>/ebeast/b/$(@F).css $(@:.mjs=.css)
#   mv $>/ebeast/b/$(@F) $(@D)/

file(GLOB VUE_SOURCE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/b/*.vue")
set(VUE_GENERATED_MJS_FILES "")
set(VUE_GENERATED_CSS_FILES "")

foreach(vue_rel_path IN LISTS VUE_SOURCE_FILES) # e.g., b/app.vue
  get_filename_component(vue_filename ${vue_rel_path} NAME) # e.g., app.vue
  get_filename_component(vue_stem ${vue_rel_path} NAME_WE) # e.g., app
  set(vue_abs_path ${CMAKE_CURRENT_SOURCE_DIR}/${vue_rel_path})

  set(output_mjs_temp ${EBEAST_BUILD_DIR}/b/${vue_filename}) # Output to ebeast/b/app.vue (as per make)
  set(output_css_final ${APP_BUILD_DIR}/b/${vue_stem}.css)
  set(output_mjs_final ${APP_BUILD_DIR}/b/${vue_stem}.mjs) # Final location after move

  add_custom_command(
    OUTPUT ${output_mjs_final} ${output_css_final} # Define both as outputs
    COMMAND ${NODE_EXECUTABLE} ${NODE_MODULES_BIN_DIR}/rollup -c ${EBEAST_BUILD_DIR}/vue.rollup.js -i ${vue_abs_path} -o ${output_mjs_temp}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EBEAST_BUILD_DIR}/b/${vue_stem}.css ${output_css_final} # mv css
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${output_mjs_temp} ${output_mjs_final} # mv mjs
    COMMAND ${CMAKE_COMMAND} -E remove ${EBEAST_BUILD_DIR}/b/${vue_stem}.css ${output_mjs_temp} # remove temp files
    WORKING_DIRECTORY ${EBEAST_BUILD_DIR} # Rollup is run from $>/ebeast/
    DEPENDS ${vue_abs_path} ${EBEAST_BUILD_DIR}/vue.rollup.js ${NPM_DONE_STAMP}
            ${EBEAST_COPY_TOOL_TARGETS} # vue.rollup.js might depend on these
    COMMENT "Rollup Vue: ${vue_filename} -> ${vue_stem}.mjs/css"
    VERBATIM
  )
  list(APPEND VUE_GENERATED_MJS_FILES ${output_mjs_final})
  list(APPEND VUE_GENERATED_CSS_FILES ${output_css_final})
endforeach()

# --- Rollup for markdown-it ---
set(MARKDOWN_IT_ESM0_JS ${EBEAST_BUILD_DIR}/markdown-it.esm0.js) # Copied earlier
set(MARKDOWN_IT_ROLLUP_JS ${EBEAST_BUILD_DIR}/markdown-it.rollup.js) # Copied earlier
set(MARKDOWN_IT_ESM1_TEMP ${EBEAST_BUILD_DIR}/markdown-it.esm1.js)
set(MARKDOWN_IT_MJS_FINAL ${APP_BUILD_DIR}/markdown-it.mjs)

add_custom_command(
  OUTPUT ${MARKDOWN_IT_MJS_FINAL}
  COMMAND ${NODE_EXECUTABLE} ${NODE_MODULES_BIN_DIR}/rollup --format=es -c ${MARKDOWN_IT_ROLLUP_JS} -i ${MARKDOWN_IT_ESM0_JS} -o ${MARKDOWN_IT_ESM1_TEMP}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${MARKDOWN_IT_ESM1_TEMP} ${MARKDOWN_IT_MJS_FINAL}
  COMMAND ${CMAKE_COMMAND} -E remove ${MARKDOWN_IT_ESM1_TEMP}
  WORKING_DIRECTORY ${EBEAST_BUILD_DIR}
  DEPENDS ${MARKDOWN_IT_ESM0_JS} ${MARKDOWN_IT_ROLLUP_JS} ${NPM_DONE_STAMP}
  COMMENT "Rollup markdown-it"
  VERBATIM
)

# --- Asset: Beastycons (Example for FetchContent) ---
include(FetchContent)
set(BEASTYCONS_TGZ_SHA256 "2daaf07f819f48116cb3880d6939c417d09c0a64a275ffc79f348371f463cdd8")
set(BEASTYCONS_TGZ_NAME "beastycons-200911.3.tgz")
set(BEASTYCONS_URL "https://github.com/tim-janik/assets/releases/download/beastycons/${BEASTYCONS_TGZ_NAME}")

FetchContent_Declare(
  beastycons_fetch
  URL ${BEASTYCONS_URL}
  URL_HASH SHA256=${BEASTYCONS_TGZ_SHA256}
  DOWNLOAD_DIR ${EBEAST_BUILD_DIR} # Download to $>/ebeast/
  SOURCE_DIR ${EBEAST_BUILD_DIR}/beastycons_src # Extract to temp source dir
  # No build command needed for simple archive extraction and copy
)
FetchContent_GetProperties(beastycons_fetch)
if(NOT beastycons_fetch_POPULATED)
  FetchContent_Populate(beastycons_fetch)
  # After populate, archive is at ${beastycons_fetch_SOURCE_DIR}/${BEASTYCONS_TGZ_NAME}
  # The Makefile extracts it to $>/ebeast/beastycons/
  # Then copies $>/ebeast/beastycons/Beastycons.css and .woff2 to $>/app/assets/
  set(BEASTYCONS_EXTRACT_DIR ${EBEAST_BUILD_DIR}/beastycons)
  add_custom_command(
    OUTPUT ${EBEAST_ASSETS_DIR}/Beastycons.css ${EBEAST_ASSETS_DIR}/Beastycons.woff2
    COMMAND ${CMAKE_COMMAND} -E tar xf ${beastycons_fetch_SOURCE_DIR}/${BEASTYCONS_TGZ_NAME} -C ${EBEAST_BUILD_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${BEASTYCONS_EXTRACT_DIR}/Beastycons.css ${EBEAST_ASSETS_DIR}/Beastycons.css
    COMMAND ${CMAKE_COMMAND} -E copy ${BEASTYCONS_EXTRACT_DIR}/Beastycons.woff2 ${EBEAST_ASSETS_DIR}/Beastycons.woff2
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${BEASTYCONS_EXTRACT_DIR} # remove temp extract dir
    DEPENDS ${beastycons_fetch_SOURCE_DIR}/${BEASTYCONS_TGZ_NAME} # Depends on the downloaded file
    COMMENT "Extracting Beastycons and copying to app/assets"
    VERBATIM
  )
else()
  # Ensure target files are declared even if populate step is skipped, so dependencies work
  # This is tricky if the add_custom_command isn't run.
  # Better to make FetchContent part of a target that generates these files.
  # For now, this might mean files are not updated if already populated but outputs are missing.
  # A stamp file for the copy would be more robust.
endif()
set(BEASTYCONS_CSS ${EBEAST_ASSETS_DIR}/Beastycons.css) # For dependency tracking

# --- Generate index-mjs.list ---
set(INDEX_MJS_LIST_FILE ${EBEAST_BUILD_DIR}/index-mjs.list)
# Depends on VUE_GENERATED_MJS_FILES and VUE_GENERATED_CSS_FILES being populated
# This command should run after all Vue components are processed by Rollup.
# For now, use VUE_SOURCE_FILES to determine stems, assuming outputs will match.
set(INDEX_MJS_LIST_CONTENT "")
foreach(vue_rel_path IN LISTS VUE_SOURCE_FILES)
  get_filename_component(vue_stem ${vue_rel_path} NAME_WE) # e.g., app from b/app.vue
  set(INDEX_MJS_LIST_CONTENT "${INDEX_MJS_LIST_CONTENT}    <link rel=\\\"stylesheet\\\"    href=\\\"/b/${vue_stem}.css\\\">\n")
  set(INDEX_MJS_LIST_CONTENT "${INDEX_MJS_LIST_CONTENT}    <link rel=\\\"modulepreload\\\" href=\\\"/b/${vue_stem}.mjs\\\" data-autoload>\n")
endforeach()
file(GLOB EBEAST_B_JS_INPUTS_FOR_INDEXLIST RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/b/*.js")
foreach(js_file_rel IN LISTS EBEAST_B_JS_INPUTS_FOR_INDEXLIST)
  get_filename_component(js_filename ${js_file_rel} NAME) # e.g., some.js from b/some.js
  set(INDEX_MJS_LIST_CONTENT "${INDEX_MJS_LIST_CONTENT}    <link rel=\\\"modulepreload\\\" href=\\\"/b/${js_filename}\\\">\n")
endforeach()

add_custom_command(
  OUTPUT ${INDEX_MJS_LIST_FILE}
  COMMAND ${CMAKE_COMMAND} -E echo "${INDEX_MJS_LIST_CONTENT}" > ${INDEX_MJS_LIST_FILE}
  # This command depends on the list of Vue files. If that list changes, CMake re-runs.
  # It also implicitly depends on the output names of the rollup process.
  # This is generated at CMake configure time. For build-time, use a cmake -P script.
  COMMENT "Generating index-mjs.list"
  VERBATIM # Not strictly needed for echo, but good practice.
)
# --- Asset: Inter Font ---
set(INTER_FONT_SHA256 "9cd56084faa8cc5ee75bf6f3d01446892df88928731ee9321e544a682aec55ef")
set(INTER_FONT_NAME "Inter-Medium.woff2")
set(INTER_FONT_URL "https://github.com/rsms/inter/raw/v3.10/docs/font-files/${INTER_FONT_NAME}")
FetchContent_Declare(
  inter_font_fetch
  URL ${INTER_FONT_URL}
  URL_HASH SHA256=${INTER_FONT_SHA256}
  DOWNLOAD_DIR ${EBEAST_ASSETS_DIR} # Download directly to assets
  SOURCE_DIR ${EBEAST_ASSETS_DIR}/inter_font_tmp_src # Not really used for single file download
  DOWNLOAD_NO_EXTRACT TRUE # Download only, no extraction needed for a single file
)
FetchContent_GetProperties(inter_font_fetch)
if(NOT inter_font_fetch_POPULATED)
  FetchContent_Populate(inter_font_fetch)
endif()
set(INTER_FONT_WOFF2 ${EBEAST_ASSETS_DIR}/${INTER_FONT_NAME}) # Path to downloaded file

# --- Asset: Material Icons ---
set(MATERIAL_ICONS_TGZ_SHA256 "11653afda6690b5230a64d67a0057634bc5a45b9f2c9fc95916de839ba72e12f")
set(MATERIAL_ICONS_TGZ_NAME "material-icons-200821-1-h0fccaba10.tar.xz")
set(MATERIAL_ICONS_URL "https://github.com/tim-janik/assets/releases/download/material-icons/${MATERIAL_ICONS_TGZ_NAME}")
FetchContent_Declare(
  material_icons_fetch
  URL ${MATERIAL_ICONS_URL}
  URL_HASH SHA256=${MATERIAL_ICONS_TGZ_SHA256}
  DOWNLOAD_DIR ${EBEAST_BUILD_DIR}
  SOURCE_DIR ${EBEAST_BUILD_DIR}/material_icons_src # Extracted content goes here
)
FetchContent_GetProperties(material_icons_fetch)
if(NOT material_icons_fetch_POPULATED)
  FetchContent_Populate(material_icons_fetch) # Downloads and extracts
  # Copy specific files (material-icons.*) from extracted dir to assets
  # Extracted dir is ${material_icons_fetch_SOURCE_DIR}/material-icons/
  add_custom_command(
    OUTPUT ${EBEAST_ASSETS_DIR}/material-icons.css # Assuming this is one of the key files
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${material_icons_fetch_SOURCE_DIR}/material-icons ${EBEAST_ASSETS_DIR} # Copy all, then list main ones as output
    DEPENDS ${material_icons_fetch_SOURCE_DIR}/material-icons # Depends on extraction
    COMMENT "Copying Material Icons to app/assets"
  )
else()
  # Ensure target files are declared for dependency graph if already populated
  # This is complex. A stamp file for the copy is better.
endif()
set(MATERIAL_ICONS_CSS ${EBEAST_ASSETS_DIR}/material-icons.css)

# --- Asset: ForkAwesome ---
set(FORKAWESOME_WOFF2_SHA256 "844517a2bc5430242cb857e56b6dccf002f469c4c1b295ed8d0b7211fb452f50")
set(FORKAWESOME_WOFF2_NAME "forkawesome-webfont.woff2")
set(FORKAWESOME_WOFF2_URL "https://raw.githubusercontent.com/ForkAwesome/Fork-Awesome/b0605a81632452818bf19c8fa97469da1206b52b/fonts/${FORKAWESOME_WOFF2_NAME}")
set(FORKAWESOME_CSS_SHA256 "630b0e84fa43579f7e97a26fd47d4b70cb5516ca7e6e73393597d12ca249a8ee")
set(FORKAWESOME_CSS_NAME "fork-awesome.css") # Original name
set(FORKAWESOME_CSS_URL "https://raw.githubusercontent.com/ForkAwesome/Fork-Awesome/b0605a81632452818bf19c8fa97469da1206b52b/css/${FORKAWESOME_CSS_NAME}")

FetchContent_Declare(forkawesome_woff2_fetch URL ${FORKAWESOME_WOFF2_URL} URL_HASH SHA256=${FORKAWESOME_WOFF2_SHA256} DOWNLOAD_DIR ${EBEAST_ASSETS_DIR} DOWNLOAD_NO_EXTRACT TRUE)
FetchContent_Declare(forkawesome_css_fetch URL ${FORKAWESOME_CSS_URL} URL_HASH SHA256=${FORKAWESOME_CSS_SHA256} DOWNLOAD_DIR ${EBEAST_ASSETS_DIR} DOWNLOAD_NO_EXTRACT TRUE)
FetchContent_MakeAvailable(forkawesome_woff2_fetch forkawesome_css_fetch)

set(FORKAWESOME_CSS_FINAL ${EBEAST_ASSETS_DIR}/forkawesome-webfont.css) # Renamed in Makefile
add_custom_command(
  OUTPUT ${FORKAWESOME_CSS_FINAL}
  COMMAND ${CMAKE_COMMAND} -E copy ${EBEAST_ASSETS_DIR}/${FORKAWESOME_CSS_NAME} ${FORKAWESOME_CSS_FINAL}.tmp
  COMMAND sed "/^ *src: *url/s,src: *url(.*);,src: url('forkawesome-webfont.woff2');," -i ${FORKAWESOME_CSS_FINAL}.tmp # Requires sed
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${FORKAWESOME_CSS_FINAL}.tmp ${FORKAWESOME_CSS_FINAL}
  COMMAND ${CMAKE_COMMAND} -E remove ${FORKAWESOME_CSS_FINAL}.tmp
  DEPENDS ${EBEAST_ASSETS_DIR}/${FORKAWESOME_CSS_NAME} # Depends on downloaded CSS
  COMMENT "Modifying and renaming ForkAwesome CSS"
  VERBATIM
)

# --- Generate index-mjs.list (Build Time) ---
set(GENERATE_INDEX_MJS_LIST_SCRIPT ${EBEAST_BUILD_DIR}/generate_index_mjs_list.cmake)
set(INDEX_MJS_LIST_FILE ${EBEAST_BUILD_DIR}/index-mjs.list)

file(WRITE ${GENERATE_INDEX_MJS_LIST_SCRIPT}
"
file(GLOB VUE_SOURCE_FILES RELATIVE \\\"${CMAKE_CURRENT_SOURCE_DIR}\\\" \\\"${CMAKE_CURRENT_SOURCE_DIR}/b/*.vue\\\")
set(content \\\"\\\")
foreach(vue_rel_path IN LISTS VUE_SOURCE_FILES)
  get_filename_component(vue_stem \\\${vue_rel_path} NAME_WE)
  set(content \\\"\${content}    <link rel=\\\\\\\"stylesheet\\\\\\\"    href=\\\\\\\"/b/\${vue_stem}.css\\\\\\\">\n\\\")
  set(content \\\"\${content}    <link rel=\\\\\\\"modulepreload\\\\\\\" href=\\\\\\\"/b/\${vue_stem}.mjs\\\\\\\" data-autoload>\n\\\")
endforeach()
file(GLOB EBEAST_B_JS_INPUTS RELATIVE \\\"${CMAKE_CURRENT_SOURCE_DIR}\\\" \\\"${CMAKE_CURRENT_SOURCE_DIR}/b/*.js\\\")
foreach(js_file_rel IN LISTS EBEAST_B_JS_INPUTS)
  get_filename_component(js_filename \\\${js_file_rel} NAME)
  set(content \\\"\${content}    <link rel=\\\\\\\"modulepreload\\\\\\\" href=\\\\\\\"/b/\${js_filename}\\\\\\\">\n\\\")
endforeach()
file(WRITE \\\"${INDEX_MJS_LIST_FILE}\\\" \\\"\${content}\\\")
"
)
add_custom_command(
  OUTPUT ${INDEX_MJS_LIST_FILE}
  COMMAND ${CMAKE_COMMAND} -P ${GENERATE_INDEX_MJS_LIST_SCRIPT}
  DEPENDS ${VUE_SOURCE_FILES} # Depends on the list of vue files (implicitly, their existence)
          # TODO: Add dependency on actual js files in ebeast/b/ as well
  COMMENT "Generating index-mjs.list (build time)"
)

# --- Generate app/package.json ---
set(APP_PACKAGE_JSON_OUT ${APP_BUILD_DIR}/package.json)
set(APP_PACKAGE_JSON_IN ${CMAKE_CURRENT_SOURCE_DIR}/app.package.json.in) # Assume this template exists or create it

# Create a simple app.package.json.in for now
file(WRITE ${APP_PACKAGE_JSON_IN}
"{
  \"name\": \"ebeast\",
  \"productName\": \"EBeast\",
  \"version\": \"@BEAST_VERSION_M_M_M@\",
  \"config\": {
    \"version\": \"@BEAST_VERSION_M_M_M@\",
    \"revdate\": \"@BEAST_REVISION_DATE@\",
    \"revision\": \"@BEAST_REVISION_LONG@\",
    \"debug\": @BEAST_IS_DEBUG@
  },
  \"main\": \"main.js\"
}
"
)

execute_process(COMMAND ${CMAKE_SOURCE_DIR}/version.sh -s OUTPUT_VARIABLE BEAST_VERSION_M_M_M_SCRIPT OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${CMAKE_SOURCE_DIR}/version.sh -d OUTPUT_VARIABLE BEAST_REVISION_DATE_SCRIPT OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${CMAKE_SOURCE_DIR}/version.sh -l OUTPUT_VARIABLE BEAST_REVISION_LONG_SCRIPT OUTPUT_STRIP_TRAILING_WHITESPACE)
set(BEAST_VERSION_M_M_M ${BEAST_VERSION_M_M_M_SCRIPT}) # Use output from version.sh
set(BEAST_REVISION_DATE ${BEAST_REVISION_DATE_SCRIPT})
set(BEAST_REVISION_LONG ${BEAST_REVISION_LONG_SCRIPT})
if(CMAKE_BUILD_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg]|[Qq][Uu][Ii][Cc][Kk])$")
  set(BEAST_IS_DEBUG "true")
else()
  set(BEAST_IS_DEBUG "false")
endif()

configure_file(${APP_PACKAGE_JSON_IN} ${APP_PACKAGE_JSON_OUT} @ONLY)


# --- Generate index.html from template ---
set(INDEX_HTML_TEMPLATE_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/index.html.in) # Assume this exists
set(INDEX_HTML_DEST ${APP_BUILD_DIR}/index.html)
set(EKNOS_SVG_PATH ${CMAKE_CURRENT_SOURCE_DIR}/eknob.svg)

# Create a simple index.html.in for now
file(WRITE ${INDEX_HTML_TEMPLATE_SOURCE}
"<!DOCTYPE html>
<html>
<head>
    <title>EBeast</title>
    <!--EMBEDD='index-mjs.list'-->
    @INDEX_MJS_LIST_CONTENT@
</head>
<body>
    <div id=\"app\"></div>
    <script type=\"application/json\" id=\"--EMBEDD-package_json\">
        @PACKAGE_JSON_CONTENT@
    </script>
    <!--EMBEDD='eknob.svg'-->
    @EKNOS_SVG_CONTENT@
    <script type=\"module\" src=\"./vue.mjs\"></script>
    <script type=\"module\" src=\"./main.js\"></script>
</body>
</html>
"
)

# This custom command will read the content of generated files and pass them to configure_file
# This makes the embedding a build-time operation.
set(CONFIGURE_INDEX_HTML_SCRIPT ${EBEAST_BUILD_DIR}/configure_index_html.cmake)
file(WRITE ${CONFIGURE_INDEX_HTML_SCRIPT}
"
file(READ \\\"${APP_PACKAGE_JSON_OUT}\\\" PACKAGE_JSON_CONTENT)
file(READ \\\"${INDEX_MJS_LIST_FILE}\\\" INDEX_MJS_LIST_CONTENT)
file(READ \\\"${EKNOS_SVG_PATH}\\\" EKNOS_SVG_CONTENT)

# Escape content for CMake configure_file (especially backslashes and @ symbols)
string(REPLACE \\\"\\\\\\\" \\\"\\\\\\\\\\\" PACKAGE_JSON_CONTENT \\\"\${PACKAGE_JSON_CONTENT}\\\")
string(REPLACE \\\"@\\\" \\\"@@\\\" PACKAGE_JSON_CONTENT \\\"\${PACKAGE_JSON_CONTENT}\\\")
string(REPLACE \\\"\\\\\\\" \\\"\\\\\\\\\\\" INDEX_MJS_LIST_CONTENT \\\"\${INDEX_MJS_LIST_CONTENT}\\\")
string(REPLACE \\\"@\\\" \\\"@@\\\" INDEX_MJS_LIST_CONTENT \\\"\${INDEX_MJS_LIST_CONTENT}\\\")
string(REPLACE \\\"\\\\\\\" \\\"\\\\\\\\\\\" EKNOS_SVG_CONTENT \\\"\${EKNOS_SVG_CONTENT}\\\")
string(REPLACE \\\"@\\\" \\\"@@\\\" EKNOS_SVG_CONTENT \\\"\${EKNOS_SVG_CONTENT}\\\")

configure_file(\\\"${INDEX_HTML_TEMPLATE_SOURCE}\\\" \\\"${INDEX_HTML_DEST}\\\" @ONLY)
"
)

add_custom_command(
    OUTPUT ${INDEX_HTML_DEST}
    COMMAND ${CMAKE_COMMAND} -P ${CONFIGURE_INDEX_HTML_SCRIPT}
    DEPENDS
        ${INDEX_HTML_TEMPLATE_SOURCE}
        ${APP_PACKAGE_JSON_OUT}
        ${INDEX_MJS_LIST_FILE}
        ${EKNOS_SVG_PATH}
        ${NPM_DONE_STAMP} # For vue.mjs copy (not explicitly listed but part of overall process)
        ${BSEAPI_JSONIPC_JS}
        ${EBEAST_COPY_TOOL_TARGETS} # For JS files copied to app/
        ${APP_COPY_CONTENT_TARGETS}
        ${VUE_GENERATED_MJS_FILES} # For Vue component bundles
        ${VUE_GENERATED_CSS_FILES}
        ${MARKDOWN_IT_MJS_FINAL}
        ${BEASTYCONS_CSS}
        ${INTER_FONT_WOFF2}
        ${MATERIAL_ICONS_CSS}
        ${FORKAWESOME_CSS_FINAL}
    COMMENT "Generating index.html with embedded content"
)

# Main target for ebeast UI build (excluding Electron packaging for now)
add_custom_target(ebeast_web_ui ALL
  DEPENDS
    ${NPM_DONE_STAMP}
    ${INDEX_HTML_DEST} # This will pull in all its dependencies
    # Ensure assets that don't directly feed into index.html but are part of the UI build are also listed
    # For example, if some CSS or JS is not bundled but copied directly and used.
  COMMENT "Building EBeast Web UI components"
)

# --- ImageMagick Asset Generation ---
# These depend on app.css which is an output of rollup for app.vue
set(APP_VUE_CSS ${APP_BUILD_DIR}/b/app.css) # Output from rollup of app.vue

# Placeholder for extracting ImageMagick commands from CSS.
# This logic is complex: tr '\n' ' ' < $>/app/b/app.css | sed -nr 's/...gradient-01.../\1/; T; p'
# For now, assume these variables get populated with the correct string of IM arguments.
set(IM_ARGS_GRADIENT "-size 1x256 gradient:rgba(0,0,0,0.2)-rgba(0,0,0,0) -sigmoidal-contrast 10x50% -depth 8") # Example placeholder
set(IM_ARGS_SCROLLBAR_ARROW "-flop -background none -fill white -stroke none -strokewidth 0") # Example placeholder

set(GRADIENT_PNG ${EBEAST_ASSETS_DIR}/gradient-01.png)
add_custom_command(
  OUTPUT ${GRADIENT_PNG}
  COMMAND ${IMAGEMAGICK_CONVERT_EXECUTABLE} ${IM_ARGS_GRADIENT} ${GRADIENT_PNG}
  DEPENDS ${APP_VUE_CSS} # Depends on app.css for argument extraction (implicitly)
  COMMENT "Generating gradient-01.png with ImageMagick"
  VERBATIM # IM_ARGS_GRADIENT might contain shell-sensitive chars if not careful
)
list(APPEND EBEAST_GENERATED_ASSETS ${GRADIENT_PNG})

set(TRI_N_PNG_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/triangle32.png)
set(TRI_N_PNG_DEST ${EBEAST_ASSETS_DIR}/tri-n.png)
add_custom_command(
  OUTPUT ${TRI_N_PNG_DEST}
  COMMAND ${IMAGEMAGICK_CONVERT_EXECUTABLE} ${TRI_N_PNG_SOURCE} ${IM_ARGS_SCROLLBAR_ARROW} ${TRI_N_PNG_DEST}
  DEPENDS ${TRI_N_PNG_SOURCE} ${APP_VUE_CSS} # Depends on app.css for argument extraction (implicitly)
  COMMENT "Generating tri-n.png with ImageMagick"
  VERBATIM
)
list(APPEND EBEAST_GENERATED_ASSETS ${TRI_N_PNG_DEST})

set(TRI_E_PNG_DEST ${EBEAST_ASSETS_DIR}/tri-e.png)
add_custom_command(OUTPUT ${TRI_E_PNG_DEST} COMMAND ${IMAGEMAGICK_CONVERT_EXECUTABLE} ${TRI_N_PNG_DEST} -rotate 90 ${TRI_E_PNG_DEST} DEPENDS ${TRI_N_PNG_DEST} COMMENT "Generating tri-e.png")
list(APPEND EBEAST_GENERATED_ASSETS ${TRI_E_PNG_DEST})
set(TRI_S_PNG_DEST ${EBEAST_ASSETS_DIR}/tri-s.png)
add_custom_command(OUTPUT ${TRI_S_PNG_DEST} COMMAND ${IMAGEMAGICK_CONVERT_EXECUTABLE} ${TRI_N_PNG_DEST} -rotate 180 ${TRI_S_PNG_DEST} DEPENDS ${TRI_N_PNG_DEST} COMMENT "Generating tri-s.png")
list(APPEND EBEAST_GENERATED_ASSETS ${TRI_S_PNG_DEST})
set(TRI_W_PNG_DEST ${EBEAST_ASSETS_DIR}/tri-w.png)
add_custom_command(OUTPUT ${TRI_W_PNG_DEST} COMMAND ${IMAGEMAGICK_CONVERT_EXECUTABLE} ${TRI_N_PNG_DEST} -rotate 270 ${TRI_W_PNG_DEST} DEPENDS ${TRI_N_PNG_DEST} COMMENT "Generating tri-w.png")
list(APPEND EBEAST_GENERATED_ASSETS ${TRI_W_PNG_DEST})

# Add these generated assets to the main UI target's dependencies if they are part of index.html directly
# or copied/used by other parts of the UI build.
# For now, INDEX_HTML_DEST command should list them if it needs them.

# --- Copy Vue.js and runtime helpers ---
set(VUE_ESM_BROWSER_JS_DEST ${APP_BUILD_DIR}/vue.mjs)
add_custom_command(
  OUTPUT ${VUE_ESM_BROWSER_JS_DEST}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EBEAST_BUILD_DIR}/node_modules/vue/dist/vue.esm.browser.js ${VUE_ESM_BROWSER_JS_DEST}
  DEPENDS ${NPM_DONE_STAMP}
  COMMENT "Copying vue.esm.browser.js to app/"
)
set(VUE_RUNTIME_HELPERS_DEST ${APP_BUILD_DIR}/vue-runtime-helpers.mjs)
set(VUE_RUNTIME_HELPERS_SOURCE ${EBEAST_BUILD_DIR}/node_modules/vue-runtime-helpers/dist/index.mjs)
add_custom_command(
  OUTPUT ${VUE_RUNTIME_HELPERS_DEST}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${VUE_RUNTIME_HELPERS_SOURCE} ${VUE_RUNTIME_HELPERS_DEST}
  # Makefile also does: sed 's|^//# sourceMappingURL=index\.mjs\.map$$||' -i $>/app/vue-runtime-helpers.mjs
  # This sed can be added here if needed, or handled by patching if critical. For now, direct copy.
  DEPENDS ${NPM_DONE_STAMP}
  COMMENT "Copying vue-runtime-helpers to app/"
)
# Update ebeast_web_ui dependencies
add_dependencies(ebeast_web_ui ${VUE_ESM_BROWSER_JS_DEST} ${VUE_RUNTIME_HELPERS_DEST})


# --- Electron Packaging Step ---
# Assumes electron is installed via npm in ${EBEAST_BUILD_DIR}/node_modules/electron/dist
set(ELECTRON_SHELL_SOURCE_DIR ${EBEAST_BUILD_DIR}/node_modules/electron/dist) # Where original electron shell parts are
set(ELECTRON_FINAL_APP_DIR ${ELECTRON_BUILD_DIR}/electron) # Target structure $>/electron/
set(ELECTRON_EBEAST_EXE ${ELECTRON_FINAL_APP_DIR}/ebeast)  # Final executable $>/electron/ebeast

# Create a stamp file to signify electron packaging is done
set(ELECTRON_PACKAGED_STAMP ${ELECTRON_BUILD_DIR}/electron_packaged.stamp)

add_custom_command(
  OUTPUT ${ELECTRON_PACKAGED_STAMP}
  # 1. Copy the whole electron dist dir
  COMMAND ${CMAKE_COMMAND} -E remove_directory ${ELECTRON_FINAL_APP_DIR} # Clean first
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${ELECTRON_SHELL_SOURCE_DIR} ${ELECTRON_FINAL_APP_DIR}
  # 2. Remove default_app.asar
  COMMAND ${CMAKE_COMMAND} -E remove ${ELECTRON_FINAL_APP_DIR}/resources/default_app.asar
  # 3. Link our app ($>/app) into $>/electron/resources/app
  COMMAND ${CMAKE_COMMAND} -E create_symlink ../../app ${ELECTRON_FINAL_APP_DIR}/resources/app # Symlink to ${APP_BUILD_DIR}
  # 4. Rename electron executable to ebeast
  COMMAND ${CMAKE_COMMAND} -E rename ${ELECTRON_FINAL_APP_DIR}/electron ${ELECTRON_EBEAST_EXE}
  COMMAND ${CMAKE_COMMAND} -E touch ${ELECTRON_PACKAGED_STAMP} # Create stamp file
  DEPENDS ebeast_web_ui ${NPM_DONE_STAMP} # Depends on UI being built and npm install (for electron download)
  COMMENT "Packaging Electron application"
  VERBATIM
)

# Overall target
add_custom_target(ebeast_all ALL # Make it part of 'all' to build by default
  DEPENDS ebeast_web_ui ${ELECTRON_PACKAGED_STAMP} # Depends on UI and Electron packaging
  COMMENT "Building EBeast Application (Web UI and Electron Shell)"
)

# --- Installation ---
set(EBEAST_INSTALL_DEST_ROOT ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}) # pkglibdir

# Install the 'app' directory (built web UI)
install(
  DIRECTORY ${APP_BUILD_DIR}/
  DESTINATION ${EBEAST_INSTALL_DEST_ROOT}/app
  COMPONENT EBeastUI # Optional component name
)

# Install the 'electron' directory (packaged Electron shell)
# This depends on ELECTRON_PACKAGED_STAMP ensuring the electron app is ready
install(
  DIRECTORY ${ELECTRON_FINAL_APP_DIR}/
  DESTINATION ${EBEAST_INSTALL_DEST_ROOT}/electron
  COMPONENT EBeastApp # Optional component name
  DEPENDS ${ELECTRON_PACKAGED_STAMP} # Ensure packaging is done before install tries to copy
)

# Create symlink: $(pkglibdir)/bin/beast -> ../electron/ebeast
# The actual symlink target inside pkglibdir/bin will be ../../lib/<project>/electron/ebeast if pkglibdir/bin is a new dir.
# Or, more simply, relative to where the link is created.
# If link is at ${EBEAST_INSTALL_DEST_ROOT}/bin/beast, target is ../electron/ebeast
set(EBEAST_SYMLINK_TARGET_NAME beast) # Name of the symlink
set(EBEAST_SYMLINK_POINTING_TO "../${CMAKE_INSTALL_LIBDIR_BASENAME}/${PROJECT_NAME}/electron/ebeast") # Path from bin to actual exe
# If installing to <prefix>/lib/<project>/bin/beast, then target is ../electron/ebeast
# Let's use the Makefile's simpler relative path from the bin dir inside pkglibdir.
install(CODE "
  file(CREATE_LINK
    ${EBEAST_INSTALL_DEST_ROOT}/electron/ebeast
    ${EBEAST_INSTALL_DEST_ROOT}/bin/${EBEAST_SYMLINK_TARGET_NAME}
    SYMBOLIC)
  " COMPONENT EBeastApp)
# A more robust way for symlinks if using CMAKE_INSTALL_BINDIR directly:
# install(CREATE_SYMLINK
#   "../${CMAKE_INSTALL_LIBDIR_BASENAME}/${PROJECT_NAME}/electron/ebeast" # Target path relative to link location
#   "${CMAKE_INSTALL_BINDIR}/${EBEAST_SYMLINK_TARGET_NAME}" # Link name (absolute path)
# )
# For now, using the pkglibdir/bin structure from Makefile.

message(STATUS "ebeast/CMakeLists.txt initial setup processed.")
message(STATUS "  NPM install target created: ${NPM_DONE_STAMP}")
message(STATUS "  bseapi_jsonipc.js generation target created: ${BSEAPI_JSONIPC_JS}")
message(STATUS "  Placeholder for index.html and full UI build. Many steps (Rollup, assets) still TODO.")
message(STATUS "  Node modules bin: ${NODE_MODULES_BIN_DIR} (use this for rollup, eslint commands)")

# TODO:
# - Implement Rollup for Vue components (*.vue -> *.mjs, *.css)
# - Implement Rollup for markdown-it.mjs
# - Implement full index.html generation (embedding package.json, index-mjs.list, eknob.svg)
# - Implement all asset generation/downloading (Beastycons, Inter font, Material Icons, ForkAwesome, gradients, tri-pngs)
# - Implement Electron packaging steps
# - Implement Linting target
# - Implement Vue Docs generation
# - Implement full Installation rules
# - Add dependencies for commands on specific executables (e.g. $<TARGET_FILE:BeastSoundEngine>)
# - Ensure all paths used by custom commands (like rollup configs) are correct for build dir execution.
# - Replicate logic for `ebeast/run` if desired as a custom target.
# - The `ebeast/copy.targets` from Makefile need to be fully implemented using add_custom_command. (Partially done)
# - Anonymize build path in node_modules/package.json (complex sed, maybe skip or do differently)
