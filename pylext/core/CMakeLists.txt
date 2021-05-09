option(BUILD_PYLEXT "Build package pylext" OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "-m64 -mpopcnt -mavx2 -mlzcnt -mbmi -mbmi2")
    if(UNIX)
        set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fPIC" )
    endif()
endif()

set(CMAKE_CXX_STANDARD 17)

include_directories(format)
include_directories(parser)
include_directories(pymacro)

add_subdirectory(format)
add_subdirectory(parser)
add_subdirectory(pymacro)

if(BUILD_PYLEXT)

    find_package(PythonExtensions REQUIRED)
    find_package(Cython REQUIRED)

    include_directories(.)

    add_cython_target(parse.pyx CXX)

    add_library(parse MODULE ${parse})
    target_link_libraries(parse pymacro parser format)

    python_extension_module(parse)
    
    install(TARGETS parse LIBRARY DESTINATION pylext/core)

else()

    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

    add_subdirectory(test)

endif()
