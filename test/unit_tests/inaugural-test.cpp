#include <gtest/gtest.h>

TEST(First, Try)
{
   int a = 2;
   int b = 2*a;
   EXPECT_EQ(2*a, b);
}

