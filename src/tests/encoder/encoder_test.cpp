#include <filesystem>
#include <fstream>
#include "gtest/gtest.h"
#include "../helpers/resources_helper.hpp"
#include "../../library/encoder/encoder.hpp"
#include "../../library/encoder/encoder_errors.hpp"

extern "C" {
#include <libavutil/channel_layout.h>
}

// Включает режим генерации образцов
bool generate_examples = false;

TEST(EncoderTest, StreamsListEmpty) {
    void *context = nullptr;
    std::map<int, encoder_stream_cfg> streams;
    EXPECT_EQ(encoder_init(&context, nullptr, streams), ENCODER_STREAMS_LIST_EMPTY_ERROR);
    EXPECT_EQ(context, nullptr);
}

TEST(EncoderTest, FileAlreadyExists) {
    auto cfg = new encoder_stream_audio_codec_cfg{32000, 2, AV_CH_LAYOUT_STEREO,
                                                  AVSampleFormat::AV_SAMPLE_FMT_FLTP};
    std::map<int, encoder_stream_cfg> streams{{0, encoder_stream_cfg{AV_CODEC_ID_AAC, cfg}}};
    std::string path = get_test_resource_path("encoder", "test.ogg");

    void *context = nullptr;
    EXPECT_EQ(encoder_init(&context, path.c_str(), streams), ENCODER_OUTPUT_STREAM_ERROR);
    EXPECT_EQ(context, nullptr);
    delete cfg;
}

TEST(EncoderTest, CodecNotFound) {
    auto cfg = new encoder_stream_audio_codec_cfg{32000, 2, AV_CH_LAYOUT_STEREO,
                                                  AVSampleFormat::AV_SAMPLE_FMT_FLTP};
    std::map<int, encoder_stream_cfg> streams{{0, encoder_stream_cfg{AV_CODEC_ID_MP2, cfg}}};

    void *context = nullptr;
    EXPECT_EQ(encoder_init(&context, "test.aac", streams), ENCODER_CODECS_INITIALIZATION_ERROR);
    EXPECT_EQ(context, nullptr);
    EXPECT_FALSE(std::filesystem::exists("test.aac"));
    delete cfg;
}

TEST(EncoderTest, SuccessInitAndFree) {
    auto cfg = new encoder_stream_audio_codec_cfg{32000, 2, AV_CH_LAYOUT_STEREO,
                                                  AVSampleFormat::AV_SAMPLE_FMT_FLTP};
    std::map<int, encoder_stream_cfg> streams{{0, encoder_stream_cfg{AV_CODEC_ID_AAC, cfg}}};
    std::string path = "test.aac";

    void *context = nullptr;
    EXPECT_EQ(encoder_init(&context, path.c_str(), streams), 0);
    EXPECT_NE(context, nullptr);

    // Проверяем параметры
    EXPECT_EQ(cfg->channels_count, 2);
    EXPECT_EQ(cfg->sample_rate, 32000);
    EXPECT_EQ(cfg->sample_format, AVSampleFormat::AV_SAMPLE_FMT_FLTP);
    EXPECT_EQ(cfg->channel_layout, AV_CH_LAYOUT_STEREO);
    EXPECT_EQ(encoder_get_samples_count_per_frame(context, 0), 1024);
    EXPECT_EQ(encoder_get_bytes_per_sample_count(context, 0), 4);
    delete cfg;

    encoder_free(&context);
    EXPECT_EQ(context, nullptr);
    EXPECT_FALSE(std::filesystem::exists(path));
}

TEST(EncoderTest, MaximalSampleRateSelecting) {
    auto cfg = new encoder_stream_audio_codec_cfg{32001, 2, AV_CH_LAYOUT_STEREO,
                                                  AVSampleFormat::AV_SAMPLE_FMT_FLTP};
    std::map<int, encoder_stream_cfg> streams{{0, encoder_stream_cfg{AV_CODEC_ID_AAC, cfg}}};
    std::string path = "test.aac";

    void *context = nullptr;
    encoder_init(&context, path.c_str(), streams);
    EXPECT_EQ(cfg->sample_rate, 44100);

    delete cfg;
    encoder_free(&context);
    EXPECT_FALSE(std::filesystem::exists(path));
}

TEST(EncoderTest, LessSampleRateSelecting) {
    auto cfg = new encoder_stream_audio_codec_cfg{96001, 2, AV_CH_LAYOUT_STEREO,
                                                  AVSampleFormat::AV_SAMPLE_FMT_FLTP};
    std::map<int, encoder_stream_cfg> streams{{0, encoder_stream_cfg{AV_CODEC_ID_AAC, cfg}}};
    std::string path = "test.aac";

    void *context = nullptr;
    encoder_init(&context, path.c_str(), streams);
    EXPECT_EQ(cfg->sample_rate, 88200);

    delete cfg;
    encoder_free(&context);
    EXPECT_FALSE(std::filesystem::exists(path));
}

TEST(EncoderTest, SampleFormatNotSupported) {
    auto cfg = new encoder_stream_audio_codec_cfg{32000, 2, AV_CH_LAYOUT_STEREO,
                                                  AVSampleFormat::AV_SAMPLE_FMT_U8};
    std::map<int, encoder_stream_cfg> streams{{0, encoder_stream_cfg{AV_CODEC_ID_AAC, cfg}}};
    std::string path = "test.aac";

    void *context = nullptr;
    encoder_init(&context, path.c_str(), streams);
    EXPECT_EQ(cfg->sample_format, AVSampleFormat::AV_SAMPLE_FMT_FLTP);

    delete cfg;
    encoder_free(&context);
    EXPECT_FALSE(std::filesystem::exists(path));
}

TEST(EncoderTest, AacEncoding) {
    void *context = nullptr;
    auto cfg = new encoder_stream_audio_codec_cfg{32000, 2, AV_CH_LAYOUT_STEREO,
                                                  AVSampleFormat::AV_SAMPLE_FMT_FLTP};
    auto path = generate_examples ? get_file_path("encoder", "test.aac", true) : "test-res.aac";
    std::filesystem::remove(path);

    if (generate_examples && !std::filesystem::exists(path)) {
        std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    }

    std::map<int, encoder_stream_cfg> streams{{0, encoder_stream_cfg{AV_CODEC_ID_AAC, cfg}}};
    encoder_init(&context, path.c_str(), streams);

    // Кодируем ...
    std::vector<std::vector<uint8_t>>
        buffer = read_matrix_from_file("encoder", "test.ogg.pcm", false);
    size_t full_length = buffer[0].size();
    size_t encoded_count = 0;
    int last_encode_result = 0;

    while (encoded_count < full_length && last_encode_result >= 0) {
        size_t need = encoder_get_samples_count_per_frame(context, 0)
            * encoder_get_bytes_per_sample_count(context, 0);
        size_t chunk_size = std::min(need, full_length - encoded_count);
        auto chunk = new uint8_t *[buffer.size()];

        for (size_t j = 0; j < buffer.size(); j++) {
            chunk[j] = new uint8_t[chunk_size];
            memcpy(chunk[j], buffer[j].data() + encoded_count, chunk_size);
        }

        last_encode_result = encoder_encode(context, 0, chunk, chunk_size);

        if (last_encode_result >= 0) {
            encoded_count += chunk_size;
        }

        for (size_t j = 0; j < buffer.size(); j++) {
            delete[] chunk[j];
        }

        delete[] chunk;
    }

    EXPECT_EQ(last_encode_result, 0);
    EXPECT_EQ(encoder_finish_encode(context), 0);

    delete cfg;
    encoder_free(&context);
    EXPECT_TRUE(std::filesystem::exists(path));

    if (generate_examples) {
        GTEST_FATAL_FAILURE_("Examples generation mode enabled!");
    }

    // Сначала прочитаем правильный файл
    std::vector<uint8_t> example_buffer = read_array_from_file("encoder", "test.aac", true);
    std::vector<uint8_t> result_buffer = read_array_from_file(path);
    EXPECT_EQ(example_buffer, result_buffer);
}