#ifndef FLUTTER_MEDIA_TOOLS_NATIVE_SRC_LIBRARY_BUFFER_BUFFER_CONTEXT_HPP_
#define FLUTTER_MEDIA_TOOLS_NATIVE_SRC_LIBRARY_BUFFER_BUFFER_CONTEXT_HPP_

#include <cstdint>

struct buffer_ctx {
  uint8_t** data = nullptr;
  std::size_t colums_count = 0;
  std::size_t rows_count = 0;
};

#endif //FLUTTER_MEDIA_TOOLS_NATIVE_SRC_LIBRARY_BUFFER_BUFFER_CONTEXT_HPP_
