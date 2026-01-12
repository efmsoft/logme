get_filename_component(LOGME_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE) 


file(TO_CMAKE_PATH ${LOGME_ROOT}/logme SRCROOT)
file(GLOB_RECURSE SOURCES ${SRCROOT}/*.cpp)
file(GLOB_RECURSE HEADERS ${SRCROOT}/*.h)

add_library(logmed SHARED ${SOURCES} ${HEADERS} ${LOGME_VERSION_RC})

target_include_directories(logmed PUBLIC
  $<BUILD_INTERFACE:${SRCROOT}/include>
  $<INSTALL_INTERFACE:include>
)

target_include_directories(logmed PRIVATE
  ${ALLSTAT_INCLUDE_DIR}
)

target_compile_definitions(logmed PRIVATE _LOGME_DLL_BUILD_ _WINSOCK_DEPRECATED_NO_WARNINGS _CRT_SECURE_NO_WARNINGS _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR)
if (LOGME_DISABLE_STD_FORMAT)
  target_compile_definitions(logmed PUBLIC LOGME_DISABLE_STD_FORMAT)
endif()
target_link_libraries(logmed LINK_PUBLIC ${ALLSTAT_LIBRARIES} ${JSONCPP_LIBRARIES})

if(NOT WIN32)
  target_compile_options(logmed PRIVATE -fPIC -fvisibility=default)
endif()

if(WIN32)
  target_link_libraries(logmed PRIVATE Ws2_32)
else()
  target_link_libraries(logmed PRIVATE pthread dl) 
endif()
