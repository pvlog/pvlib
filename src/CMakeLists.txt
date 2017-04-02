set(src
	rfcomm.cpp
	smabluetooth.cpp
	smadata2plus.cpp
	protocol.cpp
	smanet.cpp
	connection.cpp
	pvlib.cpp
	resources.cpp
	log.cpp
)
	
#add_library(pvlib SHARED ${src})
add_library(pvlib ${src})
target_link_libraries(pvlib bluetooth)

#INSTALL(FILES pvlib.h DESTINATION include/pvlib)
#INSTALL(TARGETS pvlib LIBRARY DESTINATION lib)
#INSTALL(TARGETS pvlib ARCHIVE DESTINATION lib)
