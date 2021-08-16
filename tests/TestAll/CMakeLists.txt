include_directories(${CMAKE_SOURCE_DIR}/logme/include)

add_executable(TestAll TestAll.cpp) 
target_link_libraries (TestAll LINK_PUBLIC logme) 
