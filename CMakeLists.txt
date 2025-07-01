cmake_minimum_required(VERSION 3.12)

project(pueorawdata VERSION 0.1
                    DESCRIPTION "Read/write PUEO Raw data"
                    LANGUAGES C)

# Require C17 with gcc extensions
# also float32 support
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(default_build_type "RelWithDebInfo")

find_package(ZLIB)

set (SOURCES src/rawio.c src/rawio_packets.c src/rawio_versions.c src/sensor_ids.c)

add_library(pueorawdata SHARED ${SOURCES} ${HEADERS})
add_library(pueorawdata-static STATIC ${SOURCES} ${HEADERS})

target_link_libraries(pueorawdata ZLIB::ZLIB)
target_include_directories(pueorawdata PUBLIC inc)
target_include_directories(pueorawdata PRIVATE src)

target_include_directories(pueorawdata-static PUBLIC inc)
target_include_directories(pueorawdata-static PRIVATE src)



if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
      STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui/cmake-tui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()



