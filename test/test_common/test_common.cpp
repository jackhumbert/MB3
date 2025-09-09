#include <unity.h>

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

void test_calculator_addition(void) {
    TEST_ASSERT_EQUAL(32, 32);
}

void test_calculator_subtraction(void) {
    TEST_ASSERT_EQUAL(20, 20);
}

void test_calculator_multiplication(void) {
    TEST_ASSERT_EQUAL(50, 50);
}

void test_calculator_division(void) {
    TEST_ASSERT_EQUAL(32, 32);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_calculator_addition);
    RUN_TEST(test_calculator_subtraction);
    RUN_TEST(test_calculator_multiplication);
    RUN_TEST(test_calculator_division);
    UNITY_END();

    return 0;
}