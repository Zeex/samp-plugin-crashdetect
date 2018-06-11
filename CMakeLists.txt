cmake_minimum_required(VERSION 3.0)
project(crashdetect)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(AMXConfig)
include(AddSAMPPlugin)
include(GetGitRevisionDescription)
include(CTest)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
  if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
  endif()
  if(CMAKE_COMPILER_IS_GNUCC
     AND NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 4.0)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-attributes")
  elseif(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-ignored-attributes")
  endif()
endif()
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
  endif()
  if(CMAKE_CXX_COMPILER_IS_GNUCXX
     AND NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.0)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-attributes")
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-ignored-attributes")
  endif()
endif()

add_subdirectory(deps)

git_describe(version --match v[0-9]*.[0-9]**)
if(version)
  string(REGEX REPLACE "^v(.*)$" "\\1" version ${version})
else()
  message(STATUS "Failed to get version from Git, will read VERSION.txt")
  file(READ ${CMAKE_CURRENT_SOURCE_DIR}/VERSION.txt version)
  string(STRIP ${version} version)
endif()

string(REGEX REPLACE "([0-9]+(\\.[0-9]+)+).*" "\\1" version_rc ${version})
string(REGEX REPLACE "\\." "," version_rc ${version_rc})
string(REGEX REPLACE "^([0-9]+,[0-9]+)$" "\\1,0,0" version_rc ${version_rc})
string(REGEX REPLACE "^([0-9]+,[0-9]+,[0-9]+)$" "\\1,0" version_rc
      ${version_rc})

set(PROJECT_VERSION ${version})
set(PROJECT_VERSION_RC ${version_rc})

if(WIN32)
  add_definitions(-D_WIN32_WINNT=_WIN32_WINNT_WINXP
                  -D_CRT_SECURE_NO_WARNINGS
                  -DWIN32_LEAN_AND_MEAN)
endif()

set(CRASHDETECT_SOURCES
  src/amxcallstack.cpp
  src/amxcallstack.h
  src/amxdebuginfo.cpp
  src/amxdebuginfo.h
  src/amxerror.cpp
  src/amxerror.h
  src/amxhandler.h
  src/amxopcode.cpp
  src/amxopcode.h
  src/amxpathfinder.cpp
  src/amxpathfinder.h
  src/amxscript.cpp
  src/amxscript.h
  src/amxstacktrace.cpp
  src/amxstacktrace.h
  src/crashdetecthandler.cpp
  src/crashdetecthandler.h
  src/fileutils.cpp
  src/fileutils.h
  src/log.cpp
  src/log.h
  src/logprintf.cpp
  src/logprintf.h
  src/natives.cpp
  src/natives.h
  src/os.h
  src/plugin.cpp
  src/plugin.def
  src/plugincommon.h
  src/regexp.cpp
  src/regexp.h
  src/stacktrace.cpp
  src/stacktrace.h
  src/stringutils.cpp
  src/stringutils.h
)

configure_file(src/plugin.rc.in src/plugin.rc @ONLY)
configure_file(src/pluginversion.h.in src/pluginversion.h @ONLY)

list(APPEND CRASHDETECT_SOURCES
  ${CMAKE_CURRENT_BINARY_DIR}/src/plugin.rc
  ${CMAKE_CURRENT_BINARY_DIR}/src/pluginversion.h
)

if(WIN32 OR CYGWIN)
  list(APPEND CRASHDETECT_SOURCES
    src/fileutils-win32.cpp
    src/os-win32.cpp
    src/stacktrace-win32.cpp
  )
else()
  list(APPEND CRASHDETECT_SOURCES
    src/fileutils-unix.cpp
    src/os-unix.cpp
    src/stacktrace-unix.cpp
  )
endif()

add_samp_plugin(crashdetect ${CRASHDETECT_SOURCES})

target_include_directories(crashdetect PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_BINARY_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/amx
)

add_library(amx STATIC
  src/amx/amx.c
  src/amx/amx.h
  src/amx/amxaux.c
  src/amx/amxaux.h
  src/amx/amxdbg.c
  src/amx/amxdbg.h
  src/amx/getch.h
  src/amx/osdefs.h
  src/amx/sclinux.h
)

target_compile_definitions(amx PRIVATE
  -DAMX_ALIGN
  -DAMX_CLEANUP
  -DAMX_EXEC
  -DAMX_FLAGS
  -DAMX_GETADDR
  -DAMX_INIT
  -DAMX_REGISTER
  -DAMX_SETCALLBACK
  -DAMX_SETDEBUGHOOK
  -DAMX_XXXNATIVES
  -DAMX_XXXPUBLICS
  -DAMX_XXXUSERDATA
  -DAMX_ANSIONLY
  -DAMX_NODYNALOAD
)

target_include_directories(amx PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src/amx
)

target_link_Libraries(crashdetect amx configreader pcre subhook)
if(WIN32)
  target_link_libraries(crashdetect DbgHelp)
endif()

install(FILES include/crashdetect.inc DESTINATION "include")
install(TARGETS crashdetect LIBRARY DESTINATION ".")

if(MSVC_IDE AND MSVC_VERSION GREATER 1200)
  set(config \${CMAKE_INSTALL_CONFIG_NAME})
  set(pdb_name crashdetect.pdb)
  set(pdb_path ${CMAKE_CURRENT_BINARY_DIR}/${config}/${pdb_name})
  install(FILES ${pdb_path} DESTINATION ".")
endif()

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
if(WIN32)
  set(CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_NAME}-${version}-win32)
  set(CPACK_GENERATOR ZIP)
elseif(UNIX)
  set(CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_NAME}-${version}-linux)
  set(CPACK_GENERATOR TGZ)
endif()

include(CPack)
