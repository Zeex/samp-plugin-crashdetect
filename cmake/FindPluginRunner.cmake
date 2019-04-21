include(FindPackageHandleStandardArgs)

if(WIN32)
  set(_PluginRunner_EXECUTABLE_NAME plugin-runner.exe)
else()
  set(_PluginRunner_EXECUTABLE_NAME plugin-runner)
endif()

find_file(PluginRunner_EXECUTABLE
  NAMES ${_PluginRunner_EXECUTABLE_NAME}
  HINTS ENV SAMP_SERVER_ROOT
)
find_path(PluginRunner_DIR
  NAMES ${_PluginRunner_EXECUTABLE_NAME}
  HINTS ENV SAMP_SERVER_ROOT
)

mark_as_advanced(
  PluginRunner_EXECUTABLE
  PluginRunner_DIR
)

find_package_handle_standard_args(PluginRunner
  FOUND_VAR PluginRunner_FOUND
  REQUIRED_VARS PluginRunner_EXECUTABLE PluginRunner_DIR
)
