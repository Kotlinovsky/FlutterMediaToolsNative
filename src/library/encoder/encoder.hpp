#ifndef FLUTTER_MEDIA_TOOLS_NATIVE_SRC_LIBRARY_ENCODER_ENCODER_HPP_
#define FLUTTER_MEDIA_TOOLS_NATIVE_SRC_LIBRARY_ENCODER_ENCODER_HPP_

extern "C" {
#include <libavcodec/avcodec.h>
}

#include "encoder_stream_config.hpp"
#include <map>

// Инициализирует энкодер
int encoder_init(void** ctx_ref, const char* path, std::map<int, encoder_stream_cfg>& streams);

// Выдает количество семплов во фрейме
int encoder_get_samples_count_per_frame(void *ctx_ref, int stream_tag);

// Выдает количество байтов на семпл
int encoder_get_bytes_per_sample_count(void *ctx_ref, int stream_tag);

// Кодирует указанные данные потока
int encoder_encode(void *ctx_ref, int stream_tag, uint8_t** data, size_t data_len);

// Заканчивает кодирование потока
int encoder_finish_encode(void *ctx_ref);

// Освобождает ресурсы занятые энкодером
void encoder_free(void** ctx_ref);

#endif //FLUTTER_MEDIA_TOOLS_NATIVE_SRC_LIBRARY_ENCODER_ENCODER_HPP_