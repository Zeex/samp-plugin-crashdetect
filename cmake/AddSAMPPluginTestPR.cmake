# AddSAMPPluginTestPR - add tests for SA-MP plugins using plugin-runner.
#
# To use this module you will also need to have FindPluginRunner in your CMake
# module path.

include(CMakeParseArguments)

function(add_samp_plugin_test)
  set(name "${ARGV0}")

  set(options TARGET OUTPUT_FILE SCRIPT TIMEOUT CONFIG WORKING_DIRECTORY)
  cmake_parse_arguments(ARG "" "${options}" "" ${ARGN})

  find_package(PluginRunner REQUIRED)
  set(command ${PluginRunner_EXECUTABLE})

  if(NOT ARG_SCRIPT)
    message(FATAL_ERROR "SCRIPT argument is required")
  endif()

  add_test(NAME ${name} COMMAND ${command}
           $<TARGET_FILE:${ARG_TARGET}> ${ARG_SCRIPT})

  get_filename_component(AMX_PATH ${ARG_SCRIPT} DIRECTORY)
  set_tests_properties(${name} PROPERTIES ENVIRONMENT AMX_PATH=${AMX_PATH})

  if(ARG_OUTPUT_FILE)
    file(READ ${ARG_OUTPUT_FILE} output)
    set_tests_properties(${name} PROPERTIES PASS_REGULAR_EXPRESSION ${output})
  endif()
endfunction()
