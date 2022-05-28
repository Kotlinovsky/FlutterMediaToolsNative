#ifndef FLUTTER_MEDIA_TOOLS_NATIVE_SRC_TESTS_HELPERS_AUDIO_HELPER_HPP_
#define FLUTTER_MEDIA_TOOLS_NATIVE_SRC_TESTS_HELPERS_AUDIO_HELPER_HPP_

#include <string>

// Проверяет схожесть двух аудио-файлов в PCM
bool is_audio_matches(std::vector<std::vector<uint8_t>> &test,
                      std::vector<std::vector<uint8_t>> &valid,
                      AVSampleFormat format);

// Проверяет схожесть двух .wav или .pcm файлов
bool is_audio_files_matches(const std::string &test_file_path, const std::string &valid_file_path);

#endif //FLUTTER_MEDIA_TOOLS_NATIVE_SRC_TESTS_HELPERS_AUDIO_HELPER_HPP_
