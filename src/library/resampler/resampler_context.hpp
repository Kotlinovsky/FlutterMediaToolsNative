#ifndef FLUTTER_MEDIA_TOOLS_NATIVE_SRC_LIBRARY_RESAMPLER_RESAMPLER_CONTEXT_HPP_
#define FLUTTER_MEDIA_TOOLS_NATIVE_SRC_LIBRARY_RESAMPLER_RESAMPLER_CONTEXT_HPP_

extern "C" {
#include <libswresample/swresample.h>
}

struct resampler_ctx {
  SwrContext *context = nullptr;
  AVSampleFormat in_sample_fmt = AV_SAMPLE_FMT_NONE;
  AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_NONE;
  int in_sample_rate = 0;
  int out_sample_rate = 0;
  int in_channels_count = 0;
};

#endif //FLUTTER_MEDIA_TOOLS_NATIVE_SRC_LIBRARY_RESAMPLER_RESAMPLER_CONTEXT_HPP_
