#include <map>
#include <deque>
#include <vector>
#include <iostream>
#include <functional>

extern "C" {
#include <libavutil/channel_layout.h>
}

#include "decoder.hpp"
#include "decoder_context.hpp"
#include "decoder_errors.hpp"

// Проверяет наличие всех типов стримов
bool is_all_streams_found(std::unordered_set<AVMediaType> &streams_types,
                          AVStream **streams,
                          unsigned int streams_count) {
    std::unordered_map<AVMediaType, bool> types_exists_maps;

    for (const auto &item : streams_types) {
        types_exists_maps[item] = false;
    }

    for (unsigned int i = 0; i < streams_count; ++i) {
        types_exists_maps[streams[i]->codecpar->codec_type] = true;
    }

    for (const auto &item : types_exists_maps) {
        if (!item.second) {
            return false;
        }
    }

    return std::all_of(types_exists_maps.cbegin(),
                       types_exists_maps.cend(),
                       [](std::pair<const AVMediaType, bool> value) {
                         return value.second;
                       });
}

// Удаляет контексты стримов
void free_stream_contexts(std::vector<decoder_stream_ctx *> *contexts) {
    for (const auto &item : (*contexts)) {
        avcodec_free_context(&item->context);
        delete item;
    }

    delete contexts;
}

// Инициализирует декодеры
bool init_decoders(AVStream **streams,
                   unsigned int streams_count,
                   std::unordered_set<AVMediaType> &streams_types,
                   std::vector<decoder_stream_ctx *> **contexts_ref) {
    auto contexts = new std::vector<decoder_stream_ctx *>(streams_count, nullptr);

    for (unsigned int i = 0; i < streams_count; ++i) {
        AVStream *stream = streams[i];

        if (!streams_types.contains(stream->codecpar->codec_type)) {
            continue;
        }

        const AVCodec *decoder = avcodec_find_decoder(stream->codecpar->codec_id);
        AVCodecContext *decoder_context = avcodec_alloc_context3(decoder);

        if (decoder_context == nullptr) {
            free_stream_contexts(contexts);
            return false;
        }

        decoder_context->pkt_timebase = stream->time_base;

        if (avcodec_parameters_to_context(decoder_context, stream->codecpar) < 0
            || avcodec_open2(decoder_context, decoder, nullptr) < 0) {
            free_stream_contexts(contexts);
            return false;
        }

        (*contexts)[i] = new decoder_stream_ctx{decoder, decoder_context, 0, 0};
    }

    *contexts_ref = contexts;
    return true;
}

int decoder_init(void **ctx_ref,
                 const char *path,
                 int64_t start_moment,
                 int64_t end_moment,
                 std::unordered_set<AVMediaType> &streams_types) {
    if (streams_types.empty() || streams_types.contains(AVMediaType::AVMEDIA_TYPE_UNKNOWN)) {
        return DECODER_UNSUPPORTED_MEDIA_TYPE_ERROR;
    }

    AVFormatContext *format_ctx = avformat_alloc_context();

    if (avformat_open_input(&format_ctx, path, nullptr, nullptr) < 0) {
        avformat_free_context(format_ctx);
        return DECODER_INPUT_OPENING_ERROR;
    }

    if (avformat_find_stream_info(format_ctx, nullptr) < 0) {
        avformat_close_input(&format_ctx);
        return DECODER_STREAM_INFO_SEARCHING_ERROR;
    }

    if (!is_all_streams_found(streams_types, format_ctx->streams, format_ctx->nb_streams)) {
        avformat_close_input(&format_ctx);
        return DECODER_NOT_ALL_CODECS_FOUND_ERROR;
    }

    std::vector<decoder_stream_ctx *> *stream_contexts;

    if (!init_decoders(format_ctx->streams,
                       format_ctx->nb_streams,
                       streams_types,
                       &stream_contexts)) {
        avformat_close_input(&format_ctx);
        return DECODER_NOT_ALL_CODECS_OPENED_ERROR;
    }

    if (start_moment > 0) {
        if (av_seek_frame(format_ctx, -1, start_moment * 1000, 0) < 0) {
            free_stream_contexts(stream_contexts);
            avformat_close_input(&format_ctx);
            return DECODER_UNEXPECTED_ERROR;
        }
    }

    *ctx_ref = new decoder_ctx{
        av_frame_alloc(),
        av_packet_alloc(),
        format_ctx,
        stream_contexts,
        new std::unordered_set<int>(),
        end_moment != 0 ? (end_moment - start_moment) * 1000 : -1
    };

    return 0;
}

int decoder_decode(void *ctx_ref,
                   const std::function<bool(const uint8_t **, size_t, int64_t)> &handle_frame) {
    auto *casted_ctx = static_cast<decoder_ctx *>(ctx_ref);
    int read_frame_result = av_read_frame(casted_ctx->format_ctx, casted_ctx->packet);

    if (read_frame_result == AVERROR_EOF) {
        return DECODER_END_OF_STREAM_ERROR;
    } else if (read_frame_result < 0) {
        return DECODER_UNEXPECTED_ERROR;
    }

    if (casted_ctx->decoded_channels->size() >= casted_ctx->stream_contexts->size()) {
        av_packet_unref(casted_ctx->packet);
        return DECODER_END_OF_STREAM_ERROR;
    } else if (casted_ctx->decoded_channels->contains(casted_ctx->packet->stream_index)) {
        av_packet_unref(casted_ctx->packet);
        return 0;
    }

    decoder_stream_ctx
        *stream_ctx = (*casted_ctx->stream_contexts)[casted_ctx->packet->stream_index];

    if (stream_ctx == nullptr) {
        av_packet_unref(casted_ctx->packet);
        return 0;
    } else if (avcodec_send_packet(stream_ctx->context, casted_ctx->packet) < 0) {
        av_packet_unref(casted_ctx->packet);
        return DECODER_UNEXPECTED_ERROR;
    }

    av_packet_unref(casted_ctx->packet);

    while (true) {
        int res = avcodec_receive_frame(stream_ctx->context, casted_ctx->frame);

        if (res < 0) {
            av_frame_unref(casted_ctx->frame);
            return res == AVERROR_EOF || res == AVERROR(EAGAIN) ? 0 : DECODER_UNEXPECTED_ERROR;
        }

        auto pts = av_rescale_q(casted_ctx->frame->pts,
                                stream_ctx->context->pkt_timebase,
                                AV_TIME_BASE_Q);

        if (casted_ctx->duration != -1) {
            if (!(casted_ctx->frame->flags & AV_FRAME_FLAG_DISCARD)) {
                if (stream_ctx->prev_pts != 0) {
                    stream_ctx->current_time += pts - stream_ctx->prev_pts;
                }

                stream_ctx->prev_pts = pts;
            }

            if (stream_ctx->current_time > casted_ctx->duration) {
                casted_ctx->decoded_channels->insert(casted_ctx->packet->stream_index);
                av_frame_unref(casted_ctx->frame);
                return 0;
            }
        }

        int bytes_per_sample = av_get_bytes_per_sample(stream_ctx->context->sample_fmt);

        if (stream_ctx->codec->type == AVMEDIA_TYPE_AUDIO) {
            auto const_data = const_cast<const uint8_t **>(casted_ctx->frame->data);
            std::size_t bytes_count = casted_ctx->frame->nb_samples * bytes_per_sample;

            if (!av_sample_fmt_is_planar(stream_ctx->context->sample_fmt)) {
                bytes_count *= casted_ctx->frame->channels;
            }

            bool result = handle_frame(const_data, bytes_count, pts);
            av_frame_unref(casted_ctx->frame);

            if (!result) {
                return DECODER_UNEXPECTED_ERROR;
            }
        }
    }
}

int decoder_get_sample_rate(void *ctx_ref, size_t stream_index) {
    return (*static_cast<decoder_ctx *>(ctx_ref)->stream_contexts)[stream_index]->context->sample_rate;
}

int decoder_get_channels_count(void *ctx_ref, size_t stream_index) {
    return (*static_cast<decoder_ctx *>(ctx_ref)->stream_contexts)[stream_index]->context->channels;
}

uint64_t decoder_get_channel_layout(void *ctx_ref, size_t stream_index) {
    auto casted_ctx = static_cast<decoder_ctx *>(ctx_ref);
    auto layout = (*casted_ctx->stream_contexts)[stream_index]->context->channel_layout;

    if (layout == 0) {
        return av_get_default_channel_layout(decoder_get_channels_count(ctx_ref, stream_index));
    }

    return layout;
}

AVSampleFormat decoder_get_sample_format(void *ctx_ref, size_t stream_index) {
    return (*static_cast<decoder_ctx *>(ctx_ref)->stream_contexts)[stream_index]->context->sample_fmt;
}

int64_t decoder_get_duration_in_us(void *ctx_ref) {
    return static_cast<decoder_ctx *>(ctx_ref)->format_ctx->duration;
}

uint32_t decoder_get_streams_count(void *ctx_ref) {
    return (*static_cast<decoder_ctx *>(ctx_ref)).format_ctx->nb_streams;
}

void decoder_free(void **ctx_ref) {
    auto casted_ctx_ref = reinterpret_cast<decoder_ctx **>(ctx_ref);
    auto casted_ctx = *casted_ctx_ref;

    free_stream_contexts(casted_ctx->stream_contexts);
    avformat_close_input(&casted_ctx->format_ctx);
    av_packet_free(&casted_ctx->packet);
    av_frame_free(&casted_ctx->frame);
    delete casted_ctx->decoded_channels;

    delete casted_ctx;
    *casted_ctx_ref = nullptr;
}
