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

function(add_crashdetect_test name)
	add_test(${name} "samp-server-cli"
		"--output"
		"--timeout" "0.5"
		"--plugin" "crashdetect"
		"--gamemode" "${CMAKE_CURRENT_SOURCE_DIR}/${name}"
	)
	file(READ "${CMAKE_CURRENT_SOURCE_DIR}/${name}.out" out)
	set_tests_properties(${name} PROPERTIES
		ENVIRONMENT "AMX_PATH=${CMAKE_CURRENT_SOURCE_DIR}"
		PASS_REGULAR_EXPRESSION "${out}"
	)
endfunction()

file(GLOB amx_files "*.amx")
foreach(amx IN LISTS amx_files)
	get_filename_component(amx_name ${amx} NAME_WE)
	add_crashdetect_test(${amx_name})
endforeach()
