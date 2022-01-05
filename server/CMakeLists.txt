cmake_minimum_required(VERSION 3.21)
project(vp8_server)

set(CMAKE_CXX_STANDARD 14)

FIND_PACKAGE(Boost REQUIRED COMPONENTS system thread regex)
IF(Boost_FOUND)
    INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
    LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
ENDIF(Boost_FOUND)
SET(BOOST_LIBS ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY} ${Boost_REGEX_LIBRARY})
find_package(CapnProto REQUIRED)

add_executable(vp8_server main.cpp Serialization.h Proto/Packet.capnp.h Proto/Packet.capnp.c++)

target_link_libraries(vp8_server ${BOOST_LIBS} CapnProto::capnp)