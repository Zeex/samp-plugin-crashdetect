# NOTE #1: You must have samp-server-cli (and samp-server-cli.bat on Windows)
#          in your PATH to be able to run the tests. Get it here:
#
#          https://github.com/Zeex/samp-server-cli
#
# NOTE #2: There must be an environment variable SAMP_SERVER_ROOT pointing to
#          the server's root directory because otherwise the samp-server-cli
#          script will not run.
#
# NOTE #3: If some tests do not complete in time because of slow server startup
#          consider increasing the timeout (TIMEOUT argument). Default timeout
#          is 0.5 sec.

include(CMakeParseArguments)

function(add_samp_plugin_test)
	set(name "${ARGV0}")

	set(options "TARGET" "OUT_FILE" "SCRIPT" "TIMEOUT")
	cmake_parse_arguments(ARG "" "${options}" ${ARGN})

	if(NOT ARG_TARGET)
		set(ARG_TARGET ${PROJECT_NAME})
	endif()
	if(NOT ARG_OUT_FILE)
		set(ARG_OUT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${name}.out")
	endif()
	if(NOT ARG_SCRIPT)
		set(ARG_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/${name}")
	endif()
	if(NOT ARG_TIMEOUT)
		set(ARG_TIMEOUT "0.5")
	endif()

	get_target_property(PLUGIN_PATH ${ARG_TARGET} LOCATION)

	list(APPEND arguments
		"--output"
		"--timeout"  "${ARG_TIMEOUT}"
		"--plugin"   "${PLUGIN_PATH}"
		"--gamemode" "${ARG_SCRIPT}"
	)

	set(config "${CMAKE_CURRENT_SOURCE_DIR}/${name}.cfg")
	if(EXISTS ${config})
		list(APPEND arguments "--exec" "${config}")
	endif()

	file(READ "${ARG_OUT_FILE}" out)

	add_test(${name} "samp-server-cli" ${arguments})
	set_tests_properties(${name} PROPERTIES
		ENVIRONMENT "AMX_PATH=${CMAKE_CURRENT_SOURCE_DIR}"
		PASS_REGULAR_EXPRESSION "${out}"
	)
endfunction()
