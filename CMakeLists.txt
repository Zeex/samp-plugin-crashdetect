cmake_minimum_required(VERSION 2.8.6)

project(crashdetect)

list(APPEND CMAKE_MODULE_PATH 
	${crashdetect_SOURCE_DIR}/cmake/Modules)

include(CheckHeader)
include(GetGitRevisionDescription)

check_header(alloca.h)
check_header(inttypes.h)
check_header(malloc.h)
check_header(stdint.h)
check_header_cxx(memory)
check_header_cxx(tr1/memory)

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
	src/interrupt.cpp
	src/interrupt.h
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
	if(UNIX)
		add_definitions(-DLINUX)
	endif()
endif()

if(WIN32)
	add_definitions(-DWIN32 -D_WIN32)
	set(DEF_FILE "${crashdetect_SOURCE_DIR}/src/crashdetect.def")
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

find_package(Git)
if(GIT_FOUND)
	# Get latest version tag from Git history
	git_describe(description --match v[0-9]*.[0-9]*.[0-9]*)
	string(REPLACE "-" "." description ${description})
	string(REGEX REPLACE "\\.g[0-9a-f]+$" "" description ${description})
	string(REGEX REPLACE "^v(.*)" "\\1" version ${description})
	if(NOT version MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+\\.?[0-9]*$")
		set(version "")
	endif()
endif()

if(version STREQUAL "")
	# Read version from VERSION.txt (updated on each release)
	message(STATUS "Failed to get version from Git, will read VERSION.txt")
	file(READ "${crashdetect_SOURCE_DIR}/VERSION.txt" version)
	string(STRIP ${version} version)
	if(NOT version MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
		message(FATAL_ERROR "Invalid version")
	endif()
endif()


# Comma-separated version for RC
string(REPLACE "." "," version_rc ${version})

# Generate version.h to be included from source files
file(WRITE "src/version.h" 
"/* This file is auto-generated, do not edit */
#define CRASHDETECT_VERSION \"${version}\"
#define CRASHDETECT_VERSION_RC ${version_rc}
")

message(STATUS "You're going to build crashdetect v${version}")

install(TARGETS crashdetect LIBRARY DESTINATION ".")
install(FILES README.md DESTINATION ".")

if(MSVC)
	# Copy the .pdb file 
	set(PDB_NAME "crashdetect.pdb")
	set(PDB_PATH "${CMAKE_CURRENT_BINARY_DIR}/\${CMAKE_INSTALL_CONFIG_NAME}/${PDB_NAME}")
	install(FILES ${PDB_PATH} DESTINATION ".")
endif()
set(CPACK_PACKAGE_NAME ${CMAKE_PROJECT_NAME})
if(WIN32)
	set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${version}-win32")
	set(CPACK_GENERATOR ZIP)
elseif(UNIX)
	set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${version}-linux")
	set(CPACK_GENERATOR TGZ)
endif()
include(CPack)

