set(src
	log.cpp
	Rfcomm.cpp
	Smabluetooth.cpp
	Smadata2plus.cpp
	Protocol.cpp
	Smanet.cpp
	Connection.cpp
	pvlib.cpp
	resources.cpp
)
	
#add_library(pvlib SHARED ${src})
add_library(pvlib ${src})
target_link_libraries(pvlib bluetooth)

#INSTALL(FILES pvlib.h DESTINATION include/pvlib)
#INSTALL(TARGETS pvlib LIBRARY DESTINATION lib)
#INSTALL(TARGETS pvlib ARCHIVE DESTINATION lib)
