add_executable(logmectl
  main.cpp
)

set_target_properties(logmectl PROPERTIES
  FOLDER "Tools"
)

target_link_libraries(logmectl PRIVATE logme)

if (WIN32)
  target_link_libraries(logmectl PRIVATE Ws2_32)
endif()
