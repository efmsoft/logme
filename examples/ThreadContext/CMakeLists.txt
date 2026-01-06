project(ThreadContext)

add_executable(${PROJECT_NAME} ThreadContext.cpp)

target_link_libraries(${PROJECT_NAME} LINK_PUBLIC logme)

if (WIN32)
  target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE _LOGME_STATIC_BUILD_ LOGME_INRELEASE)
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Examples")

set_target_properties(${PROJECT_NAME} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Examples"
)
