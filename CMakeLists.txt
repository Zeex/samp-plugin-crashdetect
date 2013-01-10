cmake_minimum_required(VERSION 2.8.6)
project(crashdetect)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	include("cmake/GlobalBuildConfig.cmake")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")

include(AmxConfig)
include(CheckFunctionExists)
include(CTest)
include(GetGitRevisionDescription)
include(SampPlugin)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/amx")

set(SOURCES
	"src/amxdebuginfo.cpp"
	"src/amxdebuginfo.h"
	"src/amxpathfinder.cpp"
	"src/amxpathfinder.h"
	"src/amxstacktrace.cpp"
	"src/amxstacktrace.h"
	"src/amxutils.cpp"
	"src/amxutils.h"
	"src/configreader.cpp"
	"src/configreader.h"
	"src/compiler.h"
	"src/crashdetect.cpp"
	"src/crashdetect.h"
	"src/fileutils.cpp"
	"src/fileutils.h"
	"src/hook.cpp"
	"src/hook.h"
	"src/logprintf.cpp"
	"src/logprintf.h"
	"src/npcall.cpp"
	"src/npcall.h"
	"src/os.h"
	"src/plugin.cpp"
	"src/plugin.def"
	"src/plugin.rc"
	"src/plugincommon.h"
	"src/stacktrace.cpp"
	"src/stacktrace.h"
	"src/stacktrace-manual.cpp"
	"src/stacktrace-manual.h"
	"src/version.h"
)

if(WIN32)
	list(APPEND SOURCES
		"src/fileutils-win32.cpp"
		"src/hook-win32.cpp"
		"src/os-win32.cpp"
		"src/stacktrace-win32.cpp"
	)
elseif(UNIX)
	list(APPEND SOURCES
		"src/fileutils-unix.cpp"
		"src/hook-unix.cpp"
		"src/os-unix.cpp"
		"src/stacktrace-unix.cpp"
	)
else()
	message(FATAL_ERROR "Unsupported operating system.")
endif()

if(MSVC)
	list(APPEND SOURCES "src/compiler-msvc.cpp")
elseif(CMAKE_COMPILER_IS_GNUCXX)
	list(APPEND SOURCES "src/compiler-gcc.cpp")
else()
	message(FATAL_ERROR "Unsupported compiler.")
endif()

add_samp_plugin(${PROJECT_NAME} ${SOURCES})

if(UNIX OR MINGW)
	set_property(TARGET ${PROJECT_NAME} PROPERTY PREFIX "")
endif()

if(WIN32 AND MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()	

if(UNIX)
	check_function_exists(backtrace HAVE_BACKTRACE)
	if(HAVE_BACKTRACE)
		add_definitions(-DHAVE_BACKTRACE)
	endif()
	check_function_exists(backtrace_symbols HAVE_BACKTRACE_SYMBOLS)
	if(HAVE_BACKTRACE_SYMBOLS)
		add_definitions(-DHAVE_BACKTRACE_SYMBOLS)
	endif()
	add_definitions(-DLINUX)
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
	set_property(TARGET ${PROJECT_NAME} APPEND_STRING PROPERTY
		COMPILE_FLAGS " -Wno-attributes -Wno-ignored-attributes")
endif()

target_link_Libraries(${PROJECT_NAME} amx)

git_describe(description --match v[0-9]*.[0-9]**)
if(description)
	string(REGEX REPLACE "\\-g[0-9a-f]+$" "" description ${description})
	string(REGEX REPLACE "^v(.*)" "\\1" version ${description})
else()
	message(STATUS "Failed geting version from Git, will read VERSION.txt.")
	file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION.txt" version)
	string(STRIP "${version}" version)
endif()

string(REGEX REPLACE "[.-]" "," version_rc ${version})
string(REGEX REPLACE "^([0-9]+,[0-9]+)$" "\\1,0,0" version_rc ${version_rc})
string(REGEX REPLACE "^([0-9]+,[0-9]+,[0-9]+)$" "\\1,0" version_rc ${version_rc})

set(PROJECT_VERSION    "${version}")
set(PROJECT_VERSION_RC "${version_rc}")

add_subdirectory("amx")
add_subdirectory("src")
add_subdirectory("tests")

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX})

if(MSVC)
	set(PDB_NAME "${PROJECT_NAME}.pdb")
	set(PDB_PATH "${CMAKE_CURRENT_BINARY_DIR}/\${CMAKE_INSTALL_CONFIG_NAME}/${PDB_NAME}")
	install(FILES ${PDB_PATH} DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()

set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
if(WIN32)
	set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${version}-win32")
	set(CPACK_GENERATOR ZIP)
elseif(UNIX)
	set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${version}-linux")
	set(CPACK_GENERATOR TGZ)
endif()
include(CPack)

message(STATUS "You're going to build ${PROJECT_NAME} v${PROJECT_VERSION}")
