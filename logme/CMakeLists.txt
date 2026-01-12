file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "source/*.cpp")
file(GLOB_RECURSE HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.h")
add_library(logme STATIC ${SOURCES} ${HEADERS}) 


target_include_directories(logme PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_include_directories(logme PRIVATE
  ${ALLSTAT_INCLUDE_DIR}
)

target_compile_definitions(logme PRIVATE _WINSOCK_DEPRECATED_NO_WARNINGS _CRT_SECURE_NO_WARNINGS _LOGME_STATIC_BUILD_)
if (LOGME_DISABLE_STD_FORMAT)
  target_compile_definitions(logme PUBLIC LOGME_DISABLE_STD_FORMAT)
endif()
if (NOT WIN32)
  find_package(Threads REQUIRED)
  target_link_libraries(logme PUBLIC Threads::Threads ${CMAKE_DL_LIBS})
endif()
