cmake_minimum_required(VERSION 3.8.0)
project(GFG-parser)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
else ()
	if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
		set(CMAKE_CXX_FLAGS "-m64 -mpopcnt -mavx2 -mlzcnt -mbmi -mbmi2")
	else()
		set(CMAKE_CXX_FLAGS "-m64 -mpopcnt -mavx2 -mlzcnt -mbmi -mbmi2 -fcoroutines")
	endif()
	if (WIN32)
		#set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
	endif()
endif ()

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(src/parser)
include_directories(src/format)

add_subdirectory(src)
add_subdirectory(test)
