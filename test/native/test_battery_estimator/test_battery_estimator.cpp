#include <unity.h>
#include "domain/services/BatteryEstimator.h"

void setUp() {}
void tearDown() {}

void test_full_voltage_returns_100_percent() {
    BatteryEstimator est;
    TEST_ASSERT_EQUAL_UINT8(100, est.estimate(13.6f));
}

void test_mid_voltage_returns_approximately_50_percent() {
    BatteryEstimator est;
    uint8_t pct = est.estimate(12.8f);
    TEST_ASSERT_GREATER_OR_EQUAL(45, pct);
    TEST_ASSERT_LESS_OR_EQUAL(55, pct);
}

void test_low_voltage_returns_low_percent() {
    BatteryEstimator est;
    TEST_ASSERT_LESS_OR_EQUAL(15, est.estimate(12.0f));
}

void test_below_minimum_returns_0() {
    BatteryEstimator est;
    TEST_ASSERT_EQUAL_UINT8(0, est.estimate(9.0f));
}

void test_above_maximum_returns_100() {
    BatteryEstimator est;
    TEST_ASSERT_EQUAL_UINT8(100, est.estimate(15.0f));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_full_voltage_returns_100_percent);
    RUN_TEST(test_mid_voltage_returns_approximately_50_percent);
    RUN_TEST(test_low_voltage_returns_low_percent);
    RUN_TEST(test_below_minimum_returns_0);
    RUN_TEST(test_above_maximum_returns_100);
    return UNITY_END();
}
