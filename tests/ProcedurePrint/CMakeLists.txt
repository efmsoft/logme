project(ProcedurePrint)

add_executable(ProcedurePrint
  ProcedurePrint.cpp
)

set(logme_target logme)
if(DEFINED USE_LOGME_SHARED AND USE_LOGME_SHARED)
  set(logme_target logmed)
endif()

if(NOT TARGET ${logme_target})
  message(FATAL_ERROR "Requested ${logme_target} but it is not built. Check LOGME_BUILD_STATIC/LOGME_BUILD_DYNAMIC and USE_LOGME_SHARED.")
endif()

if(NOT (DEFINED USE_LOGME_SHARED AND USE_LOGME_SHARED))
  target_compile_definitions(${PROJECT_NAME} PRIVATE _LOGME_STATIC_BUILD_)
endif()



if(WIN32)
  set(WINDOWS_LIBRARIES Ws2_32.lib)
endif()

target_link_libraries(ProcedurePrint ${logme_target} gtest_main ${WINDOWS_LIBRARIES})
target_compile_definitions(${PROJECT_NAME} PRIVATE LOGME_INRELEASE)

set_target_properties(ProcedurePrint PROPERTIES FOLDER "Tests")
