include(FindPackageHandleStandardArgs)

if(WIN32)
  set(_PawnCompiler_EXECUTABLE_NAME pawncc.exe)
else()
  set(_PawnCompiler_EXECUTABLE_NAME pawncc)
endif()

find_file(PawnCompiler_EXECUTABLE ${_PawnCompiler_EXECUTABLE_NAME})

find_package_handle_standard_args(PawnCompiler
  FOUND_VAR PawnCompiler_FOUND
  REQUIRED_VARS PawnCompiler_EXECUTABLE
)
