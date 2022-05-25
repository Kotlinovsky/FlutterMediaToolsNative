#include "transcoder.hpp"
#include "transcoder_errors.hpp"

#include "../decoder/decoder.hpp"
#include "../decoder/decoder_errors.hpp"
#include "../encoder/encoder.hpp"
#include "../encoder/encoder_errors.hpp"
#include "../resampler/resampler.hpp"
#include "../buffer/buffer.hpp"

// Считает необходимое количество байт для кодирования полного фрейма
size_t get_need_bytes_count_at_encoder(void *enc_ctx) {
    return encoder_get_samples_count_per_frame(enc_ctx, 0) * encoder_get_bytes_per_sample_count(enc_ctx, 0);
}

// Кодирует аудио-фрейм
bool encode_audio(void *buf_ctx, void *enc_ctx, size_t data_len) {
    if (encoder_encode(enc_ctx, 0, buffer_get_pointer(buf_ctx), data_len) >= 0) {
        buffer_delete_from_start(buf_ctx, data_len);
        return true;
    }

    return false;
}

// Ресемплит и кодирует аудио-фрейм
bool resample_and_encode_audio(const uint8_t **data,
                               int data_len,
                               void *sampler_ctx,
                               void *enc_ctx,
                               void *buf_ctx) {
    int need_bytes_at_buffer = resampler_get_need_bytes_count(sampler_ctx, data_len);
    uint8_t **last_buffer_pointers = buffer_allocate_new_columns(buf_ctx, need_bytes_at_buffer);

    if (resampler_resample(sampler_ctx, data, data_len, last_buffer_pointers) < 0) {
        delete[] last_buffer_pointers;
        return false;
    }

    delete[] last_buffer_pointers;
    size_t need_bytes_at_encoder = get_need_bytes_count_at_encoder(enc_ctx);

    while (buffer_get_colums_count(buf_ctx) >= need_bytes_at_encoder) {
        if (!encode_audio(buf_ctx, enc_ctx, need_bytes_at_encoder)) {
            return false;
        }
    }

    return true;
}

// Запускает декодирование аудио и далее транскодирует его
bool transcode_audio(void *dec_ctx, void *sampler_ctx, void *enc_ctx, void *buf_ctx) {
    while (true) {
        int res = decoder_decode(dec_ctx, [&sampler_ctx, &enc_ctx, &buf_ctx](const uint8_t **data,
                                                                             int data_len, auto) {
          return resample_and_encode_audio(data, data_len, sampler_ctx, enc_ctx, buf_ctx);
        });

        if (res < 0) {
            if (res != DECODER_END_OF_STREAM_ERROR) {
                return false;
            } else {
                break;
            }
        }
    }

    // Транскодирование считается успешным, если в итоге в буффере ничего не осталось, либо оставшееся было закодировано.
    size_t need_bytes_at_encoder = get_need_bytes_count_at_encoder(enc_ctx);
    size_t remained_bytes;

    while ((remained_bytes = buffer_get_colums_count(buf_ctx)) > 0) {
        size_t block_size = std::min(need_bytes_at_encoder, remained_bytes);

        if (!encode_audio(buf_ctx, enc_ctx, block_size)) {
            return false;
        }
    }

    // Также необходимо, чтобы успешно выполнилось завершение процесса кодирования.
    return encoder_finish_encode(enc_ctx) >= 0;
}

extern "C"
int transcoder_do_audio(const char *in_path,
                        const char *out_path,
                        int64_t start_moment_in_ms,
                        int64_t end_moment_in_ms) {
    void *decoder_ctx;
    std::unordered_set<AVMediaType> decode_media_types{AVMEDIA_TYPE_AUDIO};
    int decoder_result = decoder_init(&decoder_ctx,
                                      in_path,
                                      start_moment_in_ms,
                                      end_moment_in_ms,
                                      decode_media_types);

    if (decoder_result < 0) {
        switch (decoder_result) {
            case DECODER_UNSUPPORTED_MEDIA_TYPE_ERROR:
            case DECODER_NOT_ALL_CODECS_FOUND_ERROR:return TRANSCODER_UNSUPPORTED_INPUT_FORMAT;
            case DECODER_INPUT_OPENING_ERROR:return TRANSCODER_MAYBE_FILE_NOT_FOUND;
            default:return TRANSCODER_UNEXPECTED_ERROR;
        }
    } else if (decoder_get_streams_count(decoder_ctx) != 1) {
        decoder_free(&decoder_ctx);
        return TRANSCODER_UNSUPPORTED_INPUT_FORMAT;
    }

    int in_sample_rate = decoder_get_sample_rate(decoder_ctx, 0);
    int in_channels_count = decoder_get_channels_count(decoder_ctx, 0);
    uint64_t in_channel_layout = decoder_get_channel_layout(decoder_ctx, 0);
    AVSampleFormat in_sample_format = decoder_get_sample_format(decoder_ctx, 0);
    auto *audio_cfg = new encoder_stream_audio_codec_cfg{
        in_sample_rate, in_channels_count, in_channel_layout, in_sample_format};

    // Инициализируем энкодер
    void *encoder_ctx;
    encoder_stream_cfg stream_cfg{AV_CODEC_ID_AAC, audio_cfg};
    std::map<int, encoder_stream_cfg> encoders_configs{{0, stream_cfg}};
    int encoder_result = encoder_init(&encoder_ctx, out_path, encoders_configs);

    if (encoder_result < 0) {
        decoder_free(&decoder_ctx);
        delete audio_cfg;

        return encoder_result == ENCODER_OUTPUT_STREAM_ERROR
               ? TRANSCODER_MAYBE_FILE_ALREADY_EXIST
               : TRANSCODER_UNEXPECTED_ERROR;
    }

    void *resampler_ctx;
    int resampler_result = resampler_init(&resampler_ctx,
                                          static_cast<int64_t>(in_channel_layout),
                                          static_cast<int64_t>(audio_cfg->channel_layout),
                                          in_sample_format,
                                          audio_cfg->sample_format,
                                          in_sample_rate,
                                          audio_cfg->sample_rate,
                                          in_channels_count);

    if (resampler_result < 0) {
        decoder_free(&decoder_ctx);
        encoder_free(&encoder_ctx);
        delete audio_cfg;
        return TRANSCODER_UNEXPECTED_ERROR;
    }

    void *encode_buffer;
    buffer_allocate(&encode_buffer, audio_cfg->channels_count);
    int result_code = !transcode_audio(decoder_ctx, resampler_ctx, encoder_ctx, encode_buffer)
                      ? TRANSCODER_UNEXPECTED_ERROR
                      : 0;

    decoder_free(&decoder_ctx);
    encoder_free(&encoder_ctx);
    resampler_free(&resampler_ctx);
    buffer_free(&encode_buffer);
    delete audio_cfg;

    return result_code;
}
