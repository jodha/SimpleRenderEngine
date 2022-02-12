#include <gtest/gtest.h>

TEST(First, Try)
{
   // make it intentionally fail
   int a = 2;
   int b = 2*a;
   EXPECT_EQ(3*a, b);
}

