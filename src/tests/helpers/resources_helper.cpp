#include <vector>
#include <fstream>
#include <filesystem>
#include "resources_helper.hpp"

std::string get_test_resource_path(const std::string &main_folder, const std::string &path) {
    return "../src/resources/" + main_folder + "/" + path;
}

bool write_example_to_file(const std::string &main_folder,
                           const std::string &path,
                           std::vector<std::vector<uint8_t>> &data,
                           AVSampleFormat format,
                           int channels_count) {
    auto casted_path = get_test_resource_path(main_folder, "");

    for (int i = 0; i < channels_count; ++i) {
        std::vector<std::vector<uint8_t>> channel_matrix;

        if (av_sample_fmt_is_planar(format)) {
            channel_matrix = {data[i]};
        } else {
            std::vector<uint8_t> channel;

            for (size_t j = i; j < data[0].size(); j += channels_count) {
                channel.push_back(data[0][j]);
            }

            channel_matrix = {channel};
        }

        auto example_path = casted_path + "/results/";
        example_path += path + ".ch-" + std::to_string(i + 1) + ".example";

        if (!write_matrix_to_file(example_path, channel_matrix, false)) {
            return false;
        }
    }

    return write_matrix_to_file(casted_path + "/valid/" + path, data, true);
}

std::string get_file_path(const std::string &main_folder,
                          const std::string &path,
                          bool read_valid) {
    std::string input_path = get_test_resource_path(main_folder, "");

    if (read_valid) {
        input_path += "valid/";
    }

    return input_path + path;
}

std::vector<std::vector<uint8_t>> read_matrix_from_file(const std::string &path) {
    std::ifstream input(path);

    if (input.bad()) {
        throw;
    }

    std::vector<std::vector<uint8_t>> buffer;
    size_t current_chunk_length;
    input >> std::noskipws;

    while (input >> current_chunk_length) {
        std::vector<uint8_t> row(current_chunk_length);

        for (size_t i = 0; i < current_chunk_length; ++i) {
            input >> row[i];
        }

        buffer.push_back(row);
    }

    return buffer;
}

std::vector<std::vector<uint8_t>> read_matrix_from_file(const std::string &main_folder,
                                                        const std::string &path,
                                                        bool read_valid) {
    return read_matrix_from_file(get_file_path(main_folder, path, read_valid));
}

bool write_matrix_to_file(const std::string &path,
                          std::vector<std::vector<uint8_t>> &data,
                          bool write_length) {
    auto parent_path = std::filesystem::path(path).parent_path();

    if (!std::filesystem::exists(parent_path)) {
        if (!std::filesystem::create_directories(parent_path)) {
            return false;
        }
    }

    std::ofstream file(path, std::ios::binary);

    for (auto &row : data) {
        if (write_length) {
            if (!(file << row.size())) {
                return false;
            }
        }

        for (uint8_t current_char : row) {
            if (!(file << current_char)) {
                return false;
            }
        }
    }

    file.close();
    return true;
}