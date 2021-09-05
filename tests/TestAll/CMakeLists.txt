add_executable(TestAll TestAll.cpp) 
target_link_libraries (TestAll LINK_PUBLIC logme) 
set_target_properties(TestAll PROPERTIES FOLDER "Tests")
