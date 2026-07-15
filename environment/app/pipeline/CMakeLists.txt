cmake_minimum_required(VERSION 3.16)
project(mtls_graph_recovery CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Agent starter target. Implement the modules in this directory, then extend
# CMakeLists.txt to compile repair.cpp, xsd_validate.cpp, verifier_client.cpp,
# db_ingest.cpp, report.cpp, and json_util.cpp.
add_executable(recover_graph main.cpp)
