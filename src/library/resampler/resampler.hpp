#ifndef FLUTTER_MEDIA_TOOLS_NATIVE_SRC_LIBRARY_RESAMPLER_RESAMPLER_HPP_
#define FLUTTER_MEDIA_TOOLS_NATIVE_SRC_LIBRARY_RESAMPLER_RESAMPLER_HPP_

extern "C" {
#include <libavutil/samplefmt.h>
}

#include <cstdint>

// Инициализирует контекст ресемплера
int resampler_init(void **ctx_ref,
                   int64_t in_channel_layout,
                   int64_t out_channel_layout,
                   AVSampleFormat in_sample_fmt,
                   AVSampleFormat out_sample_fmt,
                   int in_sample_rate,
                   int out_sample_rate,
                   int in_channels_count);

// Возвращает необходимое количество байтов для ресемплинга указанных байтов
int resampler_get_need_bytes_count(void *ctx_ref, int data_len);

// Производит ресемплинг указанного фрагмента аудио
int resampler_resample(void *ctx_ref, const uint8_t **data, int data_len, uint8_t** output);

// Освобождает ресурсы, занятые ресемплером
void resampler_free(void **ctx_ref);

#endif //FLUTTER_MEDIA_TOOLS_NATIVE_SRC_LIBRARY_RESAMPLER_RESAMPLER_HPP_