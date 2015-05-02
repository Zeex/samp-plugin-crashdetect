# AddSAMPPluginTest - add tests for SA-MP plugins.
#
# This module reuires the samp-server-cli script to be present in PATH in
# order to be able to run the tests. The script can be downloaded here:
#
#   https://github.com/Zeex/samp-server-cli
#
# Additionally the SAMP_SERVER_ROOT environment variable must be defined and
# must point to the SA-MP server's root directory.

include(CMakeParseArguments)

function(add_samp_plugin_test)
  set(name "${ARGV0}")

  set(options TARGET OUTPUT_FILE SCRIPT TIMEOUT CONFIG WORKING_DIRECTORY)
  cmake_parse_arguments(ARG "" "${options}" "" ${ARGN})

  find_package(SAMPServerCLI REQUIRED)
  set(command ${SAMPServerCLI_EXECUTABLE})

  if(ARG_SCRIPT)
    list(APPEND args --gamemode ${ARG_SCRIPT})
  endif()

  if(ARG_TIMEOUT)
    list(APPEND args --timeout ${ARG_TIMEOUT})
  endif()

  if(ARG_WORKING_DIRECTORY)
    list(APPEND args --workdir ${ARG_WORKING_DIRECTORY})
  endif()

  add_test(NAME ${name} COMMAND ${command} ${args} --output
           --plugin $<TARGET_FILE:${ARG_TARGET}>)

  if(ARG_SCRIPT)
    get_filename_component(AMX_PATH ${ARG_SCRIPT} DIRECTORY)
    set_tests_properties(${name} PROPERTIES ENVIRONMENT AMX_PATH=${AMX_PATH})
  endif()

  if(ARG_OUTPUT_FILE)
    file(READ ${ARG_OUTPUT_FILE} output)
    set_tests_properties(${name} PROPERTIES PASS_REGULAR_EXPRESSION ${output})
  endif()
endfunction()
