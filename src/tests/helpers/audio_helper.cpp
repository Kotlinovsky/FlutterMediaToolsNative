#include <vector>
#include <bit>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

#include "audio_helper.hpp"

// Декодирует аудио в матрицу
std::vector<std::vector<uint8_t>> decode_audio(const std::string &file_path,
                                               AVSampleFormat *format) {
    AVFormatContext *format_ctx = avformat_alloc_context();

    if (avformat_open_input(&format_ctx, file_path.c_str(), nullptr, nullptr) < 0) {
        avformat_free_context(format_ctx);
        throw;
    }

    if (avformat_find_stream_info(format_ctx, nullptr) < 0) {
        avformat_close_input(&format_ctx);
        throw;
    }

    AVStream *stream = format_ctx->streams[0];
    const AVCodec *decoder = avcodec_find_decoder(stream->codecpar->codec_id);
    AVCodecContext *decoder_context = avcodec_alloc_context3(decoder);

    if (decoder_context == nullptr) {
        avcodec_free_context(&decoder_context);
        avformat_close_input(&format_ctx);
        throw;
    }

    decoder_context->pkt_timebase = stream->time_base;

    if (avcodec_parameters_to_context(decoder_context, stream->codecpar) < 0
        || avcodec_open2(decoder_context, decoder, nullptr) < 0) {
        avcodec_free_context(&decoder_context);
        avformat_close_input(&format_ctx);
        throw;
    }

    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    std::vector<std::vector<uint8_t>> buffer;

    if (decoder_context->codec->type == AVMEDIA_TYPE_AUDIO) {
        if (av_sample_fmt_is_planar(decoder_context->sample_fmt)) {
            buffer.resize(decoder_context->channels);
        } else {
            buffer.resize(1);
        }
    }

    while (true) {
        int read_frame_result = av_read_frame(format_ctx, packet);

        if (read_frame_result == AVERROR_EOF) {
            break;
        } else if (read_frame_result < 0) {
            throw;
        }

        if (avcodec_send_packet(decoder_context, packet) < 0) {
            av_packet_unref(packet);
            throw;
        }

        av_packet_unref(packet);

        while (true) {
            int result = avcodec_receive_frame(decoder_context, frame);

            if (result < 0) {
                if (result == AVERROR(EAGAIN)) {
                    break;
                }

                avcodec_free_context(&decoder_context);
                avformat_close_input(&format_ctx);
                av_frame_unref(frame);
                throw;
            }

            if (decoder_context->codec->type == AVMEDIA_TYPE_AUDIO) {
                int bytes_per_sample = av_get_bytes_per_sample(decoder_context->sample_fmt);

                if (!av_sample_fmt_is_planar(decoder_context->sample_fmt)) {
                    size_t bytes_count = frame->nb_samples * bytes_per_sample * frame->channels;

                    for (size_t j = 0; j < bytes_count; ++j) {
                        buffer[0].push_back(frame->data[0][j]);
                    }
                } else {
                    size_t bytes_count = frame->nb_samples * bytes_per_sample;

                    for (int j = 0; j < decoder_context->channels; ++j) {
                        for (size_t k = 0; k < bytes_count; ++k) {
                            buffer[j].push_back(frame->data[j][k]);
                        }
                    }
                }

                av_frame_unref(frame);
            }
        }
    }

    *format = decoder_context->sample_fmt;
    avcodec_free_context(&decoder_context);
    avformat_close_input(&format_ctx);
    av_packet_unref(packet);
    av_frame_unref(frame);

    return buffer;
}

bool is_audio_matches(std::vector<std::vector<uint8_t>> &test,
                      std::vector<std::vector<uint8_t>> &valid,
                      AVSampleFormat format) {
    if (test.size() != valid.size()) {
        return false;
    }

    for (size_t i = 0; i < test.size(); ++i) {
        if (test[i].size() != valid[i].size()) {
            return false;
        }
    }

    if (format == AV_SAMPLE_FMT_DBL || format == AV_SAMPLE_FMT_DBLP) {
        throw;
    } else if (format != AV_SAMPLE_FMT_FLT && format != AV_SAMPLE_FMT_FLTP) {
        return test == valid;
    }

    int chunk_len = av_get_bytes_per_sample(format);

    // Сравниваем нецелочисленные типы с погрешностью ;)
    for (size_t i = 0; i < test.size(); ++i) {
        for (size_t j = 0; j < test[0].size(); j += chunk_len) {
            float test_number = 0;
            float valid_number = 0;

            for (int k = 0; k < chunk_len; ++k) {
                ((uint8_t *) &test_number)[k] = test[i][j + k];
                ((uint8_t *) &valid_number)[k] = valid[i][j + k];
            }

            if (std::abs(test_number - valid_number) >= 1) {
                return false;
            }
        }
    }

    return true;
}

bool is_audio_files_matches(const std::string &test_file_path,
                            const std::string &valid_file_path) {
    AVSampleFormat sample_format;
    auto test_buffer = decode_audio(test_file_path, &sample_format);
    auto valid_buffer = decode_audio(valid_file_path, &sample_format);

    if (test_buffer.size() != valid_buffer.size()) {
        return false;
    }

    for (size_t i = 0; i < test_buffer.size(); ++i) {
        if (test_buffer[i].size() != valid_buffer[i].size()) {
            return false;
        }
    }

    int bytes = av_get_bytes_per_sample(sample_format);

    for (size_t i = 0; i < std::min(test_buffer[0].size(), valid_buffer[0].size()); i += bytes) {
        float test_number = 0;
        float valid_number = 0;

        for (int j = 0; j < bytes; ++j) {
            ((uint8_t *) &test_number)[j] = test_buffer[0][i + j];
            ((uint8_t *) &valid_number)[j] = valid_buffer[0][i + j];
        }

        if (std::abs(test_number - valid_number) >= 1) {
            return false;
        }
    }

    return true;
}