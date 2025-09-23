cmake_minimum_required(VERSION 3.12)

project(pueorawdata VERSION 0.1
                    DESCRIPTION "Read/write PUEO Raw data"
                    LANGUAGES C)

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
set(WARNFLAGS -Wall -Wextra -Wno-missing-braces -Wno-override-init)

find_package(ZLIB REQUIRED)

find_path(ZSTD_INCLUDE_DIR NAMES zstd.h)
find_library(ZSTD_LIBRARY NAMES zstd zstd_static)

# Optional helper to detect/build vendored zstd submodule. If present it may
# set ZSTD_INCLUDE_DIR, ZSTD_LIBRARY, ZSTD_CMAKE_TARGET, HAVE_ZWRAP and
# ZWRAP_SOURCE_TO_ADD. This include is intentionally minimal and optional so
# the original CMake behavior remains unchanged when the helper is absent.
include(cmake/zstd-submodule.cmake OPTIONAL)

find_package(PostgreSQL)
find_package(SQLite3)


set (SOURCES src/rawio.c src/rawio_packets.c src/rawio_versions.c src/sensor_ids.c src/pueocrc.c src/rawio_dump.c src/rawio_db.c)
set(PUBLIC_HEADERS inc/pueo/rawdata.h inc/pueo/rawio.h inc/pueo/sensor_ids.h inc/pueo/sensor_ids_compat.h inc/pueo/pueo.h)
set(PRIVATE_HEADERS src/pueocrc.h src/rawio_packets.h src/rawio_versions.h)
set(HEADERS ${PUBLIC_HEADERS} ${PRIVATE_HEADERS})

add_library(pueorawdata SHARED ${SOURCES} ${HEADERS})
add_library(pueorawdata-static STATIC ${SOURCES} ${HEADERS})

target_compile_options(pueorawdata PRIVATE ${WARNFLAGS})
target_link_libraries(pueorawdata PRIVATE ZLIB::ZLIB)
target_link_libraries(pueorawdata PRIVATE ${ZSTD_LIBRARY})

target_include_directories(pueorawdata PUBLIC inc)
target_include_directories(pueorawdata PRIVATE src)
target_include_directories(pueorawdata PRIVATE ${ZSTD_INCLUDE_DIR})
target_include_directories(pueorawdata PRIVATE ${ZLIB_INCLUDE_DIR})

target_include_directories(pueorawdata-static PUBLIC inc)
target_include_directories(pueorawdata-static PRIVATE src)

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

add_executable(read-wf test/read-wf.c)
target_link_libraries(read-wf pueorawdata)

add_executable(write-wfs test/write-wfs.c)
target_link_libraries(write-wfs pueorawdata)

add_executable(read-wfs test/read-wfs.c)
target_link_libraries(read-wfs pueorawdata)

add_executable(read-sensors-telem test/read-sensors-telem.c)
target_link_libraries(read-sensors-telem pueorawdata)

add_executable(pueo-init-db progs/pueo-init-db.c)
target_link_libraries(pueo-init-db pueorawdata)


add_executable(read-packets test/read-packets.c)
target_link_libraries(read-packets pueorawdata)
# If we built the vendored zstd as an ExternalProject, ensure our targets
# depend on it so the library is available before linking.
if(DEFINED ZSTD_VENDOR_TARGET)
  add_dependencies(pueorawdata ${ZSTD_VENDOR_TARGET})
  add_dependencies(pueorawdata-static ${ZSTD_VENDOR_TARGET})
endif()

target_compile_options(pueorawdata PRIVATE ${WARNFLAGS})
target_link_libraries(pueorawdata PRIVATE ZLIB::ZLIB)
## Link zstd: prefer an in-tree CMake target from the vendored subproject, otherwise use an explicit library path
if(DEFINED ZSTD_CMAKE_TARGET)
  target_link_libraries(pueorawdata PRIVATE ${ZSTD_CMAKE_TARGET})
elseif(ZSTD_LIBRARY AND EXISTS ${ZSTD_LIBRARY})
  target_link_libraries(pueorawdata PRIVATE ${ZSTD_LIBRARY})
else()
  message(STATUS "libzstd not found by find_library or path does not exist; if using the zlibWrapper, ensure libzstd is available and set ZSTD_LIBRARY accordingly")
  if(DEFINED ZWRAP_SOURCE_TO_ADD AND ZWRAP_SOURCE_TO_ADD)
    message(STATUS "Note: zlibWrapper sources were added but libzstd is missing. Build or install libzstd (e.g., package libzstd-dev) or provide ZSTD_LIBRARY pointing to libzstd.a or libzstd.so")
  endif()
endif()

target_include_directories(pueorawdata PUBLIC inc)
target_include_directories(pueorawdata PRIVATE src)
if(ZSTD_INCLUDE_DIR)
  target_include_directories(pueorawdata PRIVATE ${ZSTD_INCLUDE_DIR})
endif()

# Apply the same compile options, link libraries and include directories to the
# static variant so both targets build cleanly and can find the public headers.
target_compile_options(pueorawdata-static PRIVATE ${WARNFLAGS})
target_link_libraries(pueorawdata-static PRIVATE ZLIB::ZLIB)
if(DEFINED ZSTD_CMAKE_TARGET)
  target_link_libraries(pueorawdata-static PRIVATE ${ZSTD_CMAKE_TARGET})
elseif(ZSTD_LIBRARY AND EXISTS ${ZSTD_LIBRARY})
  target_link_libraries(pueorawdata-static PRIVATE ${ZSTD_LIBRARY})
else()
  # No zstd found — user will see messages above if wrapper sources were added
endif()

target_include_directories(pueorawdata-static PUBLIC inc)
target_include_directories(pueorawdata-static PRIVATE src)
if(ZSTD_INCLUDE_DIR)
  target_include_directories(pueorawdata-static PRIVATE ${ZSTD_INCLUDE_DIR})
endif()

# If vendored helper didn't set zstd vars, fall back to system find
if(NOT DEFINED ZSTD_INCLUDE_DIR OR NOT DEFINED ZSTD_LIBRARY)
  find_path(ZSTD_INCLUDE_DIR NAMES zstd.h zlibWrapper/zstd_zlibwrapper.h)
  find_library(ZSTD_LIBRARY NAMES zstd zstd_static zstd_zlibwrapper)
endif()

