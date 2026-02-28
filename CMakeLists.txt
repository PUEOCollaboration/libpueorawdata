cmake_minimum_required(VERSION 3.23)

project(pueorawdata VERSION 0.1
                    DESCRIPTION "Read/write PUEO Raw data"
                    LANGUAGES C  CXX)

if("${CMAKE_C_COMPILER_VERSION}" VERSION_LESS 13)
  message("If you are on the UChicago server: try `scl enable gcc-toolset-14 bash` and run cmake again,")
  message("Remember to clear the cache with the `--fresh` flag while re-running cmake.")
  message(FATAL_ERROR "Needs GCC 13+ for float16 support")
endif()

include(GNUInstallDirs)
add_library(pueorawdata SHARED)

# Require C17 with gcc extensions
# also float32 support
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(default_build_type "RelWithDebInfo")
set(WARNFLAGS -Wall -Wextra -Wno-missing-braces -Wno-override-init)

find_package(ZLIB REQUIRED)
find_package(PostgreSQL)
find_package(SQLite3)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # this creates compile_commands.json, useful for LSP

string(ASCII 27 Esc)
set(GREEN "${Esc}[1;32m")
set(BLUE "${Esc}[1;34m")
set(Reset "${Esc}[0m")

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  if(DEFINED ENV{PUEO_UTIL_INSTALL_DIR})
    message(STATUS "Environment variable PUEO_UTIL_INSTALL_DIR is defined and will force the install location.")
    set(CMAKE_INSTALL_PREFIX $ENV{PUEO_UTIL_INSTALL_DIR} CACHE STRING "highest priority" FORCE)
    message(STATUS "${GREEN}Will install to ${CMAKE_INSTALL_PREFIX}${Reset}")

  elseif(EXISTS /pueo)
    message(STATUS "Directory /pueo is found and will force the install location.")
    set(CMAKE_INSTALL_PREFIX /pueo CACHE STRING "Default install location on flight computer" FORCE)
    message(STATUS "${GREEN}Will install to ${CMAKE_INSTALL_PREFIX}${Reset}")

  elseif(CMAKE_INSTALL_PREFIX STREQUAL /usr/local) # <- only works on linux, but oh well
    message(WARNING "Will install to ${CMAKE_INSTALL_PREFIX}; make sure that's what you want.")
  endif()

else()
  message(STATUS "${GREEN}Will install to ${CMAKE_INSTALL_PREFIX}${Reset}")
endif()
message(STATUS "${BLUE}If the old value is cached, try running cmake again with --fresh.${Reset}")


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
  FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  LIBRARY DESTINATION lib
)

# Install the Config file so that pueoEvent knows where to find pueoEventTargets.cmake
# Why do this? cuz cmake is loves boiler plate I guess?
install(
  FILES "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake"
  DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}
)

# Generate a Targets.cmake file and directly install it
# this is so that downstream projects can find the target created by this project.
install(
  EXPORT ${PROJECT_NAME}Targets
  NAMESPACE PUEO::
  DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}
)



#add_library(fakeprio SHARED test/fakeprio.c)
#target_link_libraries(fakeprio pueorawdata)

#add_library(fakeprio++ SHARED test/fakeprio.cc)
#target_link_libraries(fakeprio++ pueorawdata)


target_compile_options(pueorawdata PRIVATE ${WARNFLAGS})
target_link_libraries(pueorawdata PRIVATE ZLIB::ZLIB m dl) # m for libm.so, dl for libdl.so
target_link_libraries(pueorawdata PRIVATE ${ZSTD_LIBRARY})


if (PostgreSQL_FOUND)
  message(STATUS "Found PostgreSQL")
  target_link_libraries(pueorawdata PRIVATE PostgreSQL::PostgreSQL)
  target_compile_definitions(pueorawdata PRIVATE PGSQL_ENABLED)
else()
  message(STATUS "PostgreSQL not found")
endif()

if (SQLite3_FOUND)
  message(STATUS "Found SQLite3")
  target_link_libraries(pueorawdata PRIVATE SQLite::SQLite3)
  target_compile_definitions(pueorawdata PRIVATE SQLITE_ENABLED)
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

macro(add_program binary_name where)
      add_executable(${binary_name} ${where}/${binary_name}.c)
      target_link_libraries(${binary_name} pueorawdata)
      install(TARGETS ${binary_name} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endmacro()

add_program(write-wf test)
add_program(write-cmd-echo test)
add_program(read-wf test)
add_program(write-wfs test)
add_program(read-wfs test)
add_program(test-prio test)
add_program(packet-type test)
add_program(read-sensors-telem test)
add_program(pueo-init-db progs)
add_program(read-packets test)
add_program(read-events test)
add_program(read-image test)
add_program(read-files test)

