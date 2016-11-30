include(AMXConfig)
include(AddSAMPPlugin)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/amx
  ${CMAKE_CURRENT_BINARY_DIR}
)

if(WIN32)
  add_definitions(-D_WIN32_WINNT=_WIN32_WINNT_WINXP
                  -D_CRT_SECURE_NO_WARNINGS)
endif()

set(CRASHDETECT_SOURCES
  amxcallstack.cpp
  amxcallstack.h
  amxdebuginfo.cpp
  amxdebuginfo.h
  amxerror.cpp
  amxerror.h
  amxopcode.cpp
  amxopcode.h
  amxpathfinder.cpp
  amxpathfinder.h
  amxscript.cpp
  amxscript.h
  amxservice.h
  amxstacktrace.cpp
  amxstacktrace.h
  crashdetect.cpp
  crashdetect.h
  fileutils.cpp
  fileutils.h
  log.cpp
  log.h
  logprintf.cpp
  logprintf.h
  natives.cpp
  natives.h
  os.h
  plugin.cpp
  plugin.def
  plugincommon.h
  regexp.cpp
  regexp.h
  stacktrace.cpp
  stacktrace.h
)

configure_file(plugin.rc.in plugin.rc @ONLY)
configure_file(pluginversion.h.in pluginversion.h @ONLY)

list(APPEND CRASHDETECT_SOURCES
  ${CMAKE_CURRENT_BINARY_DIR}/plugin.rc
  ${CMAKE_CURRENT_BINARY_DIR}/pluginversion.h
)

if(WIN32 OR CYGWIN)
  list(APPEND CRASHDETECT_SOURCES
    fileutils-win32.cpp
    os-win32.cpp
    stacktrace-win32.cpp
  )
else()
  list(APPEND CRASHDETECT_SOURCES
    fileutils-unix.cpp
    os-unix.cpp
    stacktrace-unix.cpp
  )
endif()

add_samp_plugin(crashdetect ${CRASHDETECT_SOURCES})

add_subdirectory(amx)
target_link_Libraries(crashdetect amx configreader pcre subhook)

if(WIN32)
  target_link_libraries(crashdetect DbgHelp)
endif()

install(TARGETS crashdetect LIBRARY DESTINATION ".")

if(MSVC_IDE AND MSVC_VERSION GREATER 1200)
  set(config \${CMAKE_INSTALL_CONFIG_NAME})
  set(pdb_name crashdetect.pdb)
  set(pdb_path ${CMAKE_CURRENT_BINARY_DIR}/${config}/${pdb_name})
  install(FILES ${pdb_path} DESTINATION ".")
endif()
