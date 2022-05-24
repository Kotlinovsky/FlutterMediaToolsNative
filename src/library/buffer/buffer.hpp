#ifndef FLUTTER_MEDIA_TOOLS_NATIVE_SRC_LIBRARY_BUFFER_BUFFER_HPP_
#define FLUTTER_MEDIA_TOOLS_NATIVE_SRC_LIBRARY_BUFFER_BUFFER_HPP_

#include <cstdint>

// Выделяет память для буффера
void buffer_allocate(void **ctx_ref, std::size_t rows_count);

// Выделяет место для новых столбцов, возвращает указатели на последние заполненные позиции в них.
// Внимание! Результат должен быть удален после использования!
uint8_t **buffer_allocate_new_columns(void *ctx_ref, std::size_t new_columns);

// Возвращает указатель на буффер
uint8_t **buffer_get_pointer(void *ctx_ref);

// Удаляет определенное количество байт с начала массива
void buffer_delete_from_start(void *ctx_ref, std::size_t count);

// Возвращает размер буффера
std::size_t buffer_get_colums_count(void *ctx_ref);

// Освобождает занятые ресурсы
void buffer_free(void **ctx_ref);

#endif //FLUTTER_MEDIA_TOOLS_NATIVE_SRC_LIBRARY_BUFFER_BUFFER_HPP_
