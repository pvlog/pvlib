cmake_minimum_required(VERSION 2.6)

project(Pvlib)

set(${PROJECT_NAME}_MAJOR_VERSION 0)
set(${PROJECT_NAME}_MINOR_VERSION 0)
set(${PROJECT_NAME}_PATCH_LEVEL 1)

if (CMAKE_COMPILER_IS_GNUCC) 
	    set(CMAKE_C_FLAGS "-std=c99 -Wall -pedantic")
endif (CMAKE_COMPILER_IS_GNUCC)

#
# Doxygen documentation
#

# check if doxygen is even installed
find_package(Doxygen)
if (NOT DOXYGEN_FOUND)
    message(FATAL_ERROR "Doxygen not found. Please get a copy http://www.doxygen.org")
endif (NOT DOXYGEN_FOUND)

SET( DOXYGEN_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/pvlib")

STRING(REGEX REPLACE ";" " " CMAKE_DOXYGEN_INPUT_LIST "${DOXYGEN_SOURCE_DIR}")

# prepare doxygen configuration file
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

# add doxygen as target
add_custom_target(doxygen ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

# cleanup $build/api-doc on "make clean"
set_property(DIRECTORY APPEND PROPERTY
	     ADDITIONAL_MAKE_CLEAN_FILES api-doc)

# add doxygen as dependency to doc-target
get_target_property(DOC_TARGET doc TYPE)
if(NOT DOC_TARGET)
	add_custom_target(doc)
endif()
add_dependencies(doc doxygen)


#
#pvlib library
#
add_subdirectory(src)
add_subdirectory(example)
