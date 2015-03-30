include(FindPackageHandleStandardArgs)

if(WIN32)
  set(_SAMPServer_EXECUTABLE_NAME samp-server.exe)
else()
  set(_SAMPServer_EXECUTABLE_NAME samp03svr)
endif()

find_file(SAMPServer_EXECUTABLE
  NAMES ${_SAMPServer_EXECUTABLE_NAME}
  HINTS ENV SAMP_SERVER_ROOT
)
find_path(SAMPServer_DIR
  NAMES ${_SAMPServer_EXECUTABLE_NAME}
  HINTS ENV SAMP_SERVER_ROOT
)
find_path(SAMPServer_INCLUDE_DIR
  NAMES a_samp.inc a_npc.inc
  HINTS ${SAMPServer_DIR}/include
        ${SAMPServer_DIR}/pawno/include
)

mark_as_advanced(
  SAMPServer_EXECUTABLE
  SAMPServer_DIR
  SAMPServer_INCLUDE_DIR
)

find_package_handle_standard_args(SAMPServer
  FOUND_VAR SAMPServer_FOUND
  REQUIRED_VARS SAMPServer_EXECUTABLE
                SAMPServer_DIR
                SAMPServer_INCLUDE_DIR
)
