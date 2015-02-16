# AddSAMPPluginTest - add tests for SA-MP plugins.
#
# This module reuires the samp-server-cli script to be present in PATH in
# order to be able to run the tests. The script can be downloaded here:
#
#   https://github.com/Zeex/samp-server-cli
#
# Additionally the SAMP_SERVER_ROOT environment variable must be defined and
# must point to the SA-MP server's root directory.
#
# Copyright (c) 2014-2015 Zeex
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

include(CMakeParseArguments)

function(add_samp_plugin_test)
  set(name "${ARGV0}")

  set(options TARGET OUTPUT_FILE SCRIPT TIMEOUT CONFIG WORKING_DIRECTORY)
  cmake_parse_arguments(ARG "" "${options}" "" ${ARGN})

  if(ARG_SCRIPT)
    list(APPEND args --gamemode ${ARG_SCRIPT})
  endif()

  if(ARG_TIMEOUT)
    list(APPEND args --timeout ${ARG_TIMEOUT})
  endif()

  if(ARG_WORKING_DIRECTORY)
    list(APPEND args --workdir ${ARG_WORKING_DIRECTORY})
  endif()

  if(WIN32)
    set(command samp-server-cli.bat)
  else()
    set(command samp-server-cli)
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
