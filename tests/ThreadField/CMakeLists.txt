add_executable(ThreadFieldTest
  ThreadField.cpp
)

target_link_libraries(ThreadFieldTest PRIVATE logme gtest_main)

target_compile_definitions(ThreadFieldTest PRIVATE
  LOGME_INRELEASE
  _LOGME_STATIC_BUILD_
)

if (WIN32)
  target_link_libraries(ThreadFieldTest PRIVATE ws2_32)
endif()

set_target_properties(ThreadFieldTest PROPERTIES FOLDER "Tests")
add_test(NAME ThreadField.JsonIncludesThreadFieldsFromMacro COMMAND ThreadFieldTest --gtest_filter=ThreadField.JsonIncludesThreadFieldsFromMacro)
add_test(NAME ThreadField.XmlIncludesThreadFields COMMAND ThreadFieldTest --gtest_filter=ThreadField.XmlIncludesThreadFields)
add_test(NAME ThreadField.TextIgnoresThreadFields COMMAND ThreadFieldTest --gtest_filter=ThreadField.TextIgnoresThreadFields)
add_test(NAME ThreadField.ThreadFieldsMacroRestoresPreviousFields COMMAND ThreadFieldTest --gtest_filter=ThreadField.ThreadFieldsMacroRestoresPreviousFields)
add_test(NAME ThreadField.RemoveAndClearThreadFields COMMAND ThreadFieldTest --gtest_filter=ThreadField.RemoveAndClearThreadFields)
