#ifndef FLUTTER_MEDIA_TOOLS_NATIVE_SRC_LIBRARY_DECODER_DECODER_CONTEXT_HPP_
#define FLUTTER_MEDIA_TOOLS_NATIVE_SRC_LIBRARY_DECODER_DECODER_CONTEXT_HPP_

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

#include <unordered_set>

struct decoder_stream_ctx {
  const AVCodec *codec = nullptr;
  AVCodecContext *context = nullptr;
  int64_t current_time = 0;
  int64_t prev_pts = 0;
};

struct decoder_ctx {
  AVFrame* frame = nullptr;
  AVPacket *packet = nullptr;
  AVFormatContext *format_ctx = nullptr;
  std::vector<decoder_stream_ctx *> *stream_contexts = nullptr;
  std::unordered_set<int> *decoded_channels = nullptr;
  int64_t duration = -1;
};

#endif //FLUTTER_MEDIA_TOOLS_NATIVE_SRC_LIBRARY_DECODER_DECODER_CONTEXT_HPP_
