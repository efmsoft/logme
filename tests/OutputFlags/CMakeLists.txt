add_executable(OutputFlags OutputFlags.cpp) 
target_link_libraries(OutputFlags LINK_PUBLIC logme gtest_main) 

include(GoogleTest)
gtest_discover_tests(OutputFlags)
set_target_properties(OutputFlags PROPERTIES FOLDER "Tests")
