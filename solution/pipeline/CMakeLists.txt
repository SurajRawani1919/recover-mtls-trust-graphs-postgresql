cmake_minimum_required(VERSION 3.16)
project(mtls_graph_recovery CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
find_package(CURL REQUIRED)
pkg_check_modules(LIBXML2 REQUIRED libxml-2.0)
pkg_check_modules(LIBPQ REQUIRED libpq)

add_executable(recover_graph
    main.cpp
    repair.cpp
    xsd_validate.cpp
    verifier_client.cpp
    db_ingest.cpp
    report.cpp
    json_util.cpp
)

target_include_directories(recover_graph PRIVATE ${LIBXML2_INCLUDE_DIRS} ${LIBPQ_INCLUDE_DIRS})
target_link_libraries(recover_graph PRIVATE ${LIBXML2_LIBRARIES} ${LIBPQ_LIBRARIES} CURL::libcurl)
