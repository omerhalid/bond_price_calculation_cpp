#include <gtest/gtest.h>

#include "../calculateBondPrice.cpp"

TEST(CalculateBondPriceTest, Test1) {
  // Test case 1
  double price = calculateBondPrice(1000, 0.05, 5);
  EXPECT_DOUBLE_EQ(price, 1000.0);
}

TEST(CalculateBondPriceTest, Test2) {
  // Test case 2
  double price = calculateBondPrice(2000, 0.03, 10);
  EXPECT_DOUBLE_EQ(price, 1863.43);
}

TEST(CalculateBondPriceTest, Test3) {
  // Test case 3
  double price = calculateBondPrice(500, 0.08, 2);
  EXPECT_DOUBLE_EQ(price, 456.62);
}

TEST(CalculateBondPriceTest, Test4) {
  // Test case 4
  double price = calculateBondPrice(1500, 0.06, 7);
  EXPECT_DOUBLE_EQ(price, 1354.32);
}

TEST(CalculateBondPriceTest, Test5) {
  // Test case 5
  double price = calculateBondPrice(3000, 0.04, 3);
  EXPECT_DOUBLE_EQ(price, 2824.87);
}