add_executable(logmeobf
  main.cpp
)

target_link_libraries(logmeobf PRIVATE ${LOGME_LINK_TARGET})

LogmeCopyRuntime(logmeobf)

set_target_properties(logmeobf PROPERTIES
  FOLDER "Tools"
)
