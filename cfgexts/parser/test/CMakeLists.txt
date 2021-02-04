add_library(pymacro SHARED pymacro.cpp)
add_executable(ptest main.cpp pymacro.cpp)  #  ParserThread.cpp)
target_link_libraries(ptest src)
target_link_libraries(pymacro src)
