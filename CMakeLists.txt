cmake_minimum_required(VERSION 2.8)
project(vity)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function")

include_directories(.)
include_directories(/usr/local/include)
link_directories(/usr/local/lib)

# find_library(YAMLCPP yaml-cpp)
# message("***", ${YAMLCPP})

# find_library(PTHREAD pthread)
# message("***", ${PTHREAD})

set(LIBS
        vity
        pthread
        yaml-cpp)
message("***", ${LIBS})

set(LIB_SRC
    vity/log.cc
    vity/utils.cc
    vity/config.cc
    vity/thread.cc
    vity/fiber.cc
    )

add_library(vity SHARED ${LIB_SRC})

add_executable(test tests/tests.cc)
add_dependencies(test vity)
target_link_libraries(test ${LIBS})

add_executable(test_yaml tests/test_yaml.cc)
add_dependencies(test_yaml vity)
target_link_libraries(test_yaml ${LIBS})

add_executable(test_thread tests/test_thread.cc)
add_dependencies(test_thread vity)
target_link_libraries(test_thread ${LIBS})

add_executable(test_util tests/test_util.cc)
add_dependencies(test_util vity)
target_link_libraries(test_util ${LIBS})

add_executable(test_fiber tests/test_fiber.cc)
add_dependencies(test_fiber vity)
target_link_libraries(test_fiber ${LIBS})

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
