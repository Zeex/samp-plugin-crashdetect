include(ExternalProject)

set(DEPS_PREFIX ${CMAKE_CURRENT_BINARY_DIR})
set(DEPS_INCLUDE_DIR ${DEPS_PREFIX}/include)
set(DEPS_LIBRARY_DIR ${DEPS_PREFIX}/lib)

set(DEPS_COMMON_CMAKE_ARGS
  -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
  -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
  -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
  -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
  -DCMAKE_INSTALL_PREFIX=${DEPS_PREFIX}
)

macro(get_library_filename library filename)
  set(_prefix ${CMAKE_STATIC_LIBRARY_PREFIX})
  set(_suffix ${CMAKE_STATIC_LIBRARY_SUFFIX})
  set(${filename} ${_prefix}${library}${_suffix})
endmacro()

externalproject_add(configreader-external
  PREFIX ${DEPS_PREFIX}
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/configreader
  CMAKE_ARGS ${DEPS_COMMON_CMAKE_ARGS})

add_library(configreader STATIC IMPORTED GLOBAL)
add_dependencies(configreader configreader-external)

get_library_filename(configreader CONFIGREADER_FILENAME)
set_target_properties(configreader PROPERTIES
  IMPORTED_LOCATION ${DEPS_LIBRARY_DIR}/${CONFIGREADER_FILENAME})

externalproject_add(subhook-external
  PREFIX ${DEPS_PREFIX}
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/subhook
  CMAKE_ARGS -DSUBHOOK_STATIC=ON
             ${DEPS_COMMON_CMAKE_ARGS})

add_library(subhook STATIC IMPORTED GLOBAL)
add_dependencies(subhook subhook-external)

get_library_filename(subhook SUBHOOK_FILENAME)
set_target_properties(subhook PROPERTIES
  IMPORTED_LOCATION ${DEPS_LIBRARY_DIR}/${SUBHOOK_FILENAME})

list(APPEND DEPS_DEFINITIONS SUBHOOK_STATIC)

externalproject_add(pcre-external
  PREFIX ${DEPS_PREFIX}
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/pcre
  CMAKE_ARGS -DPCRE_BUILD_TESTS=FALSE
             -DPCRE_BUILD_PCREGREP=FALSE
             ${DEPS_COMMON_CMAKE_ARGS})

add_library(pcre STATIC IMPORTED GLOBAL)
add_dependencies(pcre pcre-external)

get_library_filename(pcre PCRE_FILENAME)
set_target_properties(pcre PROPERTIES
  IMPORTED_LOCATION ${DEPS_LIBRARY_DIR}/${PCRE_FILENAME})

if(WIN32)
  get_library_filename(pcred PCRE_FILENAME_DEBUG)
  set_target_properties(pcre PROPERTIES
    IMPORTED_LOCATION_DEBUG ${DEPS_LIBRARY_DIR}/${PCRE_FILENAME_DEBUG})
endif()

list(APPEND DEPS_DEFINITIONS PCRE_STATIC)

set_property(DIRECTORY ${CMAKE_SOURCE_DIR} APPEND PROPERTY
  COMPILE_DEFINITIONS ${DEPS_DEFINITIONS})
set_property(DIRECTORY ${CMAKE_SOURCE_DIR} APPEND PROPERTY
  INCLUDE_DIRECTORIES ${DEPS_INCLUDE_DIR})

foreach(target configreader-external subhook-external pcre-external)
  file(RELATIVE_PATH folder ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
  set_property(TARGET ${target} PROPERTY FOLDER ${folder})
endforeach()
