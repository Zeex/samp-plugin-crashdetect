cmake_minimum_required(VERSION 2.8.6)

project(crashdetect)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	include("cmake/GlobalBuildConfig.cmake")
endif()

list(APPEND CMAKE_MODULE_PATH "${crashdetect_SOURCE_DIR}/cmake/Modules")

include(AmxConfig)
include(CheckFunctionExists)
include(GetGitRevisionDescription)

if(UNIX)
	check_function_exists(backtrace HAVE_BACKTRACE)
	if(HAVE_BACKTRACE)
		add_definitions(-DHAVE_BACKTRACE)
	endif()
	check_function_exists(backtrace_symbols HAVE_BACKTRACE_SYMBOLS)
	if(HAVE_BACKTRACE_SYMBOLS)
		add_definitions(-DHAVE_BACKTRACE_SYMBOLS)
	endif()
endif()

include_directories(".")
include_directories("amx")

add_subdirectory("amx")

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

add_library(crashdetect MODULE ${SOURCES})

# Don't prefix output file with "lib".
set_property(TARGET crashdetect PROPERTY PREFIX "")

if(WIN32)
	set(DEF_FILE "${crashdetect_SOURCE_DIR}/src/plugin.def")
	if(MSVC)
		add_definitions(-D_CRT_SECURE_NO_WARNINGS)
		set_property(TARGET crashdetect APPEND_STRING PROPERTY
			COMPILE_FLAGS " /Oy-")
 		set_property(TARGET crashdetect APPEND_STRING PROPERTY 
			LINK_FLAGS " /DEF:${DEF_FILE}")
	elseif(MINGW)
		set_property(TARGET crashdetect APPEND_STRING PROPERTY 
			LINK_FLAGS " -Wl,--kill-at --def ${DEF_FILE}")
	endif()
elseif(UNIX)
	add_definitions(-DLINUX)
	if(CMAKE_COMPILER_IS_GNUCXX)
		list(APPEND LIBRARIES "-ldl")
	endif()
endif()

list(APPEND LIBRARIES "amx")
target_link_Libraries(crashdetect ${LIBRARIES})

# Get latest version tag from Git.
git_describe(description --match v[0-9]*.[0-9]**)
if(description)
	string(REGEX REPLACE "\\-g[0-9a-f]+$" "" description ${description})
	string(REGEX REPLACE "^v(.*)" "\\1" version ${description})
else()
	message(STATUS "Failed geting version from Git, will read VERSION.txt.")
	file(READ "${crashdetect_SOURCE_DIR}/VERSION.txt" version)
	string(STRIP ${version} version)
endif()

# Convert version to a comma separated list for RC.
string(REGEX REPLACE "[.-]" "," version_rc ${version})
string(REGEX REPLACE "^([0-9]+,[0-9]+)$" "\\1,0,0" version_rc ${version_rc})
string(REGEX REPLACE "^([0-9]+,[0-9]+,[0-9]+)$" "\\1,0" version_rc ${version_rc})

set(PLUGIN_NAME ${CMAKE_PROJECT_NAME})
set(PLUGIN_DESCRIPTION "crashdetect plugin for SA-MP server")
set(PLUGIN_COPYRIGHT "Copyright (c) 2011-2012, Zeex")
set(PLUGIN_VERSION ${version})
set(PLUGIN_VERSION_RC ${version_rc})

configure_file(
	"${crashdetect_SOURCE_DIR}/src/plugin.rc.in"
	"${crashdetect_SOURCE_DIR}/src/plugin.rc"
	@ONLY
)
configure_file(
	"${crashdetect_SOURCE_DIR}/src/version.h.in"
	"${crashdetect_SOURCE_DIR}/src/version.h"
	@ONLY
)

message(STATUS "You're going to build crashdetect v${version}")

install(TARGETS crashdetect LIBRARY DESTINATION ".")

if(MSVC)
	# Install the PDB file. This works only for Debug/RelWithDebInfo.
	set(PDB_NAME "crashdetect.pdb")
	set(PDB_PATH "${CMAKE_CURRENT_BINARY_DIR}/\${CMAKE_INSTALL_CONFIG_NAME}/${PDB_NAME}")
	install(FILES ${PDB_PATH} DESTINATION ".")
endif()

install(FILES "LICENSE.txt" "README.txt" DESTINATION ".")

set(CPACK_PACKAGE_NAME ${CMAKE_PROJECT_NAME})
if(WIN32)
	set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${version}-win32")
	set(CPACK_GENERATOR ZIP)
elseif(UNIX)
	set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${version}-linux")
	set(CPACK_GENERATOR TGZ)
endif()
include(CPack)

