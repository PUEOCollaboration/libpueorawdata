# Helper to detect and build a vendored zstd submodule.
# Sets these variables when applicable:
#  - ZSTD_VENDOR_DIR
#  - ZSTD_CMAKE_TARGET (name of a library target if added via add_subdirectory)
#  - ZSTD_LIBRARY (path to libzstd.a/.so if available)
#  - ZSTD_INCLUDE_DIR (include dir for headers or wrapper)
#  - ZWRAP_SOURCE_TO_ADD (path to zstd_zlibwrapper.c to compile into project)
#  - HAVE_ZWRAP (TRUE if wrapper headers discovered)

if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/zstd")
  message(STATUS "Using vendored zstd in ${CMAKE_SOURCE_DIR}/third_party/zstd")
  set(ZSTD_VENDOR_DIR "${CMAKE_SOURCE_DIR}/third_party/zstd")

  # prefer prebuilt libs in third_party/zstd/lib if present (but don't
  # override externally provided ZSTD_LIBRARY)
  if(NOT DEFINED ZSTD_LIBRARY)
    if(EXISTS "${ZSTD_VENDOR_DIR}/lib/libzstd.a")
      set(ZSTD_LIBRARY "${ZSTD_VENDOR_DIR}/lib/libzstd.a")
      set(ZSTD_INCLUDE_DIR "${ZSTD_VENDOR_DIR}/lib")
      message(STATUS "Using vendored libzstd: ${ZSTD_LIBRARY}")
    elseif(EXISTS "${ZSTD_VENDOR_DIR}/lib/libzstd.so")
      set(ZSTD_LIBRARY "${ZSTD_VENDOR_DIR}/lib/libzstd.so")
      set(ZSTD_INCLUDE_DIR "${ZSTD_VENDOR_DIR}/lib")
      message(STATUS "Using vendored libzstd: ${ZSTD_LIBRARY}")
    endif()
  endif()

  # If the vendored tree provides a CMake build we can add it directly.
  if(EXISTS "${ZSTD_VENDOR_DIR}/CMakeLists.txt")
    message(STATUS "Found CMake build for vendored zstd; adding as subdirectory")
    add_subdirectory(${ZSTD_VENDOR_DIR} ${CMAKE_BINARY_DIR}/third_party/zstd/build EXCLUDE_FROM_ALL)
  elseif(EXISTS "${ZSTD_VENDOR_DIR}/build/cmake/CMakeLists.txt")
    message(STATUS "Found nested zstd CMake at ${ZSTD_VENDOR_DIR}/build/cmake; adding as subdirectory")
    add_subdirectory(${ZSTD_VENDOR_DIR}/build/cmake ${CMAKE_BINARY_DIR}/third_party/zstd/build EXCLUDE_FROM_ALL)
  endif()

  # Prefer linking to a library target produced by that subproject if available.
  if(TARGET zstd OR TARGET zstd_static OR TARGET libzstd OR TARGET libzstd_static OR TARGET libzstd_shared)
    set(_ZSTD_CMAKE_TARGETS zstd zstd_static libzstd libzstd_static libzstd_shared)
    foreach(t ${_ZSTD_CMAKE_TARGETS})
      if(TARGET ${t})
        get_target_property(_t_type ${t} TYPE)
        if(_t_type STREQUAL "STATIC_LIBRARY" OR _t_type STREQUAL "SHARED_LIBRARY" OR _t_type STREQUAL "MODULE_LIBRARY" OR _t_type STREQUAL "OBJECT_LIBRARY")
          set(ZSTD_CMAKE_TARGET ${t})
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
    set(ZWRAP_HEADER_DIR "${ZSTD_VENDOR_DIR}/zlibWrapper")
    set(ZSTD_INCLUDE_DIR "${ZSTD_VENDOR_DIR}/zlibWrapper")
    set(HAVE_ZWRAP TRUE)
    # prefer using wrapper sources from vendored tree if present
    if(EXISTS "${ZSTD_VENDOR_DIR}/zlibWrapper/zstd_zlibwrapper.c")
      set(ZWRAP_SOURCE_TO_ADD "${ZSTD_VENDOR_DIR}/zlibWrapper/zstd_zlibwrapper.c")
    endif()
  endif()

  # If we didn't get a CMake target or prebuilt lib, fall back to ExternalProject
  if(NOT DEFINED ZSTD_CMAKE_TARGET AND NOT DEFINED ZSTD_LIBRARY)
    include(ExternalProject)
    # If the vendored tree actually has a CMakeLists at its root we build that
    if(EXISTS "${ZSTD_VENDOR_DIR}/CMakeLists.txt")
      ExternalProject_Add(zstd_vendor
        SOURCE_DIR ${ZSTD_VENDOR_DIR}
        BINARY_DIR ${CMAKE_BINARY_DIR}/third_party/zstd/build
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/third_party/zstd/install -DZSTD_BUILD_TESTS=OFF -DZSTD_BUILD_CONTRIB=OFF -DZSTD_BUILD_DOC=OFF
        BUILD_BYPRODUCTS ${CMAKE_BINARY_DIR}/third_party/zstd/install/lib/libzstd.a
      )
    else()
      # fallback: clone upstream zstd into the build dir and build that
      ExternalProject_Add(zstd_vendor
        GIT_REPOSITORY https://github.com/facebook/zstd.git
        GIT_TAG v1.5.5
        SOURCE_DIR ${CMAKE_BINARY_DIR}/third_party/zstd/src
        BINARY_DIR ${CMAKE_BINARY_DIR}/third_party/zstd/build
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/third_party/zstd/install -DZSTD_BUILD_TESTS=OFF -DZSTD_BUILD_CONTRIB=OFF -DZSTD_BUILD_DOC=OFF
        BUILD_BYPRODUCTS ${CMAKE_BINARY_DIR}/third_party/zstd/install/lib/libzstd.a
      )
    endif()
    set(ZSTD_VENDOR_TARGET zstd_vendor)
    set(ZSTD_INCLUDE_DIR "${CMAKE_BINARY_DIR}/third_party/zstd/install/include")
    set(ZSTD_LIBRARY "${CMAKE_BINARY_DIR}/third_party/zstd/install/lib/libzstd.a")
  endif()
endif()
