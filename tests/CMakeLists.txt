include(AddSAMPPluginTest)

find_package(PawnCC REQUIRED)
find_package(PythonInterp 2.7 REQUIRED)
find_package(SAMPServer REQUIRED)
find_package(SAMPServerCLI REQUIRED)

macro(test target name)
  file(STRINGS ${name}.pwn _test_code)

  set(_test_output "")
  foreach(line ${_test_code})
    string(REGEX MATCHALL "OUTPUT: .*" output ${line})
    if(output)
      string(REPLACE "OUTPUT: " "" output ${output})
      set(_test_output "${_test_output}${output}\n")
    endif()
  endforeach()

  set(TEST_OUTPUT ${_test_output})
  configure_file(test.out.in ${name}.out)

  set(_compile_flags "")
  foreach(line ${_test_code})
    string(REGEX MATCHALL "FLAGS: .*" flags ${line})
    if(flags)
      string(REPLACE "FLAGS: " "" flags ${flags})
      list(APPEND _compile_flags ${flags})
    endif()
  endforeach()

  list(APPEND _compile_flags
    ${CMAKE_CURRENT_SOURCE_DIR}/${name}.pwn
    "-\;+"
    "-(+"
    -i${SAMPServer_INCLUDE_DIR}
    -i${PROJECT_SOURCE_DIR}/include
    -o${CMAKE_CURRENT_BINARY_DIR}/${name}
  )

  if(UNIX)
    string(REPLACE "\;" "\\$<SEMICOLON>" _compile_flags "${_compile_flags}")
    string(REPLACE "(" "\\(" _compile_flags "${_compile_flags}")
  endif()

  add_custom_command(
    OUTPUT            ${CMAKE_CURRENT_BINARY_DIR}/${name}.amx
    COMMAND           ${PawnCC_EXECUTABLE} ${_compile_flags}
    COMMENT           "Compiling test ${name}"
    DEPENDS           ${name}.pwn test.inc
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )

  add_samp_plugin_test(${name}
    TARGET            ${target}
    SCRIPT            ${CMAKE_CURRENT_BINARY_DIR}/${name}
    OUTPUT_FILE       ${CMAKE_CURRENT_BINARY_DIR}/${name}.out
    TIMEOUT           1
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
    list(APPEND _amx_files ${CMAKE_CURRENT_BINARY_DIR}/${name}.amx)
  endforeach()

  add_custom_target(${target}-tests ALL DEPENDS ${_amx_files})
endmacro()

file(STRINGS test.list CRASHDETECT_TESTS)
tests(crashdetect ${CRASHDETECT_TESTS})
