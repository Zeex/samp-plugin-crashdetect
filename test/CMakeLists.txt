# NOTE #1: Every .amx file must have a corresponding .out file!
#
# NOTE #2: You must have samp-server-cli (and samp-server-cli.bat on Windows)
#          in your PATH to be able to run the tests. Get it here:
#
#          https://github.com/Zeex/samp-server-cli
#
# NOTE #3: There must be an environment variable SAMP_SERVER_ROOT pointing to
#          the server's root directory because otherwise the samp-server-cli
#          script will not run.
#
# NOTE #4: Before running the tests you must copy the plugin to the server's
#          %SAMP_SERVER_ROOT%/plugins. To semi-automate this you can make
#          CMAKE_INSTALL_PREFIX point to %SAMP_SERVER_ROOT%/plugins and build
#          the install target.
#
# NOTE #5: If some test do not complete in time because of slow server startup
#          increase the timeout value (--timeout or -T option).

include(CMakeParseArguments)

function(add_plugin_test)
	set(name "${ARGV0}")

	set(options "NAME" "OUTFILE" "SCRIPT" "TIMEOUT")
	cmake_parse_arguments(ARG "" "${options}" ${ARGN})

	if(NOT ARG_OUTFILE)
		set(ARG_OUTFILE "${CMAKE_CURRENT_SOURCE_DIR}/${name}.out")
	endif()
	if(NOT ARG_SCRIPT)
		set(ARG_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/${name}")
	endif()
	if(NOT ARG_TIMEOUT)
		set(ARG_TIMEOUT "0.5")
	endif()

	list(APPEND arguments
		"--output"
		"--timeout" "${ARG_TIMEOUT}"
		"--plugin" ${CMAKE_PROJECT_NAME}
		"--gamemode" "${ARG_SCRIPT}"
	)

	set(config "${CMAKE_CURRENT_SOURCE_DIR}/${name}.cfg")
	if(EXISTS ${config})
		list(APPEND arguments "--exec" "${config}")
	endif()

	file(READ "${ARG_OUTFILE}" out)

	add_test(${name} "samp-server-cli" ${arguments})
	set_tests_properties(${name} PROPERTIES
		ENVIRONMENT "AMX_PATH=${CMAKE_CURRENT_SOURCE_DIR}"
		PASS_REGULAR_EXPRESSION "${out}"
	)
endfunction()

add_plugin_test("args")
add_plugin_test("bounds")
add_plugin_test("crash")
add_plugin_test("invinstr")
add_plugin_test("not_found")
add_plugin_test("stacklow")
