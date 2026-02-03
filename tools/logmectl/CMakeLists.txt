add_executable(logmectl
  main.cpp
)

set_target_properties(logmectl PROPERTIES
  FOLDER "Tools"
)

target_link_libraries(logmectl PRIVATE ${LOGME_LINK_TARGET})
LogmeCopyRuntime(logmectl)

if(LOGME_LINK_TARGET STREQUAL "logme")
  target_compile_definitions(logmectl PRIVATE _LOGME_STATIC_BUILD_)
endif()

if (WIN32)
  target_link_libraries(logmectl PRIVATE Ws2_32)
endif()

if (UNIX AND NOT APPLE)
  target_link_libraries(logmectl PRIVATE dl)
endif()
