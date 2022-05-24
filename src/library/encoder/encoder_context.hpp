#ifndef FLUTTER_MEDIA_TOOLS_NATIVE_SRC_LIBRARY_ENCODER_ENCODER_CONTEXT_HPP_
#define FLUTTER_MEDIA_TOOLS_NATIVE_SRC_LIBRARY_ENCODER_ENCODER_CONTEXT_HPP_

extern "C" {
#include <libavformat/avformat.h>
}

#include <unordered_map>

struct encoder_stream_ctx {
  size_t index = 0;
  const AVCodec *codec = nullptr;
  AVCodecContext *codec_context = nullptr;
  AVStream *stream = nullptr;
  AVPacket *packet = nullptr;
  AVFrame *frame = nullptr;
};

struct encoder_ctx {
  bool encode_finished = false;
  const char *encoding_file_path = nullptr;
  AVFormatContext *format_ctx = nullptr;
  std::unordered_map<int, encoder_stream_ctx *>* streams_map = nullptr;
};

#endif //FLUTTER_MEDIA_TOOLS_NATIVE_SRC_LIBRARY_ENCODER_ENCODER_CONTEXT_HPP_