# Include this file to your project's CMakeList.txt to initialize
# LOGME_INCLUDE_DIR and LOGME_LIBRARIES variables and add logme
# subproject
#
# It is clear that this file does not try to search for the location 
# of the library, but knows it exactly because it is located in the 
# cmake subdirectory. We implemented this approach to connect all our 
# libraries in a uniform way. Both from the root files of the 
# repositories, and their use by external projects. In each case, it 
# is enough to add the path to the directory and start the search
#
# list(APPEND CMAKE_MODULE_PATH <location_of_lib_in_your_project>/cmake) 
# find_package(logme MODULE) 

get_filename_component(LOGME_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE) 

set(LOGME_INCLUDE_DIR ${LOGME_ROOT}/logme/include)
set(LOGME_LIBRARIES logme)

add_subdirectory(${LOGME_ROOT}/logme)