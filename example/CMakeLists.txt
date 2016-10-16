#header dir
include_directories (${Pvlib_SOURCE_DIR}/src) 
MESSAGE(${Pvlib_SOURCE_DIR}/pvlib)
MESSAGE("test")

#library dir
#link_directories(${Pvlib_BINARY_DIR}/pvlib) 

set(src	main.cpp)
	
add_executable(pvlibshell ${src})
target_link_libraries(pvlibshell pvlib)
