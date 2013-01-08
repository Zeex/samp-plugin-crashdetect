include(SampPluginTest)

macro(test)
	add_samp_plugin_test(${ARGN} TARGET ${PROJECT_NAME} TIMEOUT 0.5)
endmacro()

test("args")
test("bounds")
test("crash")
test("invinstr")
test("not_found")
test("stacklow")
