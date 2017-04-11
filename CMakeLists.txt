cmake_minimum_required(VERSION 3.0.2)

project(Pvlib)

include(GNUInstallDirs)

set(PVLIB_MAJOR_VERSION 0)
set(PVLIB_MINOR_VERSION 0)
set(PVLIB_PATCH_VERSION 1)

set(PVLIB_VERSION ${PVLIB_MAJOR_VERSION}.${PVLIB_MINOR_VERSION}.${PVLIB_PATCH_VERSION})

if (CMAKE_COMPILER_IS_GNUCC) 
	    set(CMAKE_CXX_FLAGS "-std=c++11 -pthread -Wall -pedantic")
endif (CMAKE_COMPILER_IS_GNUCC)

#
# Doxygen documentation
#

# check if doxygen is even installed
find_package(Doxygen)
if (DOXYGEN_FOUND)
	SET(DOXYGEN_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

	STRING(REGEX REPLACE ";" " " CMAKE_DOXYGEN_INPUT_LIST "${DOXYGEN_SOURCE_DIR}")

	# prepare doxygen configuration file
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

	# add doxygen as target
	add_custom_target(doxygen ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

	# cleanup $build/api-doc on "make clean"
	set_property(DIRECTORY APPEND PROPERTY
		     ADDITIONAL_MAKE_CLEAN_FILES api-doc)

	# add doxygen as dependency to doc-target
	add_custom_target(doc)
	add_dependencies(doc doxygen)
endif (DOXYGEN_FOUND)

set(RESOURCE_DIR ${CMAKE_INSTALL_DATADIR}/pvlib)

#install resources
install(DIRECTORY resources/ DESTINATION ${RESOURCE_DIR})

configure_file(config.h.in config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

#
#pvlib library
#
add_subdirectory(src)
add_subdirectory(example)
