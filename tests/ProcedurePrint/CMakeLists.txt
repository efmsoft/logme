project(ProcedurePrint)

add_executable(ProcedurePrint
  ProcedurePrint.cpp
)

if(WIN32)
  set(WINDOWS_LIBRARIES Ws2_32.lib)
endif()

target_link_libraries(ProcedurePrint logme gtest_main ${WINDOWS_LIBRARIES})

if(WIN32 AND DEFINED USE_LOGME_SHARED AND USE_LOGME_SHARED)
  add_custom_command(TARGET ProcedurePrint POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:logmed>
      $<TARGET_FILE_DIR:ProcedurePrint>
  )
endif()
target_compile_definitions(${PROJECT_NAME} PRIVATE _LOGME_STATIC_BUILD_)
target_compile_definitions(${PROJECT_NAME} PRIVATE LOGME_INRELEASE)

set_target_properties(ProcedurePrint PROPERTIES FOLDER "Tests")

