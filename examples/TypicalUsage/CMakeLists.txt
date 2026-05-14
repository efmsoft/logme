add_executable(TypicalUsageOutOfBox
  OutOfBox.cpp
)

target_link_libraries(TypicalUsageOutOfBox PRIVATE ${LOGME_LINK_TARGET})
LogmeCopyRuntime(TypicalUsageOutOfBox)

target_compile_definitions(TypicalUsageOutOfBox PRIVATE
  LOGME_INRELEASE
)

add_executable(TypicalUsageCodeConfiguration
  CodeConfiguration.cpp
)

target_link_libraries(TypicalUsageCodeConfiguration PRIVATE ${LOGME_LINK_TARGET})
LogmeCopyRuntime(TypicalUsageCodeConfiguration)

target_compile_definitions(TypicalUsageCodeConfiguration PRIVATE
  LOGME_INRELEASE
)

add_executable(TypicalUsageConfigFile
  ConfigFile.cpp
)

target_link_libraries(TypicalUsageConfigFile PRIVATE ${LOGME_LINK_TARGET})
LogmeCopyRuntime(TypicalUsageConfigFile)

target_compile_definitions(TypicalUsageConfigFile PRIVATE
  LOGME_INRELEASE
)

if(LOGME_LINK_TARGET STREQUAL "logme")
  target_compile_definitions(TypicalUsageOutOfBox PRIVATE
    _LOGME_STATIC_BUILD_
  )
  target_compile_definitions(TypicalUsageCodeConfiguration PRIVATE
    _LOGME_STATIC_BUILD_
  )
  target_compile_definitions(TypicalUsageConfigFile PRIVATE
    _LOGME_STATIC_BUILD_
  )
endif()

set_target_properties(TypicalUsageOutOfBox PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Examples"
)

set_target_properties(TypicalUsageCodeConfiguration PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Examples"
)

set_target_properties(TypicalUsageConfigFile PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Examples"
)

set_target_properties(TypicalUsageOutOfBox PROPERTIES FOLDER "Examples")
set_target_properties(TypicalUsageCodeConfiguration PROPERTIES FOLDER "Examples")
set_target_properties(TypicalUsageConfigFile PROPERTIES FOLDER "Examples")
