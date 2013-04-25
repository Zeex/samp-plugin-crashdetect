cmake_minimum_required(VERSION 2.8.6)
project(crashdetect)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  include("cmake/GlobalBuildConfig.cmake")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")

include(AmxConfig)
include(CheckFunctionExists)
include(CTest)
include(GetGitRevisionDescription)
include(SampPlugin)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/amx")
include_directories("${CMAKE_CURRENT_BINARY_DIR}/plugin")

if(UNIX)
  add_definitions(-DLINUX)
endif()

set(SOURCES
  "plugin/amxdebuginfo.cpp"
  "plugin/amxdebuginfo.h"
  "plugin/amxerror.cpp"
  "plugin/amxerror.h"
  "plugin/amxopcode.cpp"
  "plugin/amxopcode.h"
  "plugin/amxpathfinder.cpp"
  "plugin/amxpathfinder.h"
  "plugin/amxscript.cpp"
  "plugin/amxscript.h"
  "plugin/amxservice.h"
  "plugin/amxstacktrace.cpp"
  "plugin/amxstacktrace.h"
  "plugin/configreader.cpp"
  "plugin/configreader.h"
  "plugin/compiler.h"
  "plugin/crashdetect.cpp"
  "plugin/crashdetect.h"
  "plugin/fileutils.cpp"
  "plugin/fileutils.h"
  "plugin/hook.cpp"
  "plugin/hook.h"
  "plugin/logprintf.cpp"
  "plugin/logprintf.h"
  "plugin/npcall.cpp"
  "plugin/npcall.h"
  "plugin/os.h"
  "plugin/plugin.cpp"
  "plugin/plugin.def"
  "plugin/plugincommon.h"
  "plugin/stacktrace.cpp"
  "plugin/stacktrace.h"
  "plugin/stacktrace-generic.cpp"
  "plugin/stacktrace-generic.h"
  "plugin/tcpsocket.h"
  "plugin/thread.h"
  "plugin/updater.cpp"
  "plugin/updater.h"
  "plugin/version.cpp"
  "plugin/version.h"
  "${CMAKE_CURRENT_BINARY_DIR}/plugin/plugin.rc"
  "${CMAKE_CURRENT_BINARY_DIR}/plugin/pluginversion.h"
)

if(WIN32)
  list(APPEND SOURCES
    "plugin/fileutils-win32.cpp"
    "plugin/hook-win32.cpp"
    "plugin/os-win32.cpp"
    "plugin/stacktrace-win32.cpp"
    "plugin/tcpsocket-win32.cpp"
    "plugin/thread-win32.cpp"
  )
elseif(UNIX)
  list(APPEND SOURCES
    "plugin/fileutils-unix.cpp"
    "plugin/hook-unix.cpp"
    "plugin/os-unix.cpp"
    "plugin/stacktrace-unix.cpp"
    "plugin/tcpsocket-unix.cpp"
    "plugin/thread-unix.cpp"
  )
else()
  message(FATAL_ERROR "Unsupported operating system.")
endif()

if(MSVC)
  list(APPEND SOURCES "plugin/compiler-msvc.cpp")
elseif(CMAKE_COMPILER_IS_GNUCXX)
  list(APPEND SOURCES "plugin/compiler-gcc.cpp")
else()
  message(FATAL_ERROR "Unsupported compiler.")
endif()

add_samp_plugin(${PROJECT_NAME} ${SOURCES})

if(UNIX OR MINGW)
  set_property(TARGET ${PROJECT_NAME} PROPERTY PREFIX "")
endif()

if(WIN32)
  if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  endif()
  target_link_libraries(${PROJECT_NAME} Ws2_32)
endif()  

if(UNIX)
  check_function_exists(backtrace HAVE_BACKTRACE)
  if(HAVE_BACKTRACE)
    add_definitions(-DHAVE_BACKTRACE)
  endif()
  check_function_exists(backtrace_symbols HAVE_BACKTRACE_SYMBOLS)
  if(HAVE_BACKTRACE_SYMBOLS)
    add_definitions(-DHAVE_BACKTRACE_SYMBOLS)
  endif()
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
  set_property(TARGET ${PROJECT_NAME} APPEND_STRING PROPERTY
    LINK_FLAGS " -pthread")
endif()

target_link_Libraries(${PROJECT_NAME} amx)

git_describe(description --match v[0-9]*.[0-9]**)
if(description)
  string(REGEX REPLACE "\\-g[0-9a-f]+$" "" description ${description})
  string(REGEX REPLACE "^v(.*)" "\\1" version ${description})
else()
  message(STATUS "Failed geting version from Git, will read VERSION.txt.")
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION.txt" version)
  string(STRIP "${version}" version)
endif()

string(REGEX REPLACE "[.]" "," version_rc ${version})
string(REGEX REPLACE "[-].*" "" version_rc ${version_rc})
string(REGEX REPLACE "^([0-9]+,[0-9]+)$" "\\1,0,0" version_rc ${version_rc})
string(REGEX REPLACE "^([0-9]+,[0-9]+,[0-9]+)$" "\\1,0" version_rc ${version_rc})

set(PROJECT_VERSION    "${version}")
set(PROJECT_VERSION_RC "${version_rc}")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory("amx")
add_subdirectory("plugin")
add_subdirectory("tests")

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ".")

if(MSVC)
  set(PDB_NAME "${PROJECT_NAME}.pdb")
  set(PDB_PATH "${CMAKE_CURRENT_BINARY_DIR}/\${CMAKE_INSTALL_CONFIG_NAME}/${PDB_NAME}")
  install(FILES ${PDB_PATH} DESTINATION ".")
endif()

set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
if(WIN32)
  set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${version}-win32")
  set(CPACK_GENERATOR ZIP)
elseif(UNIX)
  set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${version}-linux")
  set(CPACK_GENERATOR TGZ)
endif()
include(CPack)

message(STATUS "You're going to build ${PROJECT_NAME} v${PROJECT_VERSION}")
