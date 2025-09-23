# Helper to detect and build a vendored zstd submodule.
# Sets these variables when applicable:
#  - ZSTD_VENDOR_DIR
#  - ZSTD_CMAKE_TARGET (name of a library target if added via add_subdirectory)
#  - ZSTD_LIBRARY (path to libzstd.a/.so if available)
#  - ZSTD_INCLUDE_DIR (include dir for headers or wrapper)
#  - ZWRAP_SOURCE_TO_ADD (path to zstd_zlibwrapper.c to compile into project)
#  - HAVE_ZWRAP (TRUE if wrapper headers discovered)

function(pueo_zstd_probe_vendor)
  # Probe for a vendored third_party/zstd tree and expose variables.
  if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/zstd")
    message(STATUS "Using vendored zstd in ${CMAKE_SOURCE_DIR}/third_party/zstd")
    set(ZSTD_VENDOR_DIR "${CMAKE_SOURCE_DIR}/third_party/zstd" PARENT_SCOPE)

    # prefer prebuilt libs in third_party/zstd/lib if present (but don't
    # override externally provided ZSTD_LIBRARY)
    if(NOT DEFINED ZSTD_LIBRARY)
      if(EXISTS "${ZSTD_VENDOR_DIR}/lib/libzstd.a")
        set(ZSTD_LIBRARY "${ZSTD_VENDOR_DIR}/lib/libzstd.a" PARENT_SCOPE)
        set(ZSTD_INCLUDE_DIR "${ZSTD_VENDOR_DIR}/lib" PARENT_SCOPE)
        message(STATUS "Using vendored libzstd: ${ZSTD_LIBRARY}")
      elseif(EXISTS "${ZSTD_VENDOR_DIR}/lib/libzstd.so")
        set(ZSTD_LIBRARY "${ZSTD_VENDOR_DIR}/lib/libzstd.so" PARENT_SCOPE)
        set(ZSTD_INCLUDE_DIR "${ZSTD_VENDOR_DIR}/lib" PARENT_SCOPE)
        message(STATUS "Using vendored libzstd: ${ZSTD_LIBRARY}")
      endif()
    endif()

    # If the vendored tree provides a CMake build we can add it directly.
    # Note: we do not call add_subdirectory here to avoid side-effects on include.
    if(EXISTS "${ZSTD_VENDOR_DIR}/CMakeLists.txt")
      message(STATUS "Vendored zstd contains a CMakeLists.txt — caller may add_subdirectory(${ZSTD_VENDOR_DIR}) if desired")
      set(ZSTD_VENDOR_CMAKE_ROOT "${ZSTD_VENDOR_DIR}" PARENT_SCOPE)
    elseif(EXISTS "${ZSTD_VENDOR_DIR}/build/cmake/CMakeLists.txt")
      message(STATUS "Vendored zstd contains nested CMake at build/cmake — caller may add_subdirectory(${ZSTD_VENDOR_DIR}/build/cmake) if desired")
      set(ZSTD_VENDOR_CMAKE_NESTED_ROOT "${ZSTD_VENDOR_DIR}/build/cmake" PARENT_SCOPE)
    endif()

    # Prefer linking to a library target produced by that subproject if available.
    # Caller may add_subdirectory first and then call configure_pueorawdata_targets.
    if(TARGET zstd OR TARGET zstd_static OR TARGET libzstd OR TARGET libzstd_static OR TARGET libzstd_shared)
      set(_ZSTD_CMAKE_TARGETS zstd zstd_static libzstd libzstd_static libzstd_shared)
      foreach(t ${_ZSTD_CMAKE_TARGETS})
        if(TARGET ${t})
          get_target_property(_t_type ${t} TYPE)
          if(_t_type STREQUAL "STATIC_LIBRARY" OR _t_type STREQUAL "SHARED_LIBRARY" OR _t_type STREQUAL "MODULE_LIBRARY" OR _t_type STREQUAL "OBJECT_LIBRARY")
            set(ZSTD_CMAKE_TARGET ${t} PARENT_SCOPE)
            break()
          endif()
        endif()
      endforeach()
      if(ZSTD_CMAKE_TARGET)
        message(STATUS "Using zstd CMake target: ${ZSTD_CMAKE_TARGET}")
      endif()
    endif()

    # If the vendored zstd provides a zlibWrapper folder, prefer it for headers
    if(EXISTS "${ZSTD_VENDOR_DIR}/zlibWrapper/zstd_zlibwrapper.h")
      message(STATUS "Found zlibWrapper headers in vendored third_party/zstd/zlibWrapper")
      set(ZWRAP_HEADER_DIR "${ZSTD_VENDOR_DIR}/zlibWrapper" PARENT_SCOPE)
      set(ZSTD_INCLUDE_DIR "${ZSTD_VENDOR_DIR}/zlibWrapper" PARENT_SCOPE)
      set(HAVE_ZWRAP TRUE PARENT_SCOPE)
      # prefer using wrapper sources from vendored tree if present
      if(EXISTS "${ZSTD_VENDOR_DIR}/zlibWrapper/zstd_zlibwrapper.c")
        set(ZWRAP_SOURCE_TO_ADD "${ZSTD_VENDOR_DIR}/zlibWrapper/zstd_zlibwrapper.c" PARENT_SCOPE)
        # also detect common gz helper files located beside the wrapper
        set(_zwrap_gz_files gzclose.c gzlib.c gzguts.c gzread.c gzwrite.c)
        set(ZWRAP_GZ_SOURCES "")
        get_filename_component(_zwrap_dir "${ZWRAP_SOURCE_TO_ADD}" DIRECTORY)
        foreach(_gz ${_zwrap_gz_files})
          if(EXISTS "${_zwrap_dir}/${_gz}")
            list(APPEND ZWRAP_GZ_SOURCES "${_zwrap_dir}/${_gz}")
          endif()
        endforeach()
        set(ZWRAP_GZ_SOURCES "${ZWRAP_GZ_SOURCES}" PARENT_SCOPE)
      endif()
    endif()

    # Note: ExternalProject fallback is intentionally left as an action for
    # callers to perform since ExternalProject_Add has side-effects when run
    # at include time. If desired, callers can implement the ExternalProject
    # invocation using variables exposed by this probe (e.g., ZSTD_VENDOR_DIR).
  endif()
endfunction()


# Ensure reasonable system fallback if helper wasn't able to locate zstd
if(NOT DEFINED ZSTD_INCLUDE_DIR OR NOT DEFINED ZSTD_LIBRARY)
  find_path(ZSTD_INCLUDE_DIR NAMES zstd.h zlibWrapper/zstd_zlibwrapper.h)
  find_library(ZSTD_LIBRARY NAMES zstd zstd_static zstd_zlibwrapper)
endif()


# Public helper: configure pueorawdata targets with zlib/zstd and include dirs.
# Call as: configure_pueorawdata_targets(<shared-target> <static-target>)
function(configure_pueorawdata_targets shared_target static_target)
  # Ensure the external zstd build (if any) is built first
  if(DEFINED ZSTD_VENDOR_TARGET)
    add_dependencies(${shared_target} ${ZSTD_VENDOR_TARGET})
    add_dependencies(${static_target} ${ZSTD_VENDOR_TARGET})
  endif()

  # Compiler warnings and options
  if(DEFINED WARNFLAGS)
    target_compile_options(${shared_target} PRIVATE ${WARNFLAGS})
    target_compile_options(${static_target} PRIVATE ${WARNFLAGS})
  endif()

  # Always link against zlib
  target_link_libraries(${shared_target} PRIVATE ZLIB::ZLIB)
  target_link_libraries(${static_target} PRIVATE ZLIB::ZLIB)

  # Link zstd (prefer an in-tree CMake target, otherwise a library path)
  if(DEFINED ZSTD_CMAKE_TARGET)
    target_link_libraries(${shared_target} PRIVATE ${ZSTD_CMAKE_TARGET})
    target_link_libraries(${static_target} PRIVATE ${ZSTD_CMAKE_TARGET})
  elseif(ZSTD_LIBRARY AND EXISTS ${ZSTD_LIBRARY})
    target_link_libraries(${shared_target} PRIVATE ${ZSTD_LIBRARY})
    target_link_libraries(${static_target} PRIVATE ${ZSTD_LIBRARY})
  else()
    message(STATUS "libzstd not found by find_library or path does not exist; if using the zlibWrapper, ensure libzstd is available and set ZSTD_LIBRARY accordingly")
    if(DEFINED ZWRAP_SOURCE_TO_ADD AND ZWRAP_SOURCE_TO_ADD)
      message(STATUS "Note: zlibWrapper sources were added but libzstd is missing. Build or install libzstd or provide ZSTD_LIBRARY pointing to libzstd.a or libzstd.so")
    endif()
  endif()

  # Include directories
  target_include_directories(${shared_target} PUBLIC inc)
  target_include_directories(${shared_target} PRIVATE src)
  if(ZSTD_INCLUDE_DIR)
    target_include_directories(${shared_target} PRIVATE ${ZSTD_INCLUDE_DIR})
  endif()

  target_include_directories(${static_target} PUBLIC inc)
  target_include_directories(${static_target} PRIVATE src)
  if(ZSTD_INCLUDE_DIR)
    target_include_directories(${static_target} PRIVATE ${ZSTD_INCLUDE_DIR})
  endif()
endfunction()


# Higher-level helper to create pueorawdata targets, append wrapper sources and
# register executables. This consolidates the bulk of zstd-related wiring so
# the top-level CMakeLists can remain small.
# Note: setup_pueorawdata_project is intentionally omitted to keep the include
# passive. The original top-level `CMakeLists.txt` defines targets and
# executables; this helper only provides vendor probe and configure helper
# functions that callers may use explicitly.
