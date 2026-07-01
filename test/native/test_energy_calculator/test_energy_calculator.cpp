#include <unity.h>
#include "domain/services/EnergyCalculator.h"

void setUp() {}
void tearDown() {}

void test_initial_statistics_are_zero() {
    EnergyCalculator calc;
    auto stats = calc.statistics();
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, stats.whToday);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, stats.whTotal);
}

void test_accumulates_wh_correctly() {
    EnergyCalculator calc;
    // 100W for 1 hour = 100 Wh
    calc.update(100.0f, 0);
    calc.update(100.0f, 3600000);
    auto stats = calc.statistics();
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 100.0f, stats.whToday);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 100.0f, stats.whTotal);
}

void test_reset_today_zeroes_whtoday_keeps_whtotal() {
    EnergyCalculator calc;
    calc.update(100.0f, 0);
    calc.update(100.0f, 3600000);
    calc.resetToday();
    auto stats = calc.statistics();
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, stats.whToday);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 100.0f, stats.whTotal);
}

void test_multiple_updates_accumulate() {
    EnergyCalculator calc;
    calc.update(50.0f, 0);
    calc.update(50.0f, 1800000);  // 30 min -> 25 Wh
    calc.update(50.0f, 3600000);  // 30 min -> 25 Wh more = 50 Wh total
    auto stats = calc.statistics();
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 50.0f, stats.whToday);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_initial_statistics_are_zero);
    RUN_TEST(test_accumulates_wh_correctly);
    RUN_TEST(test_reset_today_zeroes_whtoday_keeps_whtotal);
    RUN_TEST(test_multiple_updates_accumulate);
    return UNITY_END();
}
