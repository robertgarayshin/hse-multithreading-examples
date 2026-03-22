#include <gtest/gtest.h>
#include <numeric>
#include <string>
#include "apply_function.h"

TEST(ApplyFunctionTest, SingleThreadIncrement) {
    std::vector data = {1, 2, 3, 4, 5};
    ApplyFunction<int>(data, [](int& x) { x += 1; }, 1);
    EXPECT_EQ(data, (std::vector{2, 3, 4, 5, 6}));
}

TEST(ApplyFunctionTest, MultiThreadIncrement) {
    std::vector<int> data(100);
    std::iota(data.begin(), data.end(), 0);

    std::vector<int> expected(100);
    std::iota(expected.begin(), expected.end(), 1);

    ApplyFunction<int>(data, [](int& x) { x += 1; }, 4);
    EXPECT_EQ(data, expected);
}

TEST(ApplyFunctionTest, ThreadCountExceedsElements) {
    std::vector data = {10, 20, 30};
    ApplyFunction<int>(data, [](int& x) { x *= 2; }, 1000);
    EXPECT_EQ(data, (std::vector{20, 40, 60}));
}

TEST(ApplyFunctionTest, EmptyVector) {
    std::vector<int> data;
    EXPECT_NO_THROW(ApplyFunction<int>(data, [](int& x) { x = 0; }, 4));
    EXPECT_TRUE(data.empty());
}

TEST(ApplyFunctionTest, SingleElement) {
    std::vector data = {42};
    ApplyFunction<int>(data, [](int& x) { x = -x; }, 4);
    EXPECT_EQ(data[0], -42);
}

TEST(ApplyFunctionTest, DefaultThreadCount) {
    std::vector data = {1.0, 4.0, 9.0, 16.0};
    ApplyFunction<double>(data, [](double& x) { x = std::sqrt(x); });
    EXPECT_DOUBLE_EQ(data[0], 1.0);
    EXPECT_DOUBLE_EQ(data[1], 2.0);
    EXPECT_DOUBLE_EQ(data[2], 3.0);
    EXPECT_DOUBLE_EQ(data[3], 4.0);
}

TEST(ApplyFunctionTest, OneThreadPerElement) {
    std::vector data(8, 0);
    ApplyFunction<int>(data, [](int& x) { x = 1; }, 8);
    for (int v : data) EXPECT_EQ(v, 1);
}

TEST(ApplyFunctionTest, SumPreservedAfterNegate) {
    std::vector<int> data(1000);
    std::iota(data.begin(), data.end(), 1);
    const int expected = -std::accumulate(data.begin(), data.end(), 0);

    ApplyFunction<int>(data, [](int& x) { x = -x; }, 8);

    const int actual = std::accumulate(data.begin(), data.end(), 0);
    EXPECT_EQ(actual, expected);
}

TEST(ApplyFunctionTest, StringAppend) {
    std::vector<std::string> data = {"foo", "bar", "baz"};
    ApplyFunction<std::string>(data, [](std::string& s) { s += "_ok"; }, 3);
    EXPECT_EQ(data, (std::vector<std::string>{"foo_ok", "bar_ok", "baz_ok"}));
}

TEST(ApplyFunctionTest, StringToUpper) {
    std::vector<std::string> data = {"hello", "world"};
    ApplyFunction<std::string>(data, [](std::string& s) {
        for (char& c : s) c = static_cast<char>(std::toupper(c));
    });
    EXPECT_EQ(data, (std::vector<std::string>{"HELLO", "WORLD"}));
}

TEST(ApplyFunctionTest, FloatMultiply) {
    std::vector data = {1.0f, 2.0f, 3.0f, 4.0f};
    ApplyFunction<float>(data, [](float& x) { x *= 0.5f; }, 2);
    EXPECT_FLOAT_EQ(data[0], 0.5f);
    EXPECT_FLOAT_EQ(data[1], 1.0f);
    EXPECT_FLOAT_EQ(data[2], 1.5f);
    EXPECT_FLOAT_EQ(data[3], 2.0f);
}

TEST(ApplyFunctionTest, CustomStruct) {
    struct Point { int x, y; };
    std::vector<Point> data = {{1, 2}, {3, 4}, {5, 6}};
    ApplyFunction<Point>(data, [](Point& p) { p.x *= 2; p.y *= 2; }, 3);
    EXPECT_EQ(data[0].x, 2);  EXPECT_EQ(data[0].y, 4);
    EXPECT_EQ(data[1].x, 6);  EXPECT_EQ(data[1].y, 8);
    EXPECT_EQ(data[2].x, 10); EXPECT_EQ(data[2].y, 12);
}
