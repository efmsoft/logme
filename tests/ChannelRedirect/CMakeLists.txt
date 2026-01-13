project(ChannelRedirect)

add_executable(${PROJECT_NAME} ChannelRedirect.cpp) 

set(logme_target logme)
if(DEFINED USE_LOGME_SHARED AND USE_LOGME_SHARED)
  set(logme_target logmed)
endif()

if(NOT TARGET ${logme_target})
  message(FATAL_ERROR "Requested ${logme_target} but it is not built. Check LOGME_BUILD_STATIC/LOGME_BUILD_DYNAMIC and USE_LOGME_SHARED.")
endif()


if(WIN32)
  set(WINDOWS_LIBRARIES Ws2_32.lib)
endif()

target_link_libraries(${PROJECT_NAME} LINK_PUBLIC ${logme_target} gtest_main ${WINDOWS_LIBRARIES})
if(NOT (DEFINED USE_LOGME_SHARED AND USE_LOGME_SHARED))
  target_compile_definitions(${PROJECT_NAME} PRIVATE
    _LOGME_STATIC_BUILD_
  )
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE LOGME_INRELEASE)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Tests")
