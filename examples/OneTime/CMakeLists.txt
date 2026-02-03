project(OneTime)

add_executable(${PROJECT_NAME} OneTime.cpp)

if(LOGME_LINK_TARGET STREQUAL "logme")
  target_compile_definitions(${PROJECT_NAME} PRIVATE _LOGME_STATIC_BUILD_)
endif()

target_link_libraries(${PROJECT_NAME} LINK_PUBLIC ${LOGME_LINK_TARGET})
LogmeCopyRuntime(${PROJECT_NAME})

if (WIN32)
  target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE LOGME_INRELEASE)
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Examples")

set_target_properties(${PROJECT_NAME} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Examples"
)
