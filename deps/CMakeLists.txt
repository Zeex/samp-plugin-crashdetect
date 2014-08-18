set(PCRE_BUILD_TESTS FALSE)
set(PCRE_BUILD_PCREGREP FALSE)
add_subdirectory(pcre)

set(SUBHOOK_STATIC TRUE)
add_subdirectory(subhook)

set(DEPS_DEFINITIONS
  SUBHOOK_STATIC
  PCRE_STATIC
)

foreach(def ${DEPS_DEFINITIONS})
  set_property(DIRECTORY ${CMAKE_SOURCE_DIR} APPEND PROPERTY
               COMPILE_DEFINITIONS ${def})
endforeach()

set(DEPS_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/subhook
  ${CMAKE_CURRENT_SOURCE_DIR}/pcre
  ${CMAKE_CURRENT_BINARY_DIR}/pcre
)

foreach(dir ${DEPS_INCLUDE_DIRS})
  set_property(DIRECTORY ${CMAKE_SOURCE_DIR} APPEND PROPERTY
               INCLUDE_DIRECTORIES ${dir})
endforeach()

set(DEPS_TARGETS
  pcre
  pcrecpp
  pcreposix
  subhook
)

file(RELATIVE_PATH DEPS_FOLDER ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

foreach(target ${DEPS_TARGETS})
  set_property(TARGET ${target} PROPERTY FOLDER ${DEPS_FOLDER})
endforeach()
