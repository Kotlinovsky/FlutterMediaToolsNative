#include "../../library/decoder/decoder.hpp"
#include "../../library/decoder/decoder_errors.hpp"
#include "../helpers/resources_helper.hpp"

extern "C" {
#include <libavutil/channel_layout.h>
}

#include "gtest/gtest.h"
#include "../helpers/audio_helper.hpp"

// True, если нужно сгенерировать файлы шаблонов
bool decoder_generate_examples = false;

TEST(DecoderTest, InitializationWithEmptyStreamsList) {
    void *ctx_ref = nullptr;
    std::unordered_set<AVMediaType> streams_types{};

    EXPECT_EQ(decoder_init(&ctx_ref, nullptr, 0, 0, streams_types),
              DECODER_UNSUPPORTED_MEDIA_TYPE_ERROR);
    EXPECT_EQ(ctx_ref, nullptr);
}

TEST(DecoderTest, InitializationWithUnknownType) {
    void *ctx_ref = nullptr;
    std::unordered_set<AVMediaType> streams_types{AVMediaType::AVMEDIA_TYPE_UNKNOWN};

    EXPECT_EQ(decoder_init(&ctx_ref, nullptr, 0, 0, streams_types),
              DECODER_UNSUPPORTED_MEDIA_TYPE_ERROR);
    EXPECT_EQ(ctx_ref, nullptr);
}

TEST(DecoderTest, FileNotExists) {
    void *ctx_ref = nullptr;
    std::unordered_set<AVMediaType> streams_types{AVMediaType::AVMEDIA_TYPE_AUDIO};

    EXPECT_EQ(decoder_init(&ctx_ref, ".not_exists.mp3", 0, 0, streams_types),
              DECODER_INPUT_OPENING_ERROR);
    EXPECT_EQ(ctx_ref, nullptr);
}

TEST(DecoderTest, NotAllStreamsFound) {
    void *ctx_ref = nullptr;
    std::unordered_set<AVMediaType> streams_types{AVMediaType::AVMEDIA_TYPE_VIDEO};
    std::string path = get_test_resource_path("decoder", "test.ogg");

    EXPECT_EQ(decoder_init(&ctx_ref, path.c_str(), 0, 0, streams_types),
              DECODER_NOT_ALL_CODECS_FOUND_ERROR);
    EXPECT_EQ(ctx_ref, nullptr);
}

TEST(DecoderTest, UnexpectedErrorWhenCallbackReturnsFalse) {
    void *ctx_ref = nullptr;
    std::unordered_set<AVMediaType> streams_types{AVMediaType::AVMEDIA_TYPE_AUDIO};
    std::string path = get_test_resource_path("decoder", "test.ogg");
    EXPECT_EQ(decoder_init(&ctx_ref, path.c_str(), 0, 0, streams_types), 0);
    EXPECT_NE(ctx_ref, nullptr);

    // Проверяем, что если кэллбэк вернет false, то декодер выдаст ошибку
    int last_result = 0;

    while (last_result >= 0) {
        last_result = decoder_decode(ctx_ref, [](auto, auto, auto) { return false; });
    }

    EXPECT_EQ(last_result, DECODER_UNEXPECTED_ERROR);
    decoder_free(&ctx_ref);
}

// Проверяет декодирование файла
void check_decoding(const std::string &in_path,
                    const std::string &out_path,
                    int sample_rate,
                    int channels_count,
                    uint64_t channel_layout,
                    AVSampleFormat sample_format,
                    uint64_t duration,
                    int64_t start,
                    int64_t end) {
    void *ctx_ref = nullptr;
    std::unordered_set<AVMediaType> streams_types{AVMediaType::AVMEDIA_TYPE_AUDIO};
    std::string path = get_test_resource_path("decoder", in_path);
    EXPECT_EQ(decoder_init(&ctx_ref, path.c_str(), start, end, streams_types), 0);
    EXPECT_NE(ctx_ref, nullptr);

    // Сверяем параметры
    EXPECT_EQ(decoder_get_streams_count(ctx_ref), 1);
    EXPECT_EQ(decoder_get_sample_rate(ctx_ref, 0), sample_rate);
    EXPECT_EQ(decoder_get_channels_count(ctx_ref, 0), channels_count);
    EXPECT_EQ(decoder_get_channel_layout(ctx_ref, 0), channel_layout);
    EXPECT_EQ(decoder_get_sample_format(ctx_ref, 0), sample_format);
    EXPECT_EQ(decoder_get_duration_in_us(ctx_ref), duration);

    // Выделяем буффер
    int rows_count = av_sample_fmt_is_planar(decoder_get_sample_format(ctx_ref, 0))
                     ? decoder_get_channels_count(ctx_ref, 0) : 1;
    auto buffer = std::vector<std::vector<uint8_t>>(rows_count);

    // Декодируем и запоминаем последний результат
    int last_result = 0;

    while (last_result >= 0) {
        last_result = decoder_decode(ctx_ref, [&buffer, rows_count](auto data, size_t len, auto) {
          for (int i = 0; i < rows_count; i++) {
              buffer[i].insert(buffer[i].end(), data[i], data[i] + len);
          }

          return true;
        });
    }

    // Сверяем по каналам
    std::string example_name = out_path + "_" + std::to_string(start) + "_" + std::to_string(end);
    std::string example_path = example_name + ".pcm";

    if (decoder_generate_examples) {
        write_example_to_file("decoder", example_path, buffer, sample_format, channels_count);
        GTEST_FATAL_FAILURE_("Examples generation mode enabled!");
    } else {
        std::vector<std::vector<uint8_t>>
            example_buffer = read_matrix_from_file("decoder", example_path, true);
        EXPECT_TRUE(is_audio_matches(buffer, example_buffer, sample_format));
    }

    decoder_free(&ctx_ref);
    EXPECT_EQ(last_result, DECODER_END_OF_STREAM_ERROR);
}

// Полностью проверяет декодирование, даже с нарезкой
void fully_check_decoding(const std::string &in_path,
                          const std::string &out_path,
                          int sample_rate,
                          int channels_count,
                          uint64_t channel_layout,
                          AVSampleFormat sample_format,
                          uint64_t duration) {
    check_decoding(in_path,
                   out_path,
                   sample_rate,
                   channels_count,
                   channel_layout,
                   sample_format,
                   duration,
                   0, 0);

    check_decoding(in_path,
                   out_path,
                   sample_rate,
                   channels_count,
                   channel_layout,
                   sample_format,
                   duration,
                   0, 13000);

    check_decoding(in_path,
                   out_path,
                   sample_rate,
                   channels_count,
                   channel_layout,
                   sample_format,
                   duration,
                   13000, 0);

    check_decoding(in_path,
                   out_path,
                   sample_rate,
                   channels_count,
                   channel_layout,
                   sample_format,
                   duration,
                   12000, 13000);
}

TEST(DecoderTest, VorbisOggDecoding) {
    fully_check_decoding("test.ogg",
                         "test.ogg",
                         32000,
                         2,
                         AV_CH_LAYOUT_STEREO,
                         AVSampleFormat::AV_SAMPLE_FMT_FLTP,
                         74349219);
}

TEST(DecoderTest, Mp3Decoding) {
    fully_check_decoding("test.mp3",
                         "test.mp3",
                         32000,
                         2,
                         AV_CH_LAYOUT_STEREO,
                         AVSampleFormat::AV_SAMPLE_FMT_S16P,
                         74412000);
}

TEST(DecoderTest, WavDecoding) {
    fully_check_decoding("test.wav",
                         "test.wav",
                         32000,
                         2,
                         AV_CH_LAYOUT_STEREO,
                         AVSampleFormat::AV_SAMPLE_FMT_S16,
                         74349219);
}