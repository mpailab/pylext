option(BUILD_CFGEXTS "Build package cfgexts" OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "-m64 -mpopcnt -mavx2 -mlzcnt -mbmi -mbmi2")
	if(WIN32)
		#set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
	endif()
endif()

set(CMAKE_CXX_STANDARD 17)

include_directories(src)
add_subdirectory(src)

if(BUILD_CFGEXTS)

    find_package(PythonExtensions REQUIRED)
    find_package(Cython REQUIRED)

    include_directories(.)

    add_cython_target(wrap.pyx CXX)

    add_library(wrap MODULE
        ${wrap}
        src/apply.cpp
    )
    target_link_libraries(wrap parser)

    python_extension_module(wrap)

    install(TARGETS wrap LIBRARY DESTINATION cfgexts/parser)

else()

    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

    add_subdirectory(test)

endif()
