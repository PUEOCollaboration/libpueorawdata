include("${CMAKE_CURRENT_LIST_DIR}/pueorawdataTargets.cmake")

include(CMakeFindDependencyMacro)
find_dependency(ZLIB REQUIRED)
find_package(PostgreSQL)
find_package(SQLite3)
