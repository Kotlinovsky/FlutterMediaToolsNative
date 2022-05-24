#ifndef FLUTTER_MEDIA_TOOLS_NATIVE_SRC_LIBRARY_ENCODER_ENCODER_STREAM_CONFIG_HPP_
#define FLUTTER_MEDIA_TOOLS_NATIVE_SRC_LIBRARY_ENCODER_ENCODER_STREAM_CONFIG_HPP_

extern "C" {
#include <libavcodec/avcodec.h>
}

struct encoder_stream_audio_codec_cfg {
  int sample_rate;
  int channels_count;
  uint64_t channel_layout;
  AVSampleFormat sample_format;
};

struct encoder_stream_cfg {
  AVCodecID codec_id = AV_CODEC_ID_FIRST_UNKNOWN;
  void* cfg = nullptr;
};

#endif //FLUTTER_MEDIA_TOOLS_NATIVE_SRC_LIBRARY_ENCODER_ENCODER_STREAM_CONFIG_HPP_