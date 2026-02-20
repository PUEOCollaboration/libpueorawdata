include("${CMAKE_CURRENT_LIST_DIR}/pueorawdataTargets.cmake")

include(CMakeFindDependencyMacro)
find_dependency(ZLIB REQUIRED)
find_dependency(PostgreSQL)
find_dependency(SQLite3)
