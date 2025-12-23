cmake_minimum_required(VERSION 3.12)

project(pueorawdata VERSION 0.1
                    DESCRIPTION "Read/write PUEO Raw data"
                    LANGUAGES C  CXX)

include(GNUInstallDirs)
add_library(pueorawdata SHARED)
if(DEFINED ENV{PUEO_UTIL_INSTALL_DIR})
  set(CMAKE_INSTALL_PREFIX "$ENV{PUEO_UTIL_INSTALL_DIR}" CACHE PATH "install to pueo util path, if env. var. is defined" FORCE)
endif()

# Require C17 with gcc extensions
# also float32 support
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(default_build_type "RelWithDebInfo")
set(WARNFLAGS -Wall -Wextra -Wno-missing-braces -Wno-override-init)

find_package(ZLIB REQUIRED)

find_path(ZSTD_INCLUDE_DIR NAMES zstd.h)
find_library(ZSTD_LIBRARY NAMES zstd zstd_static)

find_package(PostgreSQL)
find_package(SQLite3)


target_sources(pueorawdata PRIVATE
  src/rawio.c
  src/rawio_packets.c
  src/rawio_versions.c
  src/sensor_ids.c
  src/pueocrc.c
  src/rawio_dump.c
  src/rawio_db.c
  src/prio_interface.c
)

target_sources(pueorawdata PUBLIC
  FILE_SET HEADERS
  BASE_DIRS inc/ src  # <-- Everything in BASE_DIRS is available during the build
  FILES  inc/pueo/rawdata.h        # <-- But only FILES will be installed
         inc/pueo/rawio.h 
         inc/pueo/sensor_ids.h 
         inc/pueo/sensor_ids_compat.h 
         inc/pueo/pueo.h 
         inc/pueo/prio_interface.h
)

# Install this library and its headers, exporting as a CMake Target that downstream projects can import
install(
  TARGETS ${PROJECT_NAME} 
  EXPORT ${PROJECT_NAME}Targets 
  FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Generate a Targets.cmake file and directly install it
# this is so that downstream projects can find the target created by this project.
install(
  EXPORT ${PROJECT_NAME}Targets
  NAMESPACE pueorawdata::
  DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}
)



#add_library(fakeprio SHARED test/fakeprio.c)
#target_link_libraries(fakeprio pueorawdata)

#add_library(fakeprio++ SHARED test/fakeprio.cc)
#target_link_libraries(fakeprio++ pueorawdata)


target_compile_options(pueorawdata PRIVATE ${WARNFLAGS})
target_link_libraries(pueorawdata PRIVATE ZLIB::ZLIB -lm -ldl)
target_link_libraries(pueorawdata PRIVATE ${ZSTD_LIBRARY})

target_include_directories(pueorawdata PRIVATE ${ZSTD_INCLUDE_DIR})
target_include_directories(pueorawdata PRIVATE ${ZLIB_INCLUDE_DIR})


if (PostgreSQL_FOUND)
  message(STATUS "Found PostgreSQL")
  target_link_libraries(pueorawdata PRIVATE PostgreSQL::PostgreSQL)
  target_include_directories(pueorawdata PRIVATE ${PostgreSQL_INCLUDE_DIRS})
  add_compile_definitions(PGSQL_ENABLED)
else()
  message(STATUS "PostgreSQL not found")
endif()

if (SQLite3_FOUND)
  message(STATUS "Found SQLite3")
  target_link_libraries(pueorawdata PRIVATE ${SQLite3_LIBRARIES})
  target_include_directories(pueorawdata PRIVATE ${SQLite3_INCLUDE_DIRS})
  add_compile_definitions(SQLITE_ENABLED)
else()
  message(STATUS "SQLite3 not found")
endif()


if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
      STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui/cmake-tui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

add_executable(write-wf test/write-wf.c)
target_link_libraries(write-wf pueorawdata)

add_executable(write-cmd-echo test/write-cmd-echo.c)
target_link_libraries(write-cmd-echo pueorawdata)

add_executable(read-wf test/read-wf.c)
target_link_libraries(read-wf pueorawdata)

add_executable(write-wfs test/write-wfs.c)
target_link_libraries(write-wfs pueorawdata)

add_executable(read-wfs test/read-wfs.c)
target_link_libraries(read-wfs pueorawdata)

add_executable(test-prio test/test-prio.c)
target_link_libraries(test-prio pueorawdata)

add_executable(packet-type test/packet-type.c)
target_link_libraries(packet-type pueorawdata)


add_executable(read-sensors-telem test/read-sensors-telem.c)
target_link_libraries(read-sensors-telem pueorawdata)

add_executable(pueo-init-db progs/pueo-init-db.c)
target_link_libraries(pueo-init-db pueorawdata)


add_executable(read-packets test/read-packets.c)
target_link_libraries(read-packets pueorawdata)

add_executable(read-events test/read-events.c)
target_link_libraries(read-events pueorawdata)

add_executable(read-image test/read-image.c)
target_link_libraries(read-image pueorawdata)

add_executable(read-files test/read-files.c)
target_link_libraries(read-files pueorawdata)
