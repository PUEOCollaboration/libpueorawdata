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

## Prefer a vendored zstd submodule under third_party/zstd if present
## Prefer a vendored zstd submodule under third_party/zstd if present
if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/zstd")
  message(STATUS "Using vendored zstd in ${CMAKE_SOURCE_DIR}/third_party/zstd")
  set(ZSTD_VENDOR_DIR "${CMAKE_SOURCE_DIR}/third_party/zstd")
  # prefer a prebuilt lib in the vendored tree if present
  if(EXISTS "${ZSTD_VENDOR_DIR}/lib/libzstd.a")
    set(ZSTD_LIBRARY "${ZSTD_VENDOR_DIR}/lib/libzstd.a")
    set(ZSTD_INCLUDE_DIR "${ZSTD_VENDOR_DIR}/lib")
  elseif(EXISTS "${ZSTD_VENDOR_DIR}/lib/libzstd.so")
    set(ZSTD_LIBRARY "${ZSTD_VENDOR_DIR}/lib/libzstd.so")
    set(ZSTD_INCLUDE_DIR "${ZSTD_VENDOR_DIR}/lib")
  else()
    # No prebuilt lib found; build zstd as an ExternalProject and install into the build tree
    include(ExternalProject)
    ExternalProject_Add(zstd_vendor
      SOURCE_DIR ${ZSTD_VENDOR_DIR}
      BINARY_DIR ${CMAKE_BINARY_DIR}/third_party/zstd/build
      CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/third_party/zstd/install -DZSTD_BUILD_TESTS=OFF -DZSTD_BUILD_CONTRIB=OFF -DZSTD_BUILD_DOC=OFF
      BUILD_BYPRODUCTS ${CMAKE_BINARY_DIR}/third_party/zstd/install/lib/libzstd.a
    )
    set(ZSTD_INCLUDE_DIR "${CMAKE_BINARY_DIR}/third_party/zstd/install/include")
    set(ZSTD_LIBRARY "${CMAKE_BINARY_DIR}/third_party/zstd/install/lib/libzstd.a")
    # ensure our pueorawdata target depends on the external zstd build
    set(ZSTD_VENDOR_TARGET zstd_vendor)
  endif()
else()
  find_path(ZSTD_INCLUDE_DIR NAMES zstd.h zlibWrapper/zstd_zlibwrapper.h)
  find_library(ZSTD_LIBRARY NAMES zstd zstd_static zstd_zlibwrapper)
endif()

# Optionally enable building with zstd's zlibWrapper (provides zlib/gz API backed by zstd)
option(ENABLE_ZWRAP "Detect and build with zstd zlibWrapper when available" ON)
option(ZWRAP_SRC_DIR "Explicit path to zstd zlibWrapper source directory" "")

set(HAVE_ZWRAP FALSE)
if(ENABLE_ZWRAP)
  # Prefer an explicit wrapper source dir if provided and it has the header
  if(ZWRAP_SRC_DIR AND EXISTS "${ZWRAP_SRC_DIR}/zstd_zlibwrapper.h")
    set(ZWRAP_HEADER_DIR "${ZWRAP_SRC_DIR}")
    set(HAVE_ZWRAP TRUE)
  elseif(ZSTD_INCLUDE_DIR)
    if(EXISTS "${ZSTD_INCLUDE_DIR}/zlibWrapper/zstd_zlibwrapper.h")
      set(ZWRAP_HEADER_DIR "${ZSTD_INCLUDE_DIR}/zlibWrapper")
      set(HAVE_ZWRAP TRUE)
    elseif(EXISTS "${ZSTD_INCLUDE_DIR}/zstd_zlibwrapper.h")
      get_filename_component(ZWRAP_HEADER_DIR "${ZSTD_INCLUDE_DIR}" ABSOLUTE)
      set(HAVE_ZWRAP TRUE)
    endif()
  endif()

  if(HAVE_ZWRAP)
    message(STATUS "zstd zlibWrapper headers found in ${ZWRAP_HEADER_DIR}")
    # If user provided an explicit source dir, prefer it. Otherwise try to find
    # wrapper source next to the header (common layout in zstd repo). Also
    # prefer a vendored third_party/zstd layout if present.
    set(ZWRAP_SRC_CANDIDATES
      "${ZWRAP_SRC_DIR}/zstd_zlibwrapper.c"
      "${CMAKE_SOURCE_DIR}/third_party/zstd/zlibWrapper/zstd_zlibwrapper.c"
      "${ZWRAP_HEADER_DIR}/zstd_zlibwrapper.c"
      "${ZSTD_INCLUDE_DIR}/../zlibWrapper/zstd_zlibwrapper.c"
      "${CMAKE_SOURCE_DIR}/src/zstd/zlibWrapper/zstd_zlibwrapper.c"
      "${CMAKE_SOURCE_DIR}/../src/zstd/zlibWrapper/zstd_zlibwrapper.c"
    )
    set(ZWRAP_SRC_FILE "")
    foreach(candidate ${ZWRAP_SRC_CANDIDATES})
      if(candidate AND EXISTS "${candidate}")
        set(ZWRAP_SRC_FILE "${candidate}")
        break()
      endif()
    endforeach()

    if(ZWRAP_SRC_FILE)
      message(STATUS "zstd zlibWrapper source found: ${ZWRAP_SRC_FILE}")
      set(ZWRAP_SOURCE_TO_ADD ${ZWRAP_SRC_FILE})
      # ensure wrapper compiled with zstd enabled
      add_compile_definitions(ZWRAP_USE_ZSTD=1)
    else()
      message(STATUS "zstd zlibWrapper headers found but source not found; you must provide zstd_zlibwrapper.c and gz*.c or link a prebuilt wrapper library.")
      # still define macro so code that includes header compiles; linking must be handled externally
      add_compile_definitions(ZWRAP_USE_ZSTD=1)
    endif()
  endif()
endif()

find_package(PostgreSQL)
find_package(SQLite3)


set (SOURCES src/rawio.c src/rawio_packets.c src/rawio_versions.c src/sensor_ids.c src/pueocrc.c src/rawio_dump.c src/rawio_db.c)

# If we discovered a zlibWrapper source file, append it (and nearby gz* helpers if present)
if(DEFINED ZWRAP_SOURCE_TO_ADD AND ZWRAP_SOURCE_TO_ADD)
  list(APPEND SOURCES ${ZWRAP_SOURCE_TO_ADD})
  get_filename_component(_zwrap_dir ${ZWRAP_SOURCE_TO_ADD} DIRECTORY)
  # common gz helper names in the wrapper tree
  foreach(_gzfile gzclose.c gzlib.c gzguts.c gzread.c gzwrite.c)
    if(EXISTS "${_zwrap_dir}/${_gzfile}")
      list(APPEND SOURCES "${_zwrap_dir}/${_gzfile}")
    endif()
  endforeach()
endif()
set(PUBLIC_HEADERS inc/pueo/rawdata.h inc/pueo/rawio.h inc/pueo/sensor_ids.h inc/pueo/sensor_ids_compat.h inc/pueo/pueo.h)
set(PRIVATE_HEADERS src/pueocrc.h src/rawio_packets.h src/rawio_versions.h)
set(HEADERS ${PUBLIC_HEADERS} ${PRIVATE_HEADERS})

add_library(pueorawdata SHARED ${SOURCES} ${HEADERS})
add_library(pueorawdata-static STATIC ${SOURCES} ${HEADERS})

# If we built the vendored zstd as an ExternalProject, ensure our targets
# depend on it so the library is available before linking.
if(DEFINED ZSTD_VENDOR_TARGET)
  add_dependencies(pueorawdata ${ZSTD_VENDOR_TARGET})
  add_dependencies(pueorawdata-static ${ZSTD_VENDOR_TARGET})
endif()

target_compile_options(pueorawdata PRIVATE ${WARNFLAGS})
target_link_libraries(pueorawdata PRIVATE ZLIB::ZLIB)
# Only link ZSTD_LIBRARY if it actually exists as a file or is a target
if(ZSTD_LIBRARY AND EXISTS ${ZSTD_LIBRARY})
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
if(HAVE_ZWRAP)
  target_include_directories(pueorawdata PRIVATE ${ZWRAP_HEADER_DIR})
endif()

target_include_directories(pueorawdata-static PUBLIC inc)
target_include_directories(pueorawdata-static PRIVATE src)
if(ZSTD_INCLUDE_DIR)
  target_include_directories(pueorawdata-static PRIVATE ${ZSTD_INCLUDE_DIR})
endif()
if(HAVE_ZWRAP)
  target_include_directories(pueorawdata-static PRIVATE ${ZWRAP_HEADER_DIR})
endif()

# If wrapper compile macro was set globally earlier, prefer setting it per-target so other targets are unaffected
if(DEFINED ZWRAP_SOURCE_TO_ADD AND ZWRAP_SOURCE_TO_ADD)
  target_compile_definitions(pueorawdata PRIVATE ZWRAP_USE_ZSTD=1)
  target_compile_definitions(pueorawdata-static PRIVATE ZWRAP_USE_ZSTD=1)
elseif(HAVE_ZWRAP)
  # headers are available; still expose the macro to ensure sources including the header see the flag
  target_compile_definitions(pueorawdata PRIVATE ZWRAP_USE_ZSTD=1 HAVE_ZWRAP_HEADER=1)
  target_compile_definitions(pueorawdata-static PRIVATE ZWRAP_USE_ZSTD=1 HAVE_ZWRAP_HEADER=1)
endif()

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

# Smoke test: write/read a small .zst file using the zlibWrapper path
add_executable(read-zst test/read-zst.c)
target_link_libraries(read-zst pueorawdata)
