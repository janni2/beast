# launchers/CMakeLists.txt

set(LAUNCHER_SOURCES
  suidmain.c
  beaststart.c
)

set(LAUNCHER_EXECUTABLE_NAME BeastSoundEngine-rt)
# The Makefile output path is $>/lib/BeastSoundEngine-$(VERSION_M.M.M)-rt
# For CMake, the build location is handled by CMAKE_RUNTIME_OUTPUT_DIRECTORY,
# but the final installed name should match.
# We can set OUTPUT_NAME if needed, or ensure target name matches install name.
# Let's use the target name directly as BeastSoundEngine-rt.

add_executable(${LAUNCHER_EXECUTABLE_NAME} ${LAUNCHER_SOURCES})

# No special compile flags or link libraries mentioned for the launcher objects/program
# in the Makefile snippet, beyond what's globally set.
# It links with no explicit libs, so just standard C libraries.

# --- Installation ---
# Makefile installs to: $(DESTDIR)$(pkglibdir)/lib
# pkglibdir in CMake context is usually ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}
set(LAUNCHER_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME})

install(TARGETS ${LAUNCHER_EXECUTABLE_NAME}
  RUNTIME DESTINATION ${LAUNCHER_INSTALL_DIR}
  # No LIBRARY or ARCHIVE destination as it's an executable.
)

# IMPORTANT SUID COMMENT:
# The original Makefile attempts to set SUID root permissions (chown root, chmod 4755)
# on the installed BeastSoundEngine-rt executable.
# CMake's install() command cannot reliably perform these privileged operations
# during a standard build/install process.
# Such permissions should be handled by the system administrator or, preferably,
# by the packaging system (e.g., .deb package rules, RPM spec files)
# which have established mechanisms for managing file ownership and permissions.
#
# Example for Debian packaging (.rules file):
# dh_fixperms
# override_dh_fixperms:
#   dh_fixperms
#   chmod 4755 debian/<pkg>/usr/lib/<project-name>/BeastSoundEngine-rt
#   chown root:root debian/<pkg>/usr/lib/<project-name>/BeastSoundEngine-rt
#
# Or, a system administrator would run after installation:
# sudo chown root:root <prefix>/lib/<project-name>/BeastSoundEngine-rt
# sudo chmod 4755 <prefix>/lib/<project-name>/BeastSoundEngine-rt

message(STATUS "launchers/CMakeLists.txt processed for ${LAUNCHER_EXECUTABLE_NAME}.")
message(WARNING "SUID permissions for ${LAUNCHER_EXECUTABLE_NAME} must be handled by packaging or sysadmin post-install.")
