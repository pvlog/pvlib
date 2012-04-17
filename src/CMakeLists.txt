set(src	log.c
	rfcomm.c
	smabluetooth.c
	smadata2plus.c
	protocol.c
	smanet.c
	connection.c
	pvlib.c
	thread.c
)
	
#add_library(pvlib SHARED ${src})
add_library(pvlib ${src})
target_link_libraries(pvlib bluetooth pthread rt)

INSTALL(FILES pvlib.h DESTINATION include/pvlib)
#INSTALL(TARGETS pvlib LIBRARY DESTINATION lib)
INSTALL(TARGETS pvlib ARCHIVE DESTINATION lib)
