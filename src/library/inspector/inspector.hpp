#ifndef FLUTTER_MEDIA_TOOLS_NATIVE_SRC_LIBRARY_INSPECTOR_INSPECTOR_HPP_
#define FLUTTER_MEDIA_TOOLS_NATIVE_SRC_LIBRARY_INSPECTOR_INSPECTOR_HPP_

#include <cstdint>

// Выдает длительность аудио-записи в микросекундах
extern "C"
int64_t inspector_get_audio_duration_in_us(const char *path);

#endif //FLUTTER_MEDIA_TOOLS_NATIVE_SRC_LIBRARY_INSPECTOR_INSPECTOR_HPP_