cmake_minimum_required(VERSION 3.23)
project(flutter_media_tools_native)
set(CMAKE_SHARED_LINKER_FLAGS "-Wl,-Bsymbolic")
set(CMAKE_CXX_STANDARD 20)

message("Target system name: ${CMAKE_SYSTEM_NAME}")
message("Target system architecture: ${CMAKE_SYSTEM_PROCESSOR}")
message("Build type: ${CMAKE_BUILD_TYPE}")

# Compile FFmpeg
IF (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
    set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")

    execute_process(
            COMMAND ./configure
            --prefix="${CMAKE_CURRENT_SOURCE_DIR}/deps-tmp/linux/ffmpeg"
            --disable-everything
            --disable-autodetect
            --disable-network
            --enable-demuxer=wav,ogg,mp3,aac
            --enable-decoder=pcm*,opus,vorbis,flac,mp3,aac
            --enable-muxer=adts
            --enable-encoder=aac
            --enable-filter=aresample
            --enable-protocol=file
            --disable-ffmpeg
            --disable-ffplay
            --disable-ffprobe
            --disable-doc
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/FFmpeg/)
    execute_process(COMMAND make -j install WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/FFmpeg/)
ENDIF ()

add_library(flutter_media_tools_native SHARED
        src/library/decoder/decoder.cpp
        src/library/decoder/decoder.hpp
        src/library/decoder/decoder_errors.hpp
        src/library/decoder/decoder_context.hpp
        src/library/encoder/encoder.cpp
        src/library/encoder/encoder.hpp
        src/library/encoder/encoder_errors.hpp
        src/library/encoder/encoder_context.hpp
        src/library/encoder/encoder_stream_config.hpp
        src/library/resampler/resampler.cpp
        src/library/resampler/resampler.hpp
        src/library/resampler/resampler_context.hpp
        src/library/resampler/resampler_errors.hpp
        src/library/transcoder/transcoder.cpp
        src/library/transcoder/transcoder.hpp
        src/library/transcoder/transcoder_errors.hpp
        src/library/buffer/buffer.cpp
        src/library/buffer/buffer.hpp
        src/library/buffer/internal/buffer_context.hpp
        src/library/inspector/inspector.cpp
        src/library/inspector/inspector.hpp
        src/library/inspector/inspector_errors.hpp)
target_link_libraries(flutter_media_tools_native
        ${CMAKE_CURRENT_SOURCE_DIR}/deps-tmp/linux/ffmpeg/lib/libavformat.a
        ${CMAKE_CURRENT_SOURCE_DIR}/deps-tmp/linux/ffmpeg/lib/libavcodec.a
        ${CMAKE_CURRENT_SOURCE_DIR}/deps-tmp/linux/ffmpeg/lib/libavdevice.a
        ${CMAKE_CURRENT_SOURCE_DIR}/deps-tmp/linux/ffmpeg/lib/libavfilter.a
        ${CMAKE_CURRENT_SOURCE_DIR}/deps-tmp/linux/ffmpeg/lib/libavutil.a
        ${CMAKE_CURRENT_SOURCE_DIR}/deps-tmp/linux/ffmpeg/lib/libswresample.a
        ${CMAKE_CURRENT_SOURCE_DIR}/deps-tmp/linux/ffmpeg/lib/libswscale.a)
target_include_directories(flutter_media_tools_native INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/deps-tmp/linux/ffmpeg/include)

# Stripping
add_custom_command(TARGET flutter_media_tools_native
                   POST_BUILD COMMAND ${CMAKE_STRIP} $<TARGET_FILE:flutter_media_tools_native>)

# Tests
IF (${CMAKE_BUILD_TYPE} MATCHES "Debug")
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/deps/googletest)
add_executable(flutter_media_tools_native_tests
        src/tests/buffer/buffer_test.cpp
        src/tests/decoder/decoder_test.cpp
        src/tests/encoder/encoder_test.cpp
        src/tests/resampler/resampler_test.cpp
        src/tests/transcoder/transcoder_test.cpp
        src/tests/inspector/inspector_test.cpp
        src/tests/helpers/resources_helper.cpp
        src/tests/helpers/resources_helper.hpp
        src/tests/helpers/audio_helper.cpp
        src/tests/helpers/audio_helper.hpp
        src/tests/main.cpp)
target_include_directories(flutter_media_tools_native_tests INTERFACE ${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
target_link_libraries(flutter_media_tools_native_tests flutter_media_tools_native gtest gtest_main)
ENDIF ()
# End tests