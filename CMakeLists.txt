cmake_minimum_required(VERSION 2.8)
project(vity)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function")

include_directories(.)
include_directories(/usr/local/include)
link_directories(/usr/local/lib)

find_library(YAMLCPP yaml-cpp)
message("***", ${YAMLCPP})

set(LIB_SRC
    vity/log.cc
    vity/utils.cc
    vity/config.cc
    )

add_library(vity SHARED ${LIB_SRC})

add_executable(test tests/tests.cc)
add_dependencies(test vity)
target_link_libraries(test vity ${YAMLCPP})

add_executable(test_yaml tests/test_yaml.cc)
add_dependencies(test_yaml vity)
target_link_libraries(test_yaml vity ${YAMLCPP})

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
