#include <filesystem>
#include <unordered_map>
#include <algorithm>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
}

#include "encoder.hpp"
#include "encoder_context.hpp"
#include "encoder_errors.hpp"

// Создает таблицу с энкодерами
bool build_encoders_map(std::map<int, encoder_stream_cfg> &codec_ids,
                        std::unordered_map<int, const AVCodec *> &encoders_map) {
    for (const auto &item : codec_ids) {
        auto encoder = avcodec_find_encoder(item.second.codec_id);

        if (encoder == nullptr) {
            return false;
        }

        encoders_map[item.first] = encoder;
    }

    return true;
}

// Выбирает частоту дискретизации
void select_sample_rate(encoder_stream_audio_codec_cfg *cfg,
                        const int *supported_sample_rates) {
    if (supported_sample_rates == nullptr) {
        return;
    }

    int requested_sample_rate = cfg->sample_rate;
    int min_sample_rate = 0;
    int max_sample_rate = 0;

    while (*supported_sample_rates++ != 0) {
        if (*supported_sample_rates == requested_sample_rate) {
            return;
        }

        if (*supported_sample_rates < requested_sample_rate) {
            min_sample_rate = std::max(*supported_sample_rates, min_sample_rate);
        }

        if (*supported_sample_rates > requested_sample_rate) {
            if (max_sample_rate == 0) {
                max_sample_rate = *supported_sample_rates;
            } else {
                max_sample_rate = std::min(*supported_sample_rates, max_sample_rate);
            }
        }
    }

    if (max_sample_rate == 0) {
        cfg->sample_rate = min_sample_rate;
    } else {
        cfg->sample_rate = max_sample_rate;
    }
}

// Выбирает количество каналов и их формат
void select_channel_layout(encoder_stream_audio_codec_cfg *cfg,
                           const uint64_t *supported_channel_layouts) {
    if (supported_channel_layouts == nullptr) {
        return;
    }

    std::map<int, std::vector<uint64_t>> layouts;

    while (*supported_channel_layouts++ != 0) {
        int channels_count = av_get_channel_layout_nb_channels(*supported_channel_layouts);
        layouts[channels_count].push_back(*supported_channel_layouts);
    }

    if (layouts.contains(cfg->channels_count)) {
        if (std::find(layouts[cfg->channels_count].begin(),
                      layouts[cfg->channels_count].end(),
                      cfg->channel_layout) == layouts[cfg->channels_count].end()) {
            return;
        }

        cfg->channel_layout = layouts[cfg->channels_count][0];
    } else {
        std::vector<int> channels_count;

        for (const auto &item : layouts) {
            channels_count.push_back(item.first);
        }

        std::sort(channels_count.begin(), channels_count.end());

        int min_channels_count = 0;
        int max_channels_count = 0;

        for (const auto &item : channels_count) {
            if (item < cfg->channels_count) {
                min_channels_count = item;
            } else if (item > cfg->channels_count) {
                max_channels_count = item;
            }
        }

        if (max_channels_count != 0) {
            cfg->channel_layout = layouts[max_channels_count][0];
            cfg->channels_count = max_channels_count;
        } else {
            cfg->channel_layout = layouts[min_channels_count][0];
            cfg->channels_count = min_channels_count;
        }
    }
}

// Выбирает формат записи
void select_sample_fmt(encoder_stream_audio_codec_cfg *cfg,
                       const enum AVSampleFormat *sample_fmts) {
    if (sample_fmts == nullptr) {
        return;
    }

    AVSampleFormat first_sample_fmt = sample_fmts[0];

    while (*sample_fmts++ != -1) {
        if (*sample_fmts == cfg->sample_format) {
            return;
        }
    }

    cfg->sample_format = first_sample_fmt;
}

// Настраивает контекст энкодера
bool configure_encoder_context(const AVCodec *codec,
                               AVCodecContext *context,
                               encoder_stream_cfg &cfg) {
    if (context->codec->type == AVMEDIA_TYPE_AUDIO) {
        auto stream_cfg = static_cast<encoder_stream_audio_codec_cfg *>(cfg.cfg);
        select_sample_rate(stream_cfg, context->codec->supported_samplerates);
        select_channel_layout(stream_cfg, context->codec->channel_layouts);
        select_sample_fmt(stream_cfg, context->codec->sample_fmts);

        context->sample_rate = stream_cfg->sample_rate;
        context->channel_layout = stream_cfg->channel_layout;
        context->channels = stream_cfg->channels_count;
        context->sample_fmt = stream_cfg->sample_format;
        context->time_base = AVRational{1, stream_cfg->sample_rate};
    }

    if (avcodec_open2(context, codec, nullptr) < 0) {
        return false;
    }

    return true;
}

// Создает таблицу контекстов энкодеров
bool build_encoders_contexts_map(std::unordered_map<int, const AVCodec *> &encoders_map,
                                 std::unordered_map<int, AVCodecContext *> &contexts_map,
                                 std::map<int, encoder_stream_cfg> &streams) {
    bool result = true;

    for (const auto &item : encoders_map) {
        auto context = avcodec_alloc_context3(item.second);

        if (context == nullptr
            || !configure_encoder_context(item.second, context, streams[item.first])) {
            result = false;
            break;
        }

        contexts_map[item.first] = context;
    }

    if (!result) {
        for (auto &item : contexts_map) {
            avcodec_free_context(&item.second);
        }
    }

    return result;
}

// Создает таблицу потоков
bool build_streams_map(AVFormatContext *format_ctx,
                       std::unordered_map<int, const AVCodec *> &codecs_map,
                       std::unordered_map<int, AVCodecContext *> &codecs_contexts_map,
                       std::unordered_map<int, AVStream *> &streams_map) {
    for (const auto &item : codecs_contexts_map) {
        AVStream *stream = avformat_new_stream(format_ctx, codecs_map[item.first]);
        AVCodecContext *codec_context = codecs_contexts_map[item.first];

        if (stream == nullptr
            || avcodec_parameters_from_context(stream->codecpar, codec_context) < 0) {
            return false;
        }

        stream->time_base = codec_context->time_base;
        streams_map[item.first] = stream;
    }

    return true;
}

int encoder_init(void **ctx_ref, const char *path, std::map<int, encoder_stream_cfg> &streams) {
    if (streams.empty()) {
        return ENCODER_STREAMS_LIST_EMPTY_ERROR;
    }

    AVFormatContext *format_ctx;

    if (avformat_alloc_output_context2(&format_ctx, nullptr, nullptr, path) < 0) {
        return ENCODER_OUTPUT_STREAM_ERROR;
    }

    std::unordered_map<int, const AVCodec *> codecs_map;
    std::unordered_map<int, AVCodecContext *> codecs_contexts_map;
    std::unordered_map<int, AVStream *> streams_map;

    if (!build_encoders_map(streams, codecs_map)
        || !build_encoders_contexts_map(codecs_map, codecs_contexts_map, streams)
        || !build_streams_map(format_ctx, codecs_map, codecs_contexts_map, streams_map)) {
        avformat_free_context(format_ctx);
        return ENCODER_CODECS_INITIALIZATION_ERROR;
    }

    if (avio_open(&format_ctx->pb, path, AVIO_FLAG_WRITE) < 0) {
        avformat_free_context(format_ctx);
        return ENCODER_OUTPUT_STREAM_ERROR;
    }

    if (avformat_write_header(format_ctx, nullptr) < 0) {
        avio_closep(&format_ctx->pb);
        avformat_free_context(format_ctx);
        std::remove(path);
        return ENCODER_OUTPUT_STREAM_ERROR;
    }

    auto encoder_streams_ctxs = new std::unordered_map<int, encoder_stream_ctx *>;

    for (const auto &item : codecs_map) {
        const AVCodec *codec = codecs_map[item.first];
        AVCodecContext *codec_context = codecs_contexts_map[item.first];
        AVStream *stream = streams_map[item.first];
        size_t index = encoder_streams_ctxs->size();

        (*encoder_streams_ctxs)[item.first] = new encoder_stream_ctx{
            index,
            codec,
            codec_context,
            stream,
            av_packet_alloc(),
            av_frame_alloc(),
        };
    }

    *ctx_ref = new encoder_ctx{false, path, format_ctx, encoder_streams_ctxs};
    return 0;
}

int encoder_get_samples_count_per_frame(void *ctx_ref, int stream_tag) {
    auto casted_ctx = static_cast<encoder_ctx *>(ctx_ref);

    if (!casted_ctx->streams_map->contains(stream_tag)) {
        return ENCODER_STREAM_NOT_FOUND;
    }

    return (*casted_ctx->streams_map)[stream_tag]->codec_context->frame_size;
}

int encoder_get_bytes_per_sample_count(void *ctx_ref, int stream_tag) {
    auto casted_ctx = static_cast<encoder_ctx *>(ctx_ref);

    if (!casted_ctx->streams_map->contains(stream_tag)) {
        return ENCODER_STREAM_NOT_FOUND;
    }

    return av_get_bytes_per_sample((*casted_ctx->streams_map)[stream_tag]->codec_context->sample_fmt);
}

int encode_frame(AVCodecContext *context,
                 AVFormatContext *format_ctx,
                 AVFrame *frame,
                 size_t stream_index,
                 AVPacket *packet) {
    if (avcodec_send_frame(context, frame) < 0) {
        if (frame != nullptr) {
            av_frame_unref(frame);
        }

        return ENCODER_UNEXPECTED_ERROR;
    }

    while (true) {
        int res = avcodec_receive_packet(context, packet);

        if (res < 0) {
            av_packet_unref(packet);

            if (frame != nullptr) {
                av_frame_unref(frame);
            }

            return res == AVERROR_EOF || res == AVERROR(EAGAIN) ? 0 : ENCODER_UNEXPECTED_ERROR;
        }

        packet->stream_index = (int) stream_index;

        if (av_interleaved_write_frame(format_ctx, packet) < 0) {
            av_packet_unref(packet);

            if (frame != nullptr) {
                av_frame_unref(frame);
            }

            return ENCODER_UNEXPECTED_ERROR;
        }

        if (frame != nullptr) {
            av_frame_unref(frame);
        }
    }
}

int encoder_encode(void *ctx_ref, int stream_tag, uint8_t **data, size_t data_len) {
    auto casted_ctx = static_cast<encoder_ctx *>(ctx_ref);
    auto stream_ctx = (*casted_ctx->streams_map)[stream_tag];
    int bytes_per_sample = av_get_bytes_per_sample(stream_ctx->codec_context->sample_fmt);

    stream_ctx->frame->nb_samples = (int) data_len / bytes_per_sample;
    stream_ctx->frame->format = stream_ctx->codec_context->sample_fmt;
    stream_ctx->frame->channel_layout = stream_ctx->codec_context->channel_layout;
    stream_ctx->frame->sample_rate = stream_ctx->codec_context->sample_rate;

    if (av_frame_get_buffer(stream_ctx->frame, 0) < 0) {
        return ENCODER_UNEXPECTED_ERROR;
    }

    memcpy(stream_ctx->frame->data, data, stream_ctx->codec_context->channels * sizeof(uint8_t*));

    return encode_frame(stream_ctx->codec_context,
                        casted_ctx->format_ctx,
                        stream_ctx->frame,
                        stream_ctx->index,
                        stream_ctx->packet);
}

int encoder_finish_encode(void *ctx_ref) {
    auto casted_ctx = static_cast<encoder_ctx *>(ctx_ref);

    for (const auto &item : *casted_ctx->streams_map) {
        auto stream_ctx = (*casted_ctx->streams_map)[item.first];
        int result = encode_frame(stream_ctx->codec_context,
                                  casted_ctx->format_ctx,
                                  nullptr,
                                  stream_ctx->index,
                                  stream_ctx->packet);

        if (result < 0) {
            return result;
        }
    }

    if (av_write_trailer(casted_ctx->format_ctx) >= 0) {
        casted_ctx->encode_finished = true;
        return 0;
    }

    return ENCODER_UNEXPECTED_ERROR;
}

void encoder_free(void **ctx_ref) {
    auto casted_ctx = static_cast<encoder_ctx *>(*ctx_ref);

    for (const auto &item : *casted_ctx->streams_map) {
        av_frame_free(&(item.second->frame));
        av_packet_free(&(item.second->packet));
        avcodec_free_context(&item.second->codec_context);
        delete item.second;
    }

    delete casted_ctx->streams_map;
    avio_closep(&casted_ctx->format_ctx->pb);
    avformat_free_context(casted_ctx->format_ctx);

    if (!casted_ctx->encode_finished) {
        std::remove(casted_ctx->encoding_file_path);
    }

    delete casted_ctx;
    *ctx_ref = nullptr;
}