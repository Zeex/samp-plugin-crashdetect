include(PluginTest)

find_package(PawnCompiler REQUIRED)
find_package(PythonInterp 2.7 REQUIRED)
find_package(SAMPServer REQUIRED)
find_package(SAMPServerCLI REQUIRED)

macro(test target name)
  file(READ ${name}/output.txt TEST_OUTPUT)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/full_output.txt.in
  ${CMAKE_CURRENT_BINARY_DIR}/${name}/full_output.txt)

  set(_compile_flags "")

  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${name}/pawn.cfg)
    file(READ ${name}/pawn.cfg _compile_flags)
    string(REGEX REPLACE "[\n\\s]" ";" _compile_flags ${_compile_flags})
  endif()

  list(APPEND _compile_flags
    ${CMAKE_CURRENT_SOURCE_DIR}/${name}/test.pwn
    "-\;+"
    "-(+"
    -i${SAMPServer_INCLUDE_DIR}
    -i${CMAKE_CURRENT_SOURCE_DIR}/include
    -i${PROJECT_SOURCE_DIR}/include
    -o${CMAKE_CURRENT_BINARY_DIR}/${name}/test
  )

  if(UNIX)
    string(REPLACE "\;" "\\$<SEMICOLON>" _compile_flags "${_compile_flags}")
    string(REPLACE "(" "\\(" _compile_flags "${_compile_flags}")
  endif()

  add_custom_command(
    OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/${name}/test.amx
    COMMAND ${PawnCompiler_EXECUTABLE} ${_compile_flags}
    COMMENT "Compiling test ${name}"
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${name}/test.pwn
  )

  add_plugin_test(${name}
    TARGET      ${target}
    SCRIPT      ${CMAKE_CURRENT_BINARY_DIR}/${name}/test
    OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/${name}/full_output.txt
    TIMEOUT     0.5
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )

  get_filename_component(_python_dir ${PYTHON_EXECUTABLE} DIRECTORY)

  if(WIN32)
    set(_path "${_python_dir};${SAMPServerCLI_DIR};$ENV{Path}")
    string(REPLACE ";" "\\$<SEMICOLON>" _path "${_path}")
  else()
    set(_path "${_python_dir}:${SAMPServerCLI_DIR}:$ENV{PATH}")
  endif()

  set(_env
    SAMP_SERVER_ROOT=${SAMPServer_DIR}
    SAMP_SERVER=${SAMPServer_EXECUTABLE}
    PATH=${_path}
  )
  set_property(TEST ${name} APPEND PROPERTY ENVIRONMENT ${_env})
endmacro()

macro(tests target)
  set(_amx_files "")

  foreach(name ${ARGN})
    test(${target} ${name})
    list(APPEND _amx_files ${CMAKE_CURRENT_BINARY_DIR}/${name}/test.amx)
  endforeach()

  add_custom_target(${target}-tests ALL DEPENDS ${_amx_files})
endmacro()

file(STRINGS tests.list CRASHDETECT_TESTS)
tests(crashdetect ${CRASHDETECT_TESTS})
