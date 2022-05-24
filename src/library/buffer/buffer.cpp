#include "buffer.hpp"
#include "internal/buffer_context.hpp"
#include <cstring>

void buffer_allocate(void **ctx_ref, std::size_t rows_count) {
    auto **data = new uint8_t*[rows_count];

    for (std::size_t i = 0; i < rows_count; ++i) {
        data[i] = new uint8_t[0];
    }

    *ctx_ref = new buffer_ctx{data, 0, rows_count};
}

uint8_t **buffer_allocate_new_columns(void *ctx_ref, std::size_t new_columns) {
    auto casted_ctx = reinterpret_cast<buffer_ctx *>(ctx_ref);
    auto last_pointers = new uint8_t*[casted_ctx->rows_count];
    std::size_t new_colums_count = new_columns + casted_ctx->colums_count;

    for (std::size_t i = 0; i < casted_ctx->rows_count; ++i) {
        auto new_buffer = new uint8_t[new_colums_count];
        memcpy(new_buffer, casted_ctx->data[i], casted_ctx->colums_count);
        delete[] casted_ctx->data[i];
        casted_ctx->data[i] = new_buffer;
        last_pointers[i] = casted_ctx->data[i] + casted_ctx->colums_count;
    }

    casted_ctx->colums_count = new_colums_count;
    return last_pointers;
}

uint8_t **buffer_get_pointer(void *ctx_ref) {
    return reinterpret_cast<buffer_ctx *>(ctx_ref)->data;
}

void buffer_delete_from_start(void *ctx_ref, std::size_t count) {
    auto casted_ctx = reinterpret_cast<buffer_ctx *>(ctx_ref);

    for (std::size_t i = 0; i < casted_ctx->rows_count; ++i) {
        auto new_buffer = new uint8_t[casted_ctx->colums_count - count];
        memcpy(new_buffer, &casted_ctx->data[i][count], casted_ctx->colums_count - count);
        delete[] casted_ctx->data[i];
        casted_ctx->data[i] = new_buffer;
    }

    casted_ctx->colums_count -= count;
}

std::size_t buffer_get_colums_count(void *ctx_ref) {
    return reinterpret_cast<buffer_ctx *>(ctx_ref)->colums_count;
}

void buffer_free(void **ctx_ref) {
    auto casted_ctx = reinterpret_cast<buffer_ctx *>(*ctx_ref);

    for (std::size_t i = 0; i < casted_ctx->rows_count; ++i) {
        delete[] casted_ctx->data[i];
    }

    delete[] casted_ctx->data;
    delete casted_ctx;
    *ctx_ref = nullptr;
}
