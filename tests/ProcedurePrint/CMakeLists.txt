project(ProcedurePrint)

add_executable(ProcedurePrint
  ProcedurePrint.cpp
)

target_link_libraries(ProcedurePrint logme gtest_main)

set_target_properties(ProcedurePrint PROPERTIES FOLDER "Tests")
