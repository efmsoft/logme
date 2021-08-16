include_directories(${CMAKE_SOURCE_DIR}/logme/include)

file(TO_CMAKE_PATH ${CMAKE_SOURCE_DIR}/logme/source SRCROOT)
file(GLOB_RECURSE SOURCES ${SRCROOT}/*.cpp)

add_library(logmed SHARED ${SOURCES}) 
