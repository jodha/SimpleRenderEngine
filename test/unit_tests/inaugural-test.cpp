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

TEST(Third, Try)
{
    double estimate = 1.0003;
    EXPECT_NEAR(1.00029999, estimate, 1.0e-6);
}

TEST(Fourth, Try)
{
    int number = 2;
    EXPECT_EQ(3, number); 
}

