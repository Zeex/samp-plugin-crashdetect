include(SampPluginTest)

macro(test)
	add_samp_plugin_test(${ARGN} TARGET ${PROJECT_NAME})
endmacro()

test("args")
test("bounds")
test("crash")
test("invinstr")
test("not_found")
test("stacklow")
