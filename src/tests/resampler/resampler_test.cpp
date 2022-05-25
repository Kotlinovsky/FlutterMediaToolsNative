#include <gtest/gtest.h>
#include <fstream>

extern "C" {
#include <libavutil/channel_layout.h>
}

#include "../../library/resampler/resampler.hpp"
#include "../helpers/resources_helper.hpp"

// Включает режим генерации образцов
bool resampler_generate_examples = false;

TEST(ResamplerTest, InitAndFree) {
    void *context = nullptr;
    int result = resampler_init(&context,
                                AV_CH_LAYOUT_STEREO,
                                AV_CH_LAYOUT_STEREO,
                                AVSampleFormat::AV_SAMPLE_FMT_FLTP,
                                AVSampleFormat::AV_SAMPLE_FMT_FLTP,
                                32000,
                                32000,
                                2);
    EXPECT_EQ(result, 0);
    EXPECT_NE(context, nullptr);

    resampler_free(&context);
    EXPECT_EQ(context, nullptr);
}

// Тестирует ресемплинг
void test_resampling(const std::string &input_path,
                     const std::string &output_path,
                     AVSampleFormat input_format,
                     AVSampleFormat output_format,
                     int channels_count) {
    auto buffer = read_matrix_from_file("resampler", input_path, false);

    // Ресемплим
    void *context = nullptr;
    resampler_init(&context,
                   AV_CH_LAYOUT_STEREO,
                   AV_CH_LAYOUT_STEREO,
                   input_format,
                   output_format,
                   32000,
                   32000,
                   channels_count);

    size_t maximal_chunk_length = 1024;
    size_t channel_length = buffer[0].size();
    std::vector<std::vector<uint8_t>> result(channels_count);

    for (size_t i = 0; i < channel_length; i += maximal_chunk_length) {
        size_t chunk_size = std::min(maximal_chunk_length, channel_length - i);
        size_t output_size = resampler_get_need_bytes_count(context, (int) chunk_size);
        auto **in_temp_buffer = new uint8_t *[buffer.size()];
        auto **out_temp_buffer = new uint8_t *[channels_count];

        // Инициализируем векторы
        for (int channel = 0; channel < channels_count; channel++) {
            out_temp_buffer[channel] = new uint8_t[output_size];
        }

        for (size_t channel = 0; channel < buffer.size(); channel++) {
            in_temp_buffer[channel] = new uint8_t[chunk_size];

            for (size_t j = 0; j < chunk_size; j++) {
                in_temp_buffer[channel][j] = buffer[channel][i + j];
            }
        }

        // Теперь заполняем
        int bytes_count = resampler_resample(context,
                                             const_cast<const uint8_t **>(in_temp_buffer),
                                             (int) chunk_size,
                                             out_temp_buffer);

        if (bytes_count >= 0) {
            for (int j = 0; j < bytes_count; ++j) {
                for (int channel = 0; channel < channels_count; channel++) {
                    result[channel].push_back(out_temp_buffer[channel][j]);
                }
            }
        }

        // Освобождает использованную память
        for (int channel = 0; channel < channels_count; channel++) {
            delete[] out_temp_buffer[channel];
        }

        for (size_t channel = 0; channel < buffer.size(); channel++) {
            delete[] in_temp_buffer[channel];
        }

        delete[] out_temp_buffer;
        delete[] in_temp_buffer;

        // Проверяем количество байт
        ASSERT_EQ(bytes_count, output_size);
    }

    // Читаем и сверяем
    if (resampler_generate_examples) {
        write_example_to_file("resampler", output_path, result, output_format, channels_count);
        resampler_free(&context);
        GTEST_FATAL_FAILURE_("Examples generation mode enabled!");
    } else {
        auto valid_buffer = read_matrix_from_file("resampler", output_path, true);
        resampler_free(&context);
        EXPECT_EQ(result, valid_buffer);
    }
}

TEST(ResamplerTest, PlanarToPlanarSampling) {
    test_resampling("planar.pcm",
                    "valid-planar.pcm",
                    AV_SAMPLE_FMT_FLTP,
                    AV_SAMPLE_FMT_S32P,
                    2);
}

TEST(ResamplerTest, NonPlanarToPlanarSampling) {
    test_resampling("nonplanar.pcm",
                    "valid-nonplanar-to-planar.pcm",
                    AV_SAMPLE_FMT_S16,
                    AV_SAMPLE_FMT_FLTP,
                    2);
}