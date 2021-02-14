add_library(pymacro SHARED PyMacro.cpp)
target_link_libraries(pymacro parser format)
