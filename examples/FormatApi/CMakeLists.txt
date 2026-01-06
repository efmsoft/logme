project(FormatApi)

add_executable(${PROJECT_NAME} FormatApi.cpp)

target_link_libraries(${PROJECT_NAME} LINK_PUBLIC logme)
target_compile_definitions(${PROJECT_NAME} PRIVATE _LOGME_STATIC_BUILD_ LOGME_INRELEASE)
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Examples")
