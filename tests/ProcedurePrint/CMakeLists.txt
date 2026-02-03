project(ProcedurePrint)

add_executable(ProcedurePrint
  ProcedurePrint.cpp
)

if(LOGME_LINK_TARGET STREQUAL "logme")
  target_compile_definitions(${PROJECT_NAME} PRIVATE _LOGME_STATIC_BUILD_)
endif()


if(WIN32)
  set(WINDOWS_LIBRARIES Ws2_32.lib)
endif()

target_link_libraries(ProcedurePrint ${LOGME_LINK_TARGET} gtest_main ${WINDOWS_LIBRARIES})
LogmeCopyRuntime(ProcedurePrint)

target_compile_definitions(${PROJECT_NAME} PRIVATE LOGME_INRELEASE)

set_target_properties(ProcedurePrint PROPERTIES FOLDER "Tests")
