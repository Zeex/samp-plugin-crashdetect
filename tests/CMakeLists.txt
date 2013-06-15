include(SampPluginTest)

macro(test name)
	file(READ "${name}/output.txt" TEST_OUTPUT)
	string(STRIP "${TEST_OUTPUT}" TEST_OUTPUT)
	configure_file(
		"${CMAKE_CURRENT_SOURCE_DIR}/full_output.txt.in"
		"${CMAKE_CURRENT_SOURCE_DIR}/${name}/full_output.txt"
	)
	add_samp_plugin_test("${name}"
		TARGET   "${PROJECT_NAME}"
		SCRIPT   "${CMAKE_CURRENT_SOURCE_DIR}/${name}/test"
		OUT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${name}/full_output.txt"
		EXEC     "${CMAKE_CURRENT_SOURCE_DIR}/server.cfg"
		TIMEOUT  "0.5"
	)
endmacro()

test("misc/args")
#test("misc/bad_frame")
#test("misc/crash")
test("misc/states")

test("error/bounds")
#test("error/invinstr")
#test("error/notfound")
#test("error/stacklow")
