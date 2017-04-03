include_directories (${Pvlib_SOURCE_DIR}/src) 

set(src	main.cpp)
	
add_executable(pvlibshell ${src})
target_link_libraries(pvlibshell pvlib)

INSTALL(TARGETS pvlibshell
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
