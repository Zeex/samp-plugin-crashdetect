include(PluginTest)

macro(prepare_test name)
  file(READ ${name}/output.txt TEST_OUTPUT)
  string(STRIP "${TEST_OUTPUT}" TEST_OUTPUT)
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/full_output.txt.in
    ${CMAKE_CURRENT_SOURCE_DIR}/${name}/full_output.txt
  )
endmacro()

macro(test name)
  prepare_test(${name})
  add_plugin_test(${name}
    TARGET   crashdetect
    SCRIPT   ${CMAKE_CURRENT_SOURCE_DIR}/${name}/test
    OUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${name}/full_output.txt
    TIMEOUT  0.5
  )
endmacro()

test(misc/args)
#test(misc/bad_frame)
#test(misc/crash)
test(misc/presence)
test(misc/ref_args)
test(misc/states)

test(errors/bounds)
#test(errors/invinstr)
#test(errors/notfound)
#test(errors/stacklow)

test(bugs/orte_backtrace)
