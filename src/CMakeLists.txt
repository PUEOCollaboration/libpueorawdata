find_package(ZLIB)

set (SOURCES rawio.c rawio_packets.c rawio_versions.c) 

add_library(pueorawdata SHARED ${SOURCES} ${HEADERS}) 
add_library(pueorawdata-static STATIC ${SOURCES} ${HEADERS}) 

target_link_libraries(pueorawdata ZLIB::ZLIB) 
target_include_directories(pueorawdata PUBLIC ../inc) 
target_include_directories(pueorawdata PRIVATE .) 

target_include_directories(pueorawdata-static PUBLIC ../inc) 
target_include_directories(pueorawdata-static PRIVATE .) 


