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

	set(options "TARGET" "OUT_FILE" "SCRIPT" "TIMEOUT" "EXEC")
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
	if(NOT ARG_EXEC)
		set(ARG_EXEC "${CMAKE_CURRENT_SOURCE_DIR}/${name}.cfg")
	endif()
	if(ARG_TIMEOUT)
		list(APPEND arguments "--timeout" "${ARG_TIMEOUT}")
	endif()
	if(EXISTS "${ARG_EXEC}")
		list(APPEND arguments "--exec" "${ARG_EXEC}")
	endif()

	get_target_property(PLUGIN_PATH ${ARG_TARGET} LOCATION)

	list(APPEND arguments
		"--output"
		"--plugin"   "${PLUGIN_PATH}"
		"--gamemode" "${ARG_SCRIPT}"
	)

	file(READ "${ARG_OUT_FILE}" out)

	add_test(${name} "samp-server-cli" ${arguments})
	set_tests_properties(${name} PROPERTIES
		ENVIRONMENT "AMX_PATH=${CMAKE_CURRENT_SOURCE_DIR}/${name}"
		PASS_REGULAR_EXPRESSION "${out}"
	)
endfunction()
