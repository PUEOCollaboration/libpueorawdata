include("${CMAKE_CURRENT_LIST_DIR}/pueorawdataTargets.cmake")

include(CMakeFindDependencyMacro)
find_dependency(ZLIB REQUIRED)
find_package(PostgreSQL) #optional
find_package(SQLite3)    #optional
