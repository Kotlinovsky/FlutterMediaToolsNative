#ifndef FLUTTER_MEDIA_TOOLS_NATIVE_SRC_LIBRARY_TRANSCODER_TRANSCODER_HPP_
#define FLUTTER_MEDIA_TOOLS_NATIVE_SRC_LIBRARY_TRANSCODER_TRANSCODER_HPP_

#include <cstdint>

// Запускает транскодирование аудио-записи
extern "C"
int transcoder_do_audio(const char *in_path,
                        const char *out_path,
                        int64_t start_moment_in_ms,
                        int64_t end_moment_in_ms);

#endif //FLUTTER_MEDIA_TOOLS_NATIVE_SRC_LIBRARY_TRANSCODER_TRANSCODER_HPP_