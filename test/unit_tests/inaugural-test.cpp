#include <gtest/gtest.h>
#include <string>

TEST(First, Try)
{
   int a = 2;
   int b = 2*a;
   EXPECT_EQ(2*a, b);
}

TEST(Second, Try)
{
   const std::string message = "Hello World";
   EXPECT_EQ(std::string("Hello World"), message);
}

