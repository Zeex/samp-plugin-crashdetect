include(SampPluginTest)

function(add_crashdetect_test)
	add_samp_plugin_test(${ARGN} TARGET crashdetect)
endfunction()

add_crashdetect_test(args)
add_crashdetect_test(bounds)
add_crashdetect_test(crash)
add_crashdetect_test(invinstr)
add_crashdetect_test(not_found)
add_crashdetect_test(stacklow)
