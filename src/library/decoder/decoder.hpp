#ifndef FLUTTER_MEDIA_TOOLS_NATIVE_SRC_LIBRARY_DECODER_DECODER_HPP_
#define FLUTTER_MEDIA_TOOLS_NATIVE_SRC_LIBRARY_DECODER_DECODER_HPP_

extern "C" {
#include <libavutil/mem.h>
#include <libavutil/samplefmt.h>
}

#include <unordered_set>
#include <functional>
#include <cstdint>
#include <vector>
#include <deque>

// Инициализирует декодер
int decoder_init(void **ctx_ref,
                 const char *path,
                 int64_t start_moment,
                 int64_t end_moment,
                 std::unordered_set<AVMediaType> &streams_types);

// Выполняет декодирование
int decoder_decode(void *ctx_ref,
                   const std::function<bool(const uint8_t **, size_t)> &handle_frame);

// Выдает частоту дискретизации потока с указанным индексом
int decoder_get_sample_rate(void *ctx_ref, size_t stream_index);

// Выдает количество каналов у потока с указанным индексом
int decoder_get_channels_count(void *ctx_ref, size_t stream_index);

// Выдает формат канала потока с указанным индексом
uint64_t decoder_get_channel_layout(void *ctx_ref, size_t stream_index);

// Выдает формат записи семплов в файле
AVSampleFormat decoder_get_sample_format(void *ctx_ref, size_t stream_index);

// Выдает количество потоков
uint32_t decoder_get_streams_count(void *ctx_ref);

// Выдает длительность медиа-контента в микросекундах
int64_t decoder_get_duration_in_us(void *ctx_ref);

// Освобождает ресурсы
void decoder_free(void **ctx_ref);

#endif //FLUTTER_MEDIA_TOOLS_NATIVE_SRC_LIBRARY_DECODER_DECODER_HPP_