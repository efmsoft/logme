add_executable(ChannelSetup
  ChannelSetup.cpp
)

set(logme_target logme)
if(DEFINED USE_LOGME_SHARED AND USE_LOGME_SHARED)
  set(logme_target logmed)
endif()

if(NOT TARGET ${logme_target})
  message(FATAL_ERROR "Requested ${logme_target} but it is not built. Check LOGME_BUILD_STATIC/LOGME_BUILD_DYNAMIC and USE_LOGME_SHARED.")
endif()


target_link_libraries(ChannelSetup PRIVATE ${logme_target})

target_compile_definitions(ChannelSetup PRIVATE
  LOGME_INRELEASE
)

if(NOT (DEFINED USE_LOGME_SHARED AND USE_LOGME_SHARED))
  target_compile_definitions(ChannelSetup PRIVATE
    _LOGME_STATIC_BUILD_
  )
endif()

if (WIN32)
  target_link_libraries(ChannelSetup PRIVATE ws2_32)
endif()

set_target_properties(ChannelSetup PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Examples"
)

set_target_properties(ChannelSetup PROPERTIES FOLDER "Examples")
