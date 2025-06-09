# plugins/CMakeLists.txt

# Find required tools and packages
find_package(GLIB2 REQUIRED COMPONENTS gio)
include_directories(SYSTEM ${GLIB2_INCLUDE_DIRS})

# Assuming gdk-pixbuf-csource is found by the root or bse CMakeLists.txt
# If not, add:
# pkg_check_modules(GDK_PIXBUF REQUIRED gdk-pixbuf-2.0)
# find_program(GDK_PIXBUF_CSOURCE_EXECUTABLE NAMES gdk-pixbuf-csource gdk-pixbuf-csource-2.0 REQUIRED)
# For now, assume GDK_PIXBUF_CSOURCE_EXECUTABLE is available (e.g., set as a CACHE variable or found by an earlier CMake file)

# SFIDL and libbse targets are expected to be available from other subdirectories
set(SFIDL_TARGET_NAME sfidl) # From sfi/CMakeLists.txt
set(BSE_TARGET_NAME bse)     # From bse/CMakeLists.txt

# --- Icon C Source Generation ---
set(PLUGIN_ICON_PNG_FILES
  adsr.png
  ampmix.png
  atan.png
  canyon.png
  drum.png
  filter.png
  prod.png
  reverb.png # Used by freeverb
  sequencer.png
  strings.png
  sub.png
  sum.png
)

set(PLUGIN_ICON_GENERATED_SOURCES "")
set(PLUGIN_ICONS_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/icons) # Output to plugins/icons
file(MAKE_DIRECTORY ${PLUGIN_ICONS_OUTPUT_DIR})

foreach(icon_png_basename IN LISTS PLUGIN_ICON_PNG_FILES)
  set(icon_png_full_path ${CMAKE_CURRENT_SOURCE_DIR}/icons/${icon_png_basename})
  string(REPLACE ".png" "" c_name_base ${icon_png_basename})
  # Makefile uses $*_icon -> adsr_png_icon, but gdk-pixbuf-csource --name usually just wants the identifier part
  set(c_identifier "${c_name_base}_icon") # e.g. adsr_icon
  set(generated_c_file ${PLUGIN_ICONS_OUTPUT_DIR}/${c_name_base}_icon.c)

  if(NOT EXISTS ${icon_png_full_path})
    message(WARNING "Icon source file not found: ${icon_png_full_path}")
    continue()
  endif()

  add_custom_command(
    OUTPUT ${generated_c_file}
    COMMAND ${GDK_PIXBUF_CSOURCE_EXECUTABLE} --name=${c_identifier} ${icon_png_full_path} > ${generated_c_file}.tmp
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${generated_c_file}.tmp ${generated_c_file}
    COMMAND ${CMAKE_COMMAND} -E remove ${generated_c_file}.tmp
    DEPENDS ${icon_png_full_path} ${GDK_PIXBUF_CSOURCE_EXECUTABLE}
    COMMENT "Generating C source from plugin icon: ${icon_png_basename}"
    VERBATIM
  )
  list(APPEND PLUGIN_ICON_GENERATED_SOURCES ${generated_c_file})
endforeach()

# Specific icon needed for freeverb
set(REVERB_ICON_C_FILE ${PLUGIN_ICONS_OUTPUT_DIR}/reverb_icon.c) # Path matches above loop's output for reverb.png

# --- IDL to .genidl.hh Generation (for bseplugins.so) ---
set(BSEPLUGINS_IDL_FILES
  artscompressor.idl
  bseamplifier.idl
  bsebalance.idl
  bsecontribsampleandhold.idl
  bsenoise.idl
  bsequantizer.idl
  bsesummation.idl
  davbassfilter.idl
  davchorus.idl
  davorgan.idl
  standardguspatchenvelope.idl
  standardsaturator.idl
)

set(BSEPLUGINS_GENERATED_IDL_HEADERS "")
foreach(idl_basename IN LISTS BSEPLUGINS_IDL_FILES)
  set(idl_full_path ${CMAKE_CURRENT_SOURCE_DIR}/${idl_basename})
  get_filename_component(idl_name_we ${idl_basename} NAME_WE) # e.g., artscompressor
  set(generated_idl_header ${CMAKE_CURRENT_BINARY_DIR}/${idl_name_we}.genidl.hh) # Output to plugins/

  if(NOT EXISTS ${idl_full_path})
    message(WARNING "IDL source file not found: ${idl_full_path}")
    continue()
  endif()

  # Makefile: $(sfi/sfidl) $(sfi/sfidl.includes) --plugin --macro $(<F) -I. $< > $@.tmp
  # $(<F) is the filename part of $<. Here, idl_basename.
  # -I. means include current directory (where plugins/*.idl are).
  add_custom_command(
    OUTPUT ${generated_idl_header}
    COMMAND ${SFIDL_TARGET_NAME} ${SFIDL_INCLUDES_ARGS} # SFIDL_INCLUDES_ARGS is still TODO from bse/
      --plugin
      --macro ${idl_basename} # Pass the filename for the --macro argument
      -I ${CMAKE_CURRENT_SOURCE_DIR} # -I. (relative to where command is run, or pass absolute path)
      ${idl_full_path} > ${generated_idl_header}.tmp
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${generated_idl_header}.tmp ${generated_idl_header}
    COMMAND ${CMAKE_COMMAND} -E remove ${generated_idl_header}.tmp
    DEPENDS ${idl_full_path} ${SFIDL_TARGET_NAME}
    COMMENT "Generating IDL header from: ${idl_basename}"
    VERBATIM
  )
  list(APPEND BSEPLUGINS_GENERATED_IDL_HEADERS ${generated_idl_header})
endforeach()


# --- cxxplugins.so ---
set(CXXPLUGINS_SOURCES
  bseadder.cc
  bseatandistort.cc
  bseiirfilter.cc
  bsemixer.cc
  bsemult.cc
  bsesequencer.cc
  bsesimpleadsr.cc
  davcanyondelay.cc
  davsyndrum.cc
  davxtalstrings.cc
)
# Prepend CMAKE_CURRENT_SOURCE_DIR to source files
set(CXXPLUGINS_FULLPATH_SOURCES "")
foreach(src IN LISTS CXXPLUGINS_SOURCES)
  list(APPEND CXXPLUGINS_FULLPATH_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${src}")
endforeach()

add_library(cxxplugins MODULE
  ${CXXPLUGINS_FULLPATH_SOURCES}
  ${PLUGIN_ICON_GENERATED_SOURCES} # All generated icons go into cxxplugins as per Makefile logic (implicit)
                                   # This might need refinement if specific icons go to specific plugins.
                                   # Makefile links $(plugins/icons/c.files) to $(plugins/cxxplugins.objects)
                                   # and also to $(plugins/bseplugins.objects)
)
target_include_directories(cxxplugins PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR} # For generated icon C files and IDL headers
  ${CMAKE_CURRENT_SOURCE_DIR} # For plugin's own headers
  $<TARGET_PROPERTY:bse,INTERFACE_INCLUDE_DIRECTORIES> # Includes from libbse
  ${GLIB2_INCLUDE_DIRS}
)
target_link_libraries(cxxplugins PRIVATE
  ${BSE_TARGET_NAME} # Link against libbse
  # BSEDEPS_LIBS are expected to be linked via libbse, or need to be added here if used directly
  ${GLIB2_LIBRARIES}
)
set_target_properties(cxxplugins PROPERTIES PREFIX "") # Typically plugins don't have "lib" prefix
set_target_properties(cxxplugins PROPERTIES INSTALL_RPATH "$ORIGIN/../lib") # RPATH to find libbse

# --- bseplugins.so ---
set(BSEPLUGINS_SOURCES
  artscompressor.cc
  bseamplifier.cc
  bsebalance.cc
  bsenoise.cc
  bsequantizer.cc
  bsesummation.cc
  standardsaturator.cc
  standardguspatchenvelope.cc
  bsecontribsampleandhold.cc
  davbassfilter.cc
  davchorus.cc
  davorgan.cc
)
set(BSEPLUGINS_FULLPATH_SOURCES "")
foreach(src IN LISTS BSEPLUGINS_SOURCES)
  list(APPEND BSEPLUGINS_FULLPATH_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${src}")
endforeach()

add_library(bseplugins MODULE
  ${BSEPLUGINS_FULLPATH_SOURCES}
  ${BSEPLUGINS_GENERATED_IDL_HEADERS} # These are headers, sources will include them.
                                      # The custom commands generate .hh files.
                                      # The .o files depend on .genidl.hh in Makefile.
                                      # This means sources must #include these generated headers.
  ${PLUGIN_ICON_GENERATED_SOURCES}    # Also links against all icons.
)
target_include_directories(bseplugins PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  $<TARGET_PROPERTY:bse,INTERFACE_INCLUDE_DIRECTORIES>
  ${GLIB2_INCLUDE_DIRS}
)
target_link_libraries(bseplugins PRIVATE
  ${BSE_TARGET_NAME}
  ${GLIB2_LIBRARIES}
)
set_target_properties(bseplugins PROPERTIES PREFIX "")
set_target_properties(bseplugins PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")


# --- freeverb.so ---
set(FREEVERB_SOURCES
  freeverb/allpass.cc
  freeverb/comb.cc
  freeverb/revmodel.cc
  freeverb/bsefreeverbcpp.cc
  freeverb/bsefreeverb.cc
)
set(FREEVERB_FULLPATH_SOURCES "")
foreach(src IN LISTS FREEVERB_SOURCES)
  list(APPEND FREEVERB_FULLPATH_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${src}")
endforeach()

add_library(freeverb MODULE
  ${FREEVERB_FULLPATH_SOURCES}
  ${REVERB_ICON_C_FILE} # Specific icon for freeverb
)
target_include_directories(freeverb PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR} # For reverb_icon.c
  ${CMAKE_CURRENT_SOURCE_DIR} # For its own headers in freeverb/
  ${CMAKE_CURRENT_SOURCE_DIR}/freeverb # For its own headers in freeverb/
  $<TARGET_PROPERTY:bse,INTERFACE_INCLUDE_DIRECTORIES>
  ${GLIB2_INCLUDE_DIRS}
)
# Special compile flag from Makefile for freeverb.o from bsefreeverb.cc (not freeverb/freeverb.o as that doesn't exist)
# The makefile rule is: $>/devices/freeverb/freeverb.o: EXTRA_FLAGS ::= -Wno-unused-function
# This was for a file in devices/, not plugins/freeverb.
# plugins/freeverb/bsefreeverb.o might need it if it contains similar code.
# For now, no special flags here unless a specific file is identified from plugins/freeverb.

target_link_libraries(freeverb PRIVATE
  ${BSE_TARGET_NAME}
  ${GLIB2_LIBRARIES}
)
set_target_properties(freeverb PROPERTIES PREFIX "")
set_target_properties(freeverb PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")


# --- Installation ---
set(PLUGINS_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}/plugins)
install(TARGETS cxxplugins bseplugins freeverb
  LIBRARY DESTINATION ${PLUGINS_INSTALL_DIR} # MODULE type installs to LIBRARY_OUTPUT_PATH
  RUNTIME DESTINATION ${PLUGINS_INSTALL_DIR} # For Windows DLLs if applicable
)

message(STATUS "Plugins CMakeLists.txt processed.")
message(STATUS "  Output directory for icons: ${PLUGIN_ICONS_OUTPUT_DIR}")
message(STATUS "  Output directory for IDL headers: ${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "  SFIDL_INCLUDES_ARGS still needs to be determined for sfidl calls if they fail.")
message(STATUS "  BSEDEPS_LIBS linkage for plugins relies on libbse exporting them or direct linking here (currently not done).")
