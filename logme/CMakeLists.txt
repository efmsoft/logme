include_directories(${CMAKE_SOURCE_DIR}/logme/include)

file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "source/*.cpp")
add_library(logme STATIC ${SOURCES}) 

