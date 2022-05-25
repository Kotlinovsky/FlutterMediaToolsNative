cmake_minimum_required(VERSION 3.23)
project(flutter_media_tools_native)
set(CMAKE_SHARED_LINKER_FLAGS "-Wl,-Bsymbolic")
set(CMAKE_CXX_STANDARD 20)

IF (UNIX)
    SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
    SET(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")

    EXECUTE_PROCESS(
            COMMAND ./configure
            --prefix="${CMAKE_CURRENT_SOURCE_DIR}/deps-tmp/linux/ffmpeg"
            --disable-everything
            --disable-network
            --disable-autodetect
            --enable-demuxer=wav,ogg,mp3,aac
            --enable-decoder=pcm*,opus,vorbis,flac,mp3,aac
            --enable-muxer=adts
            --enable-encoder=aac
            --enable-filter=aresample
            --enable-protocol=file
            --disable-doc
            --disable-ffmpeg
            --disable-ffplay
            --disable-ffprobe
            --disable-vaapi
            --disable-vdpau
            --disable-zlib
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/FFmpeg/)
    EXECUTE_PROCESS(COMMAND make -j install WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/FFmpeg/)
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
        ${CMAKE_CURRENT_SOURCE_DIR}/deps-tmp/linux/ffmpeg/lib/libavcodec.a
        ${CMAKE_CURRENT_SOURCE_DIR}/deps-tmp/linux/ffmpeg/lib/libavdevice.a
        ${CMAKE_CURRENT_SOURCE_DIR}/deps-tmp/linux/ffmpeg/lib/libavfilter.a
        ${CMAKE_CURRENT_SOURCE_DIR}/deps-tmp/linux/ffmpeg/lib/libavformat.a
        ${CMAKE_CURRENT_SOURCE_DIR}/deps-tmp/linux/ffmpeg/lib/libavutil.a
        ${CMAKE_CURRENT_SOURCE_DIR}/deps-tmp/linux/ffmpeg/lib/libswresample.a
        ${CMAKE_CURRENT_SOURCE_DIR}/deps-tmp/linux/ffmpeg/lib/libswscale.a)
target_include_directories(flutter_media_tools_native INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/deps-tmp/linux/ffmpeg/include)

# Tests
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
        src/tests/main.cpp)
target_include_directories(flutter_media_tools_native_tests INTERFACE ${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
target_link_libraries(flutter_media_tools_native_tests flutter_media_tools_native gtest gtest_main)
# End tests