set(src
	rfcomm.cpp
	smabluetooth.cpp
	smadata2plus.cpp
	protocol.cpp
	smanet.cpp
	connection.cpp
	pvlib.cpp
	log.cpp
	utility.cpp
)
	
add_library(pvlib ${src})
target_link_libraries(pvlib bluetooth)
set_target_properties(pvlib PROPERTIES PUBLIC_HEADER "pvlib.h")

INSTALL(TARGETS pvlib
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/pvlib
)

