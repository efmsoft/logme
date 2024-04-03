get_filename_component(LOGME_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE) 

file(TO_CMAKE_PATH ${LOGME_ROOT}/logme/source SRCROOT)
file(GLOB_RECURSE SOURCES ${SRCROOT}/*.cpp)
file(GLOB_RECURSE HEADERS ${SRCROOT}/*.h)

add_library(logmed SHARED ${SOURCES} ${HEADERS})

target_include_directories(logmed PRIVATE 
  ${LOGME_INCLUDE_DIR}
  ${ALLSTAT_INCLUDE_DIR}
)

target_compile_definitions(logmed PRIVATE _LOGME_DLL_BUILD_ _WINSOCK_DEPRECATED_NO_WARNINGS)
target_link_libraries(logmed LINK_PUBLIC ${ALLSTAT_LIBRARIES} ${JSONCPP_LIBRARIES})

if(NOT WIN32)
  target_compile_options(logmed PRIVATE -fPIC -fvisibility=default)
endif()

if(WIN32)
  target_link_libraries(logmed PRIVATE Ws2_32)
else()
  target_link_libraries(logmed LINK_PUBLIC pthread) 
endif()