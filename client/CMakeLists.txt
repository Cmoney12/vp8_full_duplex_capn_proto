cmake_minimum_required(VERSION 3.21)
project(vp8_full_duplex_capn)

set(CMAKE_CXX_STANDARD 14)

find_package(OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )
find_package (SQLite3)
if (SQLITE3_FOUND)
    message("SQLite3 found")
    include_directories(${SQLITE3_INCLUDE_DIRS})
endif()

FIND_PATH(VPX_INCLUDE_DIR
        NAMES vpx/vpx_encoder.h
        PATHS /usr/local/include/
        /usr/include/)
MARK_AS_ADVANCED(VPX_INCLUDE_DIR)
FIND_LIBRARY(VPX_LIBRARY
        NAMES vpx
        PATHS ${LIBVPXDIR}/lib/
        /usr/lib/arm-linux-gnueabihf/
        /usr/lib/arm-linux-gnueabi/
        /usr/lib/x86_64-linux-gnu/
        /usr/local/lib64/
        /usr/lib64/
        /usr/lib/)
MARK_AS_ADVANCED(VPX_LIBRARY)

###########################################################################
IF (VPX_INCLUDE_DIR
        AND VPX_LIBRARY)
    SET(VPX_FOUND 1)
    SET(VPX_LIBRARIES ${VPX_LIBRARY})
    SET(VPX_INCLUDE_DIRS ${VPX_INCLUDE_DIR})
ENDIF()

MARK_AS_ADVANCED(VPX_LIBRARIES)
MARK_AS_ADVANCED(VPX_INCLUDE_DIRS)

IF (VPX_FOUND)
    MESSAGE(STATUS "Found libvpx: ${VPX_INCLUDE_DIRS}, ${VPX_LIBRARIES}")
ELSE ()
    MESSAGE(STATUS "Could not find libvpx")
ENDIF()

FIND_PACKAGE(Boost REQUIRED COMPONENTS system thread regex)
IF(Boost_FOUND)
    INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
    LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
ENDIF(Boost_FOUND)

SET(USED_LIBS ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY} ${Boost_REGEX_LIBRARY})


if (PORTAUDIO_LIBRARIES AND PORTAUDIO_INCLUDE_DIRS)
    # in cache already
    set(PORTAUDIO_FOUND TRUE)
else (PORTAUDIO_LIBRARIES AND PORTAUDIO_INCLUDE_DIRS)
    if (NOT WIN32)
        include(FindPkgConfig)
        pkg_check_modules(PORTAUDIO2 portaudio-2.0)
    endif (NOT WIN32)

    if (PORTAUDIO2_FOUND)
        set(PORTAUDIO_INCLUDE_DIRS
                ${PORTAUDIO2_INCLUDE_DIRS}
                )
        if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
            set(PORTAUDIO_LIBRARIES "${PORTAUDIO2_LIBRARY_DIRS}/lib${PORTAUDIO2_LIBRARIES}.dylib")
        else (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
            set(PORTAUDIO_LIBRARIES
                    ${PORTAUDIO2_LIBRARIES}
                    )
        endif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        set(PORTAUDIO_VERSION
                19
                )
        set(PORTAUDIO_FOUND TRUE)
    else (PORTAUDIO2_FOUND)
        find_path(PORTAUDIO_INCLUDE_DIR
                NAMES
                portaudio.h
                PATHS
                /usr/include
                /usr/local/include
                /opt/local/include
                /sw/include
                )

        find_library(PORTAUDIO_LIBRARY
                NAMES
                portaudio
                PATHS
                /usr/lib
                /usr/local/lib
                /opt/local/lib
                /sw/lib
                )

        find_path(PORTAUDIO_LIBRARY_DIR
                NAMES
                portaudio
                PATHS
                /usr/lib
                /usr/local/lib
                /opt/local/lib
                /sw/lib
                )

        set(PORTAUDIO_INCLUDE_DIRS
                ${PORTAUDIO_INCLUDE_DIR}
                )
        set(PORTAUDIO_LIBRARIES
                ${PORTAUDIO_LIBRARY}
                )

        set(PORTAUDIO_LIBRARY_DIRS
                ${PORTAUDIO_LIBRARY_DIR}
                )

        set(PORTAUDIO_VERSION
                19
                )

        if (PORTAUDIO_INCLUDE_DIRS AND PORTAUDIO_LIBRARIES)
            set(PORTAUDIO_FOUND TRUE)
        endif (PORTAUDIO_INCLUDE_DIRS AND PORTAUDIO_LIBRARIES)

        if (PORTAUDIO_FOUND)
            if (NOT Portaudio_FIND_QUIETLY)
                message(STATUS "Found Portaudio: ${PORTAUDIO_LIBRARIES}")
            endif (NOT Portaudio_FIND_QUIETLY)
        else (PORTAUDIO_FOUND)
            if (Portaudio_FIND_REQUIRED)
                message(FATAL_ERROR "Could not find Portaudio")
            endif (Portaudio_FIND_REQUIRED)
        endif (PORTAUDIO_FOUND)
    endif (PORTAUDIO2_FOUND)


    # show the PORTAUDIO_INCLUDE_DIRS and PORTAUDIO_LIBRARIES variables only in the advanced view
    mark_as_advanced(PORTAUDIO_INCLUDE_DIRS PORTAUDIO_LIBRARIES)

endif (PORTAUDIO_LIBRARIES AND PORTAUDIO_INCLUDE_DIRS)


find_library(OPUS_LIBRARY NAMES opus libopus)
if(OPUS_LIBRARY)
    message(STATUS "Opus library found: " ${OPUS_LIBRARY})
    #Set header to default include path
    add_definitions(-DOPUS_HEADER="opus/opus.h")
endif()


find_path(OPUS_HEADER_PATH "opus.h" HINTS lib/opus/include)
if(OPUS_HEADER_PATH)
    message(STATUS "Compiling provided opus-library!")
    # Compile ourselves and set correct header-path
    add_subdirectory(lib/opus build/opus)
    add_definitions(-DOPUS_HEADER="../lib/opus/include/opus.h")
endif()
if(NOT OPUS_LIBRARY AND NOT OPUS_HEADER_PATH)
    message(WARNING "No opus library found")
endif()

find_package(CapnProto REQUIRED)

# git clone --branch main https://github.com/webmproject/libvpx.git
#--enable-libyuv \
#./configure --prefix=/usr/include --enable-shared --enable-libyuv --enable-vp8 --enable-vp9 --enable-postproc --enable-vp9-postproc --enable-vp9-highbitdepth --enable-vp9-temporal-denoising
# make
# make install
# ldconfig
###########################################################################


add_executable(vp8_full_duplex_capn main.cpp Proto/Packet.capnp.c++ Proto/Packet.capnp.h Serialization.h Audio/Audio.h Video/Encoder.h Video/Decoder.h chat_client.h video_call.h ThreadSafeDeque.h)

target_link_libraries(vp8_full_duplex_capn ${OpenCV_LIBS} ${VPX_LIBRARIES} ${USED_LIBS} ${PORTAUDIO_LIBRARIES} opus ${OPUS_LIBRARIES} CapnProto::capnp)
