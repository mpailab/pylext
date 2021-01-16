add_library(pymacro SHARED base.cpp pymacro.cpp)
add_executable(ptest main.cpp base.cpp pymacro.cpp)  #  ParserThread.cpp)
target_link_libraries(ptest parser format)
target_link_libraries(pymacro parser format)
