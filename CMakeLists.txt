cmake_minimum_required(VERSION 2.8.6)

project(crashdetect)

list(APPEND CMAKE_MODULE_PATH 
	${crashdetect_SOURCE_DIR}/cmake/Modules)

include(CheckIncludeFile)
include(GetGitRevisionDescription)

check_include_file(alloca.h HAVE_ALLOCA_H)
if(HAVE_ALLOCA_H)
	add_definitions(-DHAVE_ALLOCA_H)
endif()
check_include_file(inttypes.h HAVE_INTTYPES_H)
if(HAVE_INTTYPES_H)
	add_definitions(-DHAVE_INTTYPES_H)
endif()
check_include_file(malloc.h HAVE_MALLOC_H)
if(HAVE_MALLOC_H)
	add_definitions(-DHAVE_MALLOC_H)
endif()
check_include_file(stdint.h HAVE_STDINT_H)
if(HAVE_STDINT_H)
	add_definitions(-DHAVE_STDINT_H)
endif()

find_package(Boost 1.47 REQUIRED COMPONENTS filesystem system)

include_directories(src)
include_directories(src/amx) 
include_directories(${Boost_INCLUDE_DIRS})

# Disable Boost auto-link feature 
add_definitions(-DBOOST_ALL_NO_LIB)

# AMX definitions
add_definitions(
	-DAMX_ALIGN 
	-DAMX_CLEANUP
	-DAMX_EXEC 
	-DAMX_FLAGS
	-DAMX_INIT
	-DAMX_REGISTER 
	-DAMX_SETCALLBACK
	-DAMX_SETDEBUGHOOK
	-DAMX_XXXNATIVES
	-DAMX_XXXPUBLICS
	-DAMX_ANSIONLY
	-DAMX_NODYNALOAD)

set(SOURCES
	src/amx/amx.c
	src/amx/amx.h
	src/amx/amxaux.c
	src/amx/amxaux.h
	src/amx/amxdbg.c
	src/amx/amxdbg.h
	src/amx/getch.h
	src/amx/osdefs.h
	src/amx/sclinux.h
	src/amxcallstack.cpp
	src/amxcallstack.h
	src/amxdebuginfo.cpp
	src/amxdebuginfo.h
	src/amxerror.cpp
	src/amxpathfinder.cpp
	src/amxpathfinder.h
	src/amxutils.cpp
	src/amxutils.h
	src/configreader.cpp
	src/configreader.h
	src/crashdetect.cpp
	src/crashdetect.h
	src/jump-x86.cpp
	src/jump-x86.h
	src/logprintf.cpp
	src/logprintf.h
	src/os.h
	src/plugin.cpp
	src/plugin.rc
	src/plugincommon.h
)

if(WIN32)
	set(SOURCES ${SOURCES}
		src/os-win32.cpp
	)
elseif(UNIX)
	set(SOURCES ${SOURCES}
		src/os-unix.cpp
	)
endif()

add_library(crashdetect MODULE ${SOURCES})

# Don't prefix the output .so with "lib" 
set_property(TARGET crashdetect PROPERTY PREFIX "")

if(UNIX OR MINGW)
	set_property(TARGET crashdetect APPEND_STRING PROPERTY
		COMPILE_FLAGS " -m32 -Wno-attributes")
	set_property(TARGET crashdetect APPEND_STRING PROPERTY
		LINK_FLAGS " -m32 -Wl,--no-undefined")
	if(UNIX)
		add_definitions(-DLINUX)
	endif()
endif()

if(WIN32)
	set(DEF_FILE "${crashdetect_SOURCE_DIR}/src/plugin.def")
	if(MSVC)
		add_definitions(-D_CRT_SECURE_NO_WARNINGS) # Shut up security warnings
		set_property(TARGET crashdetect APPEND_STRING PROPERTY 
			LINK_FLAGS " /DEF:${DEF_FILE}")
	elseif(MINGW)
		set_property(TARGET crashdetect APPEND_STRING PROPERTY 
			LINK_FLAGS " -Wl,--kill-at --def ${DEF_FILE}")
	endif()
endif()

target_link_Libraries(crashdetect ${Boost_LIBRARIES})
if(CMAKE_COMPILER_IS_GNUCXX)
	target_link_Libraries(crashdetect -ldl)
endif()

find_package(Git)
if(GIT_FOUND)
	# Get latest version tag from Git history
	git_describe(description --match v[0-9]*.[0-9]**)
	string(REGEX REPLACE "\\-g[0-9a-f]+$" "" description ${description})
	string(REGEX REPLACE "^v(.*)" "\\1" version ${description})
endif()

if(version STREQUAL "")
	# Read version from VERSION.txt (updated on each release)
	message(STATUS "Failed to get version from Git, will read VERSION.txt")
	file(READ "${crashdetect_SOURCE_DIR}/VERSION.txt" version)
	string(STRIP ${version} version)
endif()

# Comma-separated version for RC
string(REGEX REPLACE "[.-]" "," version_rc ${version})
# Append one or two ",0" to the end to make windres happy
string(REGEX REPLACE "^([0-9]+,[0-9]+)$" "\\1,0,0" version_rc ${version_rc})
string(REGEX REPLACE "^([0-9]+,[0-9]+,[0-9]+)$" "\\1,0" version_rc ${version_rc})

# Generate version.h to be included from source files
file(WRITE "src/version.h" 
"/* This file is auto-generated, do not edit */
#define CRASHDETECT_VERSION \"${version}\"
#define CRASHDETECT_VERSION_RC ${version_rc}
")

message(STATUS "You're going to build crashdetect v${version}")

install(TARGETS crashdetect LIBRARY DESTINATION ".")
install(FILES README.md DESTINATION ".")

set(CPACK_PACKAGE_NAME ${CMAKE_PROJECT_NAME})

if(WIN32)
	set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${version}-win32")
	set(CPACK_GENERATOR ZIP)
elseif(UNIX)
	set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${version}-linux")
	set(CPACK_GENERATOR TGZ)
endif()

include(CPack)

