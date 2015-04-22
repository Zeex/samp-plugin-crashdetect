include(FindPackageHandleStandardArgs)

if(WIN32)
  set(_SAMPServerCLI_EXECUTABLE_NAME samp-server-cli.bat
                                     samp-server-cli.exe)
else()
  set(_SAMPServerCLI_EXECUTABLE_NAME samp-server-cli)
endif()

find_file(SAMPServerCLI_EXECUTABLE
  NAMES ${_SAMPServerCLI_EXECUTABLE_NAME}
  HINTS ENV SAMP_SERVER_ROOT
)
find_path(SAMPServerCLI_DIR
  NAMES ${_SAMPServerCLI_EXECUTABLE_NAME}
  HINTS ENV SAMP_SERVER_ROOT
)

mark_as_advanced(
  SAMPServerCLI_EXECUTABLE
  SAMPServerCLI_DIR
)

find_package_handle_standard_args(SAMPServerCLI
  FOUND_VAR SAMPServerCLI_FOUND
  REQUIRED_VARS SAMPServerCLI_EXECUTABLE
                 SAMPServerCLI_DIR
)
