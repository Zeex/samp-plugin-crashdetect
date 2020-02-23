# AddSAMPPluginTestPR - add tests for SA-MP plugins using plugin-runner.
#
# To use this module you will also need to have FindPluginRunner in your CMake
# module path.

include(CMakeParseArguments)

function(add_samp_plugin_test)
  set(name "${ARGV0}")

  cmake_parse_arguments(
    ARG
    ""
    "OUTPUT_FILE;SCRIPT;TIMEOUT;CONFIG;WORKING_DIRECTORY"
    "TARGETS"
    ${ARGN}
  )

  find_package(PluginRunner REQUIRED)
  set(command ${PluginRunner_EXECUTABLE})

  if(NOT ARG_SCRIPT)
    message(FATAL_ERROR "SCRIPT argument is required")
  endif()

  set(plugins "")
  foreach(target ${ARG_TARGETS})
    list(APPEND plugins $<TARGET_FILE:${target}>)
  endforeach()

  add_test(NAME ${name} COMMAND ${command} ${plugins} ${ARG_SCRIPT})

  get_filename_component(AMX_PATH ${ARG_SCRIPT} DIRECTORY)
  set_tests_properties(${name} PROPERTIES ENVIRONMENT AMX_PATH=${AMX_PATH})

  if(ARG_OUTPUT_FILE)
    file(READ ${ARG_OUTPUT_FILE} output)
    set_tests_properties(${name} PROPERTIES PASS_REGULAR_EXPRESSION ${output})
  endif()

  if(ARG_TIMEOUT)
    set_tests_properties(${name} PROPERTIES TIMEOUT ${ARG_TIMEOUT})
  endif()

  if(ARG_WORKING_DIRECTORY)
    set_tests_properties(${name} PROPERTIES
                         WORKING_DIRECTORY ${ARG_WORKING_DIRECTORY})
  endif()
endfunction()
