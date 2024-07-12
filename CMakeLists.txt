cmake_minimum_required(VERSION 2.6)
project(lss)
set(CMAKE_INSTALL_PREFIX ../bin)

include(ExternalProject)
ExternalProject_Add(jsoncpp
        SOURCE_DIR ${PROJECT_SOURCE_DIR}/third_party/jsoncpp
        CONFIGURE_COMMAND mkdir -p ../jsoncpp-build && cd ../jsoncpp-build && cmake ../jsoncpp -DCMAKE_INSTALL_PREFIX=${PROJECT_SOURCE_DIR}/lib/jsoncpp/
        BUILD_COMMAND cd ../jsoncpp-build && make 
        BUILD_IN_SOURCE 1
        INSTALL_COMMAND cd ../jsoncpp-build && make install
)

include_directories(${PROJECT_SOURCE_DIR}/lib/jsoncpp/include/)
link_directories(${PROJECT_SOURCE_DIR}/lib/jsoncpp/lib64/)
link_directories(${PROJECT_SOURCE_DIR}/lib/jsoncpp/lib/)

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O0 -g -pipe -Wall -Werror -Wno-unused-variable -lrt -pthread")
elseif (NOT CMAKE_BUILD_TYPE STREQUAL Debug)
    message(STATUS "Release Version.")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3 -g -pipe -Wall -Werror -Wno-unused-variable -lrt -pthread")
endif ()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-strict-aliasing -Wno-error=shadow -Wno-sign-compare")

message(STATUS "USING GCC VERSION: ${CMAKE_CXX_COMPILER_VERSION}")

if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 7.2.9)
    message(STATUS "GCC Version > 7.3.0, USING -Wno-error=unused-result -Werror=return-type")
    set(CMAKE_CXX_FLAGS "-Wno-error=unused-result -Werror=return-type ${CMAKE_CXX_FLAGS}")
endif()

include_directories(src)
add_subdirectory(src)