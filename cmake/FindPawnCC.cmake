include(FindPackageHandleStandardArgs)

if(WIN32)
  set(_PawnCC_EXECUTABLE_NAME pawncc.exe)
else()
  set(_PawnCC_EXECUTABLE_NAME pawncc)
endif()

find_file(PawnCC_EXECUTABLE ${_PawnCC_EXECUTABLE_NAME})

mark_as_advanced(PawnCC_EXECUTABLE)

find_package_handle_standard_args(PawnCC
  FOUND_VAR PawnCC_FOUND
  REQUIRED_VARS PawnCC_EXECUTABLE
)
