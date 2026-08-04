#include <gtest/gtest.h>
#include <rpmalloc/rpmalloc.h>

int main(int argc, char** argv)
{
    rpmalloc_initialize();
    rpmalloc_thread_initialize();

    testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();

    rpmalloc_thread_finalize(1);
    rpmalloc_finalize();

    return result;
}