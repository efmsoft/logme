project(Multithread)

add_executable(${PROJECT_NAME} Multithread.cpp)

find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} LINK_PUBLIC Threads::Threads)

target_link_libraries(${PROJECT_NAME} LINK_PUBLIC logme)
target_compile_definitions(${PROJECT_NAME} PRIVATE _LOGME_STATIC_BUILD_ LOGME_INRELEASE)
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Examples")
