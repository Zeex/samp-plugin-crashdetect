include(AmxConfig)
include(CheckFunctionExists)
include(SampPlugin)

install(FILES crashdetect.inc DESTINATION ".")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/amx)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(UNIX)
  add_definitions(-DLINUX)
endif()

if(WIN32)
  add_definitions(-D_WIN32_WINNT=_WIN32_WINNT_WINXP
                  -D_CRT_SECURE_NO_WARNINGS)
endif()

set(PLUGIN_SOURCES
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
  configreader.cpp
  configreader.h
  crashdetect.cpp
  crashdetect.h
  fileutils.cpp
  fileutils.h
  logprintf.cpp
  logprintf.h
  natives.cpp
  natives.h
  os.h
  plugin.cpp
  plugin.def
  plugincommon.h
  stacktrace.cpp
  stacktrace.h
)

configure_file(plugin.rc.in plugin.rc @ONLY)
configure_file(pluginversion.h.in pluginversion.h @ONLY)

list(APPEND PLUGIN_SOURCES
  ${CMAKE_CURRENT_BINARY_DIR}/plugin.rc
  ${CMAKE_CURRENT_BINARY_DIR}/pluginversion.h
)

if(WIN32)
  list(APPEND PLUGIN_SOURCES
    fileutils-win32.cpp
    os-win32.cpp
    stacktrace-win32.cpp
  )
else()
  list(APPEND PLUGIN_SOURCES
    fileutils-unix.cpp
    os-unix.cpp
    stacktrace-unix.cpp
  )
endif()

add_samp_plugin(plugin ${PLUGIN_SOURCES})

if(UNIX OR MINGW)
  set_property(TARGET plugin PROPERTY PREFIX "")
endif()

if(WIN32)
  target_link_libraries(plugin DbgHelp Ws2_32)
endif()

add_subdirectory(amx)

target_link_Libraries(plugin amx subhook)

install(TARGETS plugin LIBRARY DESTINATION ".")