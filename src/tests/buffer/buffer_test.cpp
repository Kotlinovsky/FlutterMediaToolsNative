#include "../../library/buffer/buffer.hpp"
#include "gtest/gtest.h"

TEST(BufferTest, Allocating) {
    void *ctx_ref = nullptr;
    buffer_allocate(&ctx_ref, 1);
    EXPECT_EQ(buffer_get_colums_count(ctx_ref), 0);
    EXPECT_NE(ctx_ref, nullptr);
    buffer_free(&ctx_ref);
    EXPECT_EQ(ctx_ref, nullptr);
}

TEST(BufferTest, NewColumnsAllocating) {
    void *ctx_ref = nullptr;
    buffer_allocate(&ctx_ref, 2);
    delete[] buffer_allocate_new_columns(ctx_ref, 1);
    EXPECT_EQ(buffer_get_colums_count(ctx_ref), 1);
    buffer_get_pointer(ctx_ref)[0][0] = 1;
    buffer_get_pointer(ctx_ref)[1][0] = 2;
    EXPECT_EQ(buffer_get_pointer(ctx_ref)[0][0], 1);
    EXPECT_EQ(buffer_get_pointer(ctx_ref)[1][0], 2);

    auto pointers = buffer_allocate_new_columns(ctx_ref, 2);
    pointers[0][0] = 5;
    pointers[1][0] = 6;
    EXPECT_EQ(buffer_get_pointer(ctx_ref)[0][0], 1);
    EXPECT_EQ(buffer_get_pointer(ctx_ref)[1][0], 2);
    EXPECT_EQ(buffer_get_pointer(ctx_ref)[0][1], 5);
    EXPECT_EQ(buffer_get_pointer(ctx_ref)[1][1], 6);
    delete[] pointers;

    buffer_get_pointer(ctx_ref)[0][1] = 3;
    buffer_get_pointer(ctx_ref)[1][1] = 4;
    EXPECT_EQ(buffer_get_pointer(ctx_ref)[0][0], 1);
    EXPECT_EQ(buffer_get_pointer(ctx_ref)[1][0], 2);
    EXPECT_EQ(buffer_get_pointer(ctx_ref)[0][1], 3);
    EXPECT_EQ(buffer_get_pointer(ctx_ref)[1][1], 4);

    buffer_free(&ctx_ref);
}

TEST(BufferTest, DeletingFromStart) {
    void *ctx_ref = nullptr;
    buffer_allocate(&ctx_ref, 2);
    delete[] buffer_allocate_new_columns(ctx_ref, 128);
    buffer_delete_from_start(ctx_ref, 128);
    EXPECT_EQ(buffer_get_colums_count(ctx_ref), 0);
    buffer_free(&ctx_ref);
}