#include <filesystem>
#include <gtest/gtest.h>
#include "../helpers/resources_helper.hpp"
#include "../../library/transcoder/transcoder.hpp"
#include "../../library/transcoder/transcoder_errors.hpp"
#include "../helpers/audio_helper.hpp"

// Включает режим генерации образцов
bool transcoder_generate_examples = false;

TEST(TranscoderTest, FileNotExists) {
    std::string output_path = "test-1.aac";
    std::string input_path = get_test_resource_path("transcoder", "test.iformat");
    EXPECT_EQ(transcoder_do_audio(input_path.c_str(), output_path.c_str(), 0, 0),
              TRANSCODER_MAYBE_FILE_NOT_FOUND);
    EXPECT_FALSE(std::filesystem::exists(output_path));
}

TEST(TranscoderTest, StreamsMoreThanOne) {
    std::string output_path = "test-1.aac";
    std::string input_path = get_test_resource_path("transcoder", "multiple.ogg");
    EXPECT_EQ(transcoder_do_audio(input_path.c_str(), output_path.c_str(), 0, 0),
              TRANSCODER_UNSUPPORTED_INPUT_FORMAT);
    EXPECT_FALSE(std::filesystem::exists(output_path));
}

TEST(TranscoderTest, FileAlreadyExists) {
    std::string output_path = get_test_resource_path("transcoder", "exists.mp3");
    std::string input_path = get_test_resource_path("transcoder", "test.ogg");
    EXPECT_EQ(transcoder_do_audio(input_path.c_str(), output_path.c_str(), 0, 0),
              TRANSCODER_MAYBE_FILE_ALREADY_EXIST);
    EXPECT_TRUE(std::filesystem::exists(output_path));
}

// Проверяет транскодирование
void check_transcoding_to_aac(const std::string &input_file,
                              const std::string &output_file,
                              int64_t start_moment_in_ms,
                              int64_t end_moment_in_ms) {
    std::string input_path = get_test_resource_path("transcoder", input_file);
    std::string output_file_name = output_file + "_transcoder_" + std::to_string(start_moment_in_ms) + "_"
        + std::to_string(end_moment_in_ms) + ".aac";
    std::string valid_path = get_file_path("transcoder", output_file_name, true);

    if (transcoder_generate_examples) {
        auto output_parent_path = std::filesystem::path(valid_path).parent_path();

        if (!std::filesystem::exists(output_parent_path)) {
            std::filesystem::create_directories(output_parent_path);
        }

        transcoder_do_audio(input_path.c_str(),
                            valid_path.c_str(),
                            start_moment_in_ms,
                            end_moment_in_ms);
        GTEST_FATAL_FAILURE_("Examples generation mode enabled!");
    }

    std::remove(output_file_name.c_str());
    int result = transcoder_do_audio(input_path.c_str(), output_file_name.c_str(), start_moment_in_ms,
                                     end_moment_in_ms);
    EXPECT_EQ(result, 0);
    EXPECT_TRUE(is_audio_files_matches(output_file_name, valid_path));
}

// Проверяет все варианты транскодирования в AAC
void check_all_variants_of_transcoding_to_aac(const std::string &input_file,
                                              const std::string &output_file) {
    check_transcoding_to_aac(input_file, output_file, 0, 0);
    check_transcoding_to_aac(input_file, output_file, 0, 12000);
    check_transcoding_to_aac(input_file, output_file, 13000, 0);
    check_transcoding_to_aac(input_file, output_file, 12000, 13000);
}

TEST(TranscoderTest, TranscodeOgg) {
    check_all_variants_of_transcoding_to_aac("test.ogg", "test_ogg");
}

TEST(TranscoderTest, TranscodeMp3) {
    check_all_variants_of_transcoding_to_aac("test.mp3", "test_mp3");
}

TEST(TranscoderTest, TranscodeWav) {
    check_all_variants_of_transcoding_to_aac("test.wav", "test_wav");
}