cmake_minimum_required(VERSION 2.8.6)

project(crashdetect)

list(APPEND CMAKE_MODULE_PATH ${crashdetect_SOURCE_DIR}/cmake/Modules)

include(CheckIncludeFile)
include(CheckIncludeFileCXX)
include(GetGitRevisionDescription)

function(check_include_and_define include_file define)
	check_include_file(${include_file} HAVE_${include_file})
	if(HAVE_${include_file})
		add_definitions(-D${define})
	endif()
endfunction()

function(check_cxx_include_and_define include_file define)
	check_include_file_cxx(${include_file} HAVE_${include_file})
	if(HAVE_${include_file})
		add_definitions(-D${define})
	endif()
endfunction()

check_include_and_define(alloca.h HAVE_ALLOCA_H)
check_include_and_define(inttypes.h HAVE_INTTYPES_H)
check_include_and_define(malloc.h HAVE_MALLOC_H)
check_include_and_define(stdint.h HAVE_STDINT_H)

check_cxx_include_and_define(memory HAVE_MEMORY)
check_cxx_include_and_define(tr1/memory HAVE_TR1_MEMORY)

find_package(Boost 1.47 REQUIRED COMPONENTS filesystem system)

include_directories(lib)
include_directories(lib/amx) 
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

add_library(crashdetect MODULE
	lib/amx/amx.c
	lib/amx/amx.h
	lib/amx/amxaux.c
	lib/amx/amxaux.h
	lib/amx/amxdbg.c
	lib/amx/amxdbg.h
	lib/amx/getch.h
	lib/amx/osdefs.h
	lib/amx/sclinux.h
	lib/plugincommon.h
	src/amxcallstack.cpp
	src/amxcallstack.h
	src/amxdebuginfo.cpp
	src/amxdebuginfo.h
	src/amxpathfinder.cpp
	src/amxpathfinder.h
	src/crash.cpp
	src/crash.h
	src/crashdetect.cpp
	src/crashdetect.h
	src/crashdetect.rc
	src/crashdetect.rc
	src/jump-x86.cpp
	src/jump-x86.h
)

# Don't prefix the output .so with "lib" 
set_property(TARGET crashdetect PROPERTY PREFIX "")

if(UNIX OR MINGW)
	set_property(TARGET crashdetect APPEND_STRING PROPERTY
		COMPILE_FLAGS " -m32 -Wno-attributes")
	set_property(TARGET crashdetect APPEND_STRING PROPERTY
		LINK_FLAGS " -m32 -Wl,--no-undefined -ldl")
endif()

if(UNIX)
	add_definitions(-DLINUX)
endif()

if(WIN32)
	add_definitions(-DWIN32 -D_WIN32)
	set(DEF_FILE "${crashdetect_SOURCE_DIR}/src/crashdetect.def")
	if(MSVC)
		set_property(TARGET crashdetect APPEND_STRING PROPERTY 
			LINK_FLAGS " /DEF:${DEF_FILE}")
	elseif(MINGW)
		set_property(TARGET crashdetect APPEND_STRING PROPERTY 
			LINK_FLAGS " -Wl,--kill-at --def ${DEF_FILE}")
	endif()
endif()

target_link_Libraries(crashdetect ${Boost_LIBRARIES})

set(version_tag_regex "v([0-9]+)\\.([0-9]+)\\.([0-9]+)-?([0-9]*)-?g?([a-f0-9]*)")

git_describe(description --match v[0-9]*.[0-9]*.[0-9]*)
if(description MATCHES ${version_tag_regex})
	string(REGEX REPLACE ${version_tag_regex} 
		"\\1;\\2;\\3;\\4;\\5" version_components ${description})
	list(GET version_components 0 version_major)
	list(GET version_components 1 version_minor)
	list(GET version_components 2 version_patch)
	list(GET version_components 3 version_commit)
	if(version_commit STREQUAL "")
		set(version_commit 0)
		set(dotted_version ${version_major}.${version_minor}.${version_patch})
	else()
		set(dotted_version ${version_major}.${version_minor}.${version_patch}.${version_commit})
	endif()
else()
	message(WARNING "Couldn't obtain version info")
	set(version_major  0)
	set(version_minor  0)
	set(version_patch  0)
	set(version_commit 0)
	set(dotted_version "0.0.0")
endif()

file(WRITE "src/version.h" 
"/* This file is auto-generated, do not edit */
#ifndef SAMP_PROFILER_VERSION_H
#define SAMP_PROFILER_VERSION_H

#define VERSION_MAJOR ${version_major}
#define VERSION_MINOR ${version_minor}
#define VERSION_PATCH ${version_patch}
#define VERSION_COMMIT ${version_commit}
#define VERSION_STRING \"${dotted_version}\"

#endif
")

message(STATUS "You're going to build crashdetect v${dotted_version}")

install(TARGETS crashdetect LIBRARY DESTINATION ".")
install(FILES README.md DESTINATION ".")

if(MSVC)
	set(PDB_NAME "crashdetect.pdb")
	set(PDB_PATH "${CMAKE_CURRENT_BINARY_DIR}/\${CMAKE_INSTALL_CONFIG_NAME}/${PDB_NAME}")
	install(FILES ${PDB_PATH} DESTINATION ".")
endif()
set(CPACK_PACKAGE_NAME ${CMAKE_PROJECT_NAME})
if(WIN32)
	set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${dotted_version}-win32")
	set(CPACK_GENERATOR ZIP)
elseif(UNIX)
	set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${dotted_version}-linux")
	set(CPACK_GENERATOR TGZ)
endif()
include(CPack)

