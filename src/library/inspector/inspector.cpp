#include "inspector.hpp"
#include "inspector_errors.hpp"
#include "../decoder/decoder.hpp"
#include "../decoder/decoder_errors.hpp"

int64_t inspector_get_audio_duration_in_us(const char *path) {
    void *decoder_ctx;
    std::unordered_set<AVMediaType> media_types{AVMEDIA_TYPE_AUDIO};
    int decoder_result = decoder_init(&decoder_ctx, path, 0, 0, media_types);

    if (decoder_result < 0) {
        switch (decoder_result) {
            case DECODER_UNSUPPORTED_MEDIA_TYPE_ERROR:
            case DECODER_NOT_ALL_CODECS_FOUND_ERROR:return INSPECTOR_UNSUPPORTED_INPUT_FORMAT;
            case DECODER_INPUT_OPENING_ERROR:return INSPECTOR_MAYBE_FILE_NOT_FOUND;
            default:return INSPECTOR_UNEXPECTED_ERROR;
        }
    }

    int64_t duration = decoder_get_duration_in_us(decoder_ctx);

    if (duration != AV_NOPTS_VALUE) {
        decoder_free(&decoder_ctx);
        return duration;
    }

    // Бывает такое, что для определения длительности необходимо декодировать звук
    int last_result = 0;
    int64_t last_pts = 0;

    while (last_result >= 0) {
        last_result = decoder_decode(decoder_ctx, [&last_pts](auto, auto, int64_t pts) {
          last_pts = pts;
          return true;
        });
    }

    decoder_free(&decoder_ctx);

    return last_result == DECODER_END_OF_STREAM_ERROR ?
           last_pts : INSPECTOR_UNEXPECTED_ERROR;
}