#ifndef FLUTTER_MEDIA_TOOLS_NATIVE_SRC_TESTS_HELPERS_RESOURCES_HELPER_HPP_
#define FLUTTER_MEDIA_TOOLS_NATIVE_SRC_TESTS_HELPERS_RESOURCES_HELPER_HPP_

#include <string>

extern "C" {
#include <libavutil/samplefmt.h>
}

// Возвращает путь до ресурса окружения
std::string get_file_path(const std::string &main_folder,
                          const std::string &path,
                          bool read_valid);

// Возвращает путь до ресурса в тестовом окружении
std::string get_test_resource_path(const std::string &main_folder, const std::string &path);

// Записывает образец в файл
bool write_example_to_file(const std::string &main_folder,
                           const std::string &path,
                           std::vector<std::vector<uint8_t>> &data,
                           AVSampleFormat format,
                           int channels_count);

// Читает матрицу из файла
std::vector<std::vector<uint8_t>> read_matrix_from_file(const std::string &path);

// Читает матрицу из файла
std::vector<std::vector<uint8_t>> read_matrix_from_file(const std::string &main_folder,
                                                        const std::string &path,
                                                        bool read_valid);

// Записывает матрицу в файл
bool write_matrix_to_file(const std::string &path, std::vector<std::vector<uint8_t>> &data,
                          bool write_length);

#endif //FLUTTER_MEDIA_TOOLS_NATIVE_SRC_TESTS_HELPERS_RESOURCES_HELPER_HPP_