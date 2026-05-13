project(CApiTest C)

if(MSVC)
  get_directory_property(DirCompileOptions COMPILE_OPTIONS)
  if(DirCompileOptions)
    list(REMOVE_ITEM DirCompileOptions "/Zc:preprocessor")
    set_directory_properties(PROPERTIES COMPILE_OPTIONS "${DirCompileOptions}")
  endif()
endif()

add_executable(${PROJECT_NAME} CApiTest.c)

if(LOGME_LINK_TARGET STREQUAL "logme")
  target_compile_definitions(${PROJECT_NAME} PRIVATE _LOGME_STATIC_BUILD_)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE LOGME_INRELEASE)

if(MSVC)
  target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
  target_compile_options(${PROJECT_NAME} PRIVATE /TC)
endif()
target_link_libraries(${PROJECT_NAME} LINK_PUBLIC ${LOGME_LINK_TARGET})

if(WIN32)
  target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
endif()
LogmeCopyRuntime(${PROJECT_NAME})

set_target_properties(${PROJECT_NAME} PROPERTIES
  FOLDER "Tests"
  LINKER_LANGUAGE CXX
)

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
