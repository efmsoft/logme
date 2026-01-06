add_executable(ChannelSetup ChannelSetup.cpp)

target_link_libraries(ChannelSetup PRIVATE logme)

target_compile_definitions(ChannelSetup PRIVATE LOGME_INRELEASE)
