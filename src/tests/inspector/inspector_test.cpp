#include <gtest/gtest.h>
#include "../../library/inspector/inspector.hpp"
#include "../../library/inspector/inspector_errors.hpp"
#include "../helpers/resources_helper.hpp"

TEST(InspectorTest, FileNotExists) {
    EXPECT_EQ(inspector_get_audio_duration_in_us(".not_exists"), INSPECTOR_MAYBE_FILE_NOT_FOUND);
}

TEST(InspectorTest, OggDuration) {
    std::string path = get_test_resource_path("inspector", "test.ogg");
    EXPECT_EQ(inspector_get_audio_duration_in_us(path.c_str()), 74349219);
}

TEST(InspectorTest, WavDuration) {
    std::string path = get_test_resource_path("inspector", "test.wav");
    EXPECT_EQ(inspector_get_audio_duration_in_us(path.c_str()), 74349219);
}

TEST(InspectorTest, Mp3Duration) {
    std::string path = get_test_resource_path("inspector", "test.mp3");
    EXPECT_EQ(inspector_get_audio_duration_in_us(path.c_str()), 74412000);
}

TEST(InspectorTest, AacDuration) {
    std::string path = get_test_resource_path("inspector", "test.aac");
    EXPECT_EQ(inspector_get_audio_duration_in_us(path.c_str()), 74368000);
}