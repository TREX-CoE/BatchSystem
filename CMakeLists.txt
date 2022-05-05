cmake_minimum_required(VERSION 3.5)

# detect if Catch is being bundled,
# disable testsuite in that case
if(NOT DEFINED PROJECT_NAME)
  set(NOT_SUBPROJECT ON)
else()
  set(NOT_SUBPROJECT OFF)
endif()

project(batchsystem LANGUAGES CXX VERSION 1.0.0)

FIND_PACKAGE(PkgConfig REQUIRED) # workaround else PKG_SEARCH_MODULE isn't found
PKG_SEARCH_MODULE(LIBXMLPP REQUIRED libxml++-2.6 libxml++-3.0)
message(STATUS "libxml++: ${LIBXMLPP_LIBRARIES}")

add_subdirectory(src/)

if(NOT_SUBPROJECT)
    add_subdirectory(test/)
endif(NOT_SUBPROJECT)
