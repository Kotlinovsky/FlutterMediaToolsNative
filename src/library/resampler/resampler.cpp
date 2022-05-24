extern "C" {
#include <libswresample/swresample.h>
}

#include <iostream>
#include "resampler.hpp"
#include "resampler_context.hpp"
#include "resampler_errors.hpp"

// Считает количество семплов по длине массива байтов
int get_input_samples_count(resampler_ctx *ctx, int data_len) {
    int in_samples_count = (int) data_len / av_get_bytes_per_sample(ctx->in_sample_fmt);

    if (!av_sample_fmt_is_planar(ctx->in_sample_fmt)) {
        in_samples_count /= ctx->in_channels_count;
    }

    return in_samples_count;
}

// Считает количество семплов после ресемплинга
int64_t calculate_output_samples_count(int64_t max_in_samples_count,
                                       int in_sample_rate,
                                       int out_sample_rate) {
    return av_rescale_rnd(max_in_samples_count, out_sample_rate, in_sample_rate, AV_ROUND_UP);
}

// Считает количество семплов после ресемплинга
int get_output_samples_count(resampler_ctx *ctx, int data_len) {
    int in_samples_count = get_input_samples_count(ctx, data_len);

    return (int) calculate_output_samples_count(in_samples_count,
                                                ctx->in_sample_rate,
                                                ctx->out_sample_rate);
}

int resampler_init(void **ctx_ref,
                   int64_t in_channel_layout,
                   int64_t out_channel_layout,
                   AVSampleFormat in_sample_fmt,
                   AVSampleFormat out_sample_fmt,
                   int in_sample_rate,
                   int out_sample_rate,
                   int in_channels_count) {
    SwrContext *context = swr_alloc_set_opts(nullptr,
                                             out_channel_layout,
                                             out_sample_fmt,
                                             out_sample_rate,
                                             in_channel_layout,
                                             in_sample_fmt,
                                             in_sample_rate,
                                             0,
                                             nullptr);

    if (swr_init(context) < 0) {
        swr_free(&context);
        return RESAMPLER_UNEXPECTED_ERROR;
    }

    *ctx_ref = new resampler_ctx{
        context,
        in_sample_fmt,
        out_sample_fmt,
        in_sample_rate,
        out_sample_rate,
        in_channels_count
    };

    return 0;
}

int resampler_get_need_bytes_count(void *ctx_ref, int data_len) {
    auto casted_ctx = static_cast<resampler_ctx *>(ctx_ref);

    return get_output_samples_count(casted_ctx, data_len)
        * av_get_bytes_per_sample(casted_ctx->out_sample_fmt);
}

int resampler_resample(void *ctx_ref, const uint8_t **data, int data_len, uint8_t **output) {
    auto casted_ctx = static_cast<resampler_ctx *>(ctx_ref);
    int in_samples = get_input_samples_count(casted_ctx, data_len);
    int out_samples = (int) calculate_output_samples_count(in_samples,
                                                           casted_ctx->in_sample_rate,
                                                           casted_ctx->out_sample_rate);
    int result = swr_convert(casted_ctx->context, output, out_samples, data, in_samples);

    if (result < 0) {
        return RESAMPLER_UNEXPECTED_ERROR;
    }

    return result * av_get_bytes_per_sample(casted_ctx->out_sample_fmt);
}

void resampler_free(void **ctx_ref) {
    auto casted_ctx = static_cast<resampler_ctx *>(*ctx_ref);
    swr_close(casted_ctx->context);
    swr_free(&casted_ctx->context);
    delete casted_ctx;
    *ctx_ref = nullptr;
}