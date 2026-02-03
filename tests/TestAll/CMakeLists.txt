project(TestAll)
add_executable(${PROJECT_NAME} TestAll.cpp) 

if(LOGME_LINK_TARGET STREQUAL "logme")
  target_compile_definitions(${PROJECT_NAME} PRIVATE _LOGME_STATIC_BUILD_)
endif()

if(WIN32)
  set(WINDOWS_LIBRARIES Ws2_32.lib)
endif()

target_link_libraries(${PROJECT_NAME} LINK_PUBLIC ${LOGME_LINK_TARGET} ${WINDOWS_LIBRARIES}) 
LogmeCopyRuntime(${PROJECT_NAME})

target_compile_definitions(${PROJECT_NAME} PRIVATE LOGME_INRELEASE)

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Tests")
