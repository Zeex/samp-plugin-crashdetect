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
	set(options "NAME" "OUTFILE" "SCRIPT" "TIMEOUT" "EXEC")
	cmake_parse_arguments(ARG "" "${options}" ${ARGN})

	if(NOT ARG_NAME)
		message(FATAL_ERROR "NAME is not set")
	endif()
	if(NOT ARG_OUTFILE)
		set(ARG_OUTFILE "${CMAKE_CURRENT_SOURCE_DIR}/${ARG_NAME}.out")
	endif()
	if(NOT ARG_SCRIPT)
		message(FATAL_ERROR "SCRIPT is not set")
	endif()
	if(NOT ARG_TIMEOUT)
		set(ARG_TMIEOUT "0.5")
	endif()

	list(APPEND arguments
		"--output"
		"--timeout" "${ARG_TIMEOUT}"
		"--plugin" ${CMAKE_PROJECT_NAME}
		"--gamemode" "${ARG_SCRIPT}"
	)

	if(ARG_EXEC)
		list(APPEND arguments "--exec" "${ARG_EXEC}")
	endif()

	file(READ "${ARG_OUTFILE}" out)

	add_test("${ARG_NAME}" "samp-server-cli" ${arguments})
	set_tests_properties("${ARG_NAME}" PROPERTIES
		ENVIRONMENT "AMX_PATH=${CMAKE_CURRENT_SOURCE_DIR}"
		PASS_REGULAR_EXPRESSION "${out}"
	)
endfunction()

file(GLOB amx_files "*.amx")
foreach(amx IN LISTS amx_files)
	get_filename_component(amx_name ${amx} NAME_WE)
	add_plugin_test(CMAKE_IS_BUGGED
		NAME ${amx_name}
		SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/${amx_name}"
		TIMEOUT 0.5
	)
endforeach()
