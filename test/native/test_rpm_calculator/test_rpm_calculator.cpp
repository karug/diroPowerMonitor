#include <unity.h>
#include "domain/services/RpmCalculator.h"

void setUp() {}
void tearDown() {}

void test_zero_pulses_returns_zero_rpm() {
    RpmCalculator calc(1);
    Rpm rpm = calc.calculate(0, 3000);
    TEST_ASSERT_EQUAL_UINT16(0, rpm.value);
}

void test_60_pulses_in_1min_with_1ppr_equals_60rpm() {
    RpmCalculator calc(1);
    Rpm rpm = calc.calculate(60, 60000);   // 60 pulses in 1 minute
    TEST_ASSERT_EQUAL_UINT16(60, rpm.value);
}

void test_60_pulses_in_1min_with_2ppr_equals_30rpm() {
    RpmCalculator calc(2);
    Rpm rpm = calc.calculate(60, 60000);   // 60 pulses in 1 minute, 2 PPR = 30 RPM
    TEST_ASSERT_EQUAL_UINT16(30, rpm.value);
}

void test_set_pulses_per_rev_changes_divisor() {
    RpmCalculator calc(1);
    calc.setPulsesPerRev(3);
    Rpm rpm = calc.calculate(90, 60000);   // 90 pulses, 3 PPR = 30 rev in 1 min = 30 RPM
    TEST_ASSERT_EQUAL_UINT16(30, rpm.value);
}

void test_zero_window_returns_zero_rpm() {
    RpmCalculator calc(1);
    Rpm rpm = calc.calculate(100, 0);
    TEST_ASSERT_EQUAL_UINT16(0, rpm.value);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_zero_pulses_returns_zero_rpm);
    RUN_TEST(test_60_pulses_in_1min_with_1ppr_equals_60rpm);
    RUN_TEST(test_60_pulses_in_1min_with_2ppr_equals_30rpm);
    RUN_TEST(test_set_pulses_per_rev_changes_divisor);
    RUN_TEST(test_zero_window_returns_zero_rpm);
    return UNITY_END();
}
