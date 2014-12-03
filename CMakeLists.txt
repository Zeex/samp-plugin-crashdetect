project(crashdetect)

cmake_minimum_required(VERSION 2.8.8)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(GetGitRevisionDescription)
include(CTest)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(CMAKE_COMPILER_IS_GNUCC)
  if(NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 4.0)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-attributes")
  endif()
endif()
if(CMAKE_COMPILER_IS_GNUCXX)
  if(NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.0)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-attributes")
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

set(PROJECT_VERSION    ${version})
set(PROJECT_VERSION_RC ${version_rc})

add_subdirectory(include)
add_subdirectory(src)

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
