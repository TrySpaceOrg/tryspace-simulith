#include "simulith_pwm.h"
#include "unity.h"
#include <string.h>

void setUp(void)
{
    // Nothing to do
}

void tearDown(void)
{
    // Clean up any initialized channels
    for (uint8_t channel = 0; channel < SIMULITH_PWM_MAX_CHANNELS; channel++)
    {
        simulith_pwm_close(channel);
    }
}

void test_pwm_init(void)
{
    simulith_pwm_config_t config = {.frequency_hz = 1000, .duty_cycle = 50};

    // Test invalid channel
    TEST_ASSERT_EQUAL_INT(-1, simulith_pwm_init(SIMULITH_PWM_MAX_CHANNELS, &config));

    // Test NULL config
    TEST_ASSERT_EQUAL_INT(-1, simulith_pwm_init(0, NULL));

    // Test invalid frequency
    config.frequency_hz = 0; // Below minimum
    TEST_ASSERT_EQUAL_INT(-1, simulith_pwm_init(0, &config));
    config.frequency_hz = SIMULITH_PWM_MAX_FREQ_HZ + 1; // Above maximum
    TEST_ASSERT_EQUAL_INT(-1, simulith_pwm_init(0, &config));

    // Test invalid duty cycle
    config.frequency_hz = 1000;
    config.duty_cycle   = 101; // Above 100%
    TEST_ASSERT_EQUAL_INT(-1, simulith_pwm_init(0, &config));

    // Test successful initialization
    config.duty_cycle = 50;
    TEST_ASSERT_EQUAL_INT(0, simulith_pwm_init(0, &config));

    // Test duplicate initialization
    TEST_ASSERT_EQUAL_INT(-1, simulith_pwm_init(0, &config));
}

void test_pwm_start_stop(void)
{
    simulith_pwm_config_t config = {.frequency_hz = 1000, .duty_cycle = 50};

    // Test start/stop on uninitialized channel
    TEST_ASSERT_EQUAL_INT(-1, simulith_pwm_start(0));
    TEST_ASSERT_EQUAL_INT(-1, simulith_pwm_stop(0));

    // Initialize channel
    TEST_ASSERT_EQUAL_INT(0, simulith_pwm_init(0, &config));

    // Test start/stop sequence
    TEST_ASSERT_EQUAL_INT(0, simulith_pwm_start(0));
    TEST_ASSERT_EQUAL_INT(0, simulith_pwm_stop(0));

    // Test invalid channel
    TEST_ASSERT_EQUAL_INT(-1, simulith_pwm_start(SIMULITH_PWM_MAX_CHANNELS));
    TEST_ASSERT_EQUAL_INT(-1, simulith_pwm_stop(SIMULITH_PWM_MAX_CHANNELS));
}

void test_pwm_duty_cycle(void)
{
    simulith_pwm_config_t config = {.frequency_hz = 1000, .duty_cycle = 50};

    // Initialize channel
    TEST_ASSERT_EQUAL_INT(0, simulith_pwm_init(0, &config));

    // Test valid duty cycle changes
    TEST_ASSERT_EQUAL_INT(0, simulith_pwm_set_duty(0, 0));   // 0%
    TEST_ASSERT_EQUAL_INT(0, simulith_pwm_set_duty(0, 100)); // 100%
    TEST_ASSERT_EQUAL_INT(0, simulith_pwm_set_duty(0, 25));  // 25%

    // Test invalid duty cycle
    TEST_ASSERT_EQUAL_INT(-1, simulith_pwm_set_duty(0, 101));

    // Test uninitialized channel
    TEST_ASSERT_EQUAL_INT(-1, simulith_pwm_set_duty(1, 50));

    // Test invalid channel
    TEST_ASSERT_EQUAL_INT(-1, simulith_pwm_set_duty(SIMULITH_PWM_MAX_CHANNELS, 50));
}

void test_pwm_frequency(void)
{
    simulith_pwm_config_t config = {.frequency_hz = 1000, .duty_cycle = 50};

    // Initialize channel
    TEST_ASSERT_EQUAL_INT(0, simulith_pwm_init(0, &config));

    // Test valid frequency changes
    TEST_ASSERT_EQUAL_INT(0, simulith_pwm_set_frequency(0, SIMULITH_PWM_MIN_FREQ_HZ));
    TEST_ASSERT_EQUAL_INT(0, simulith_pwm_set_frequency(0, SIMULITH_PWM_MAX_FREQ_HZ));
    TEST_ASSERT_EQUAL_INT(0, simulith_pwm_set_frequency(0, 10000));

    // Test invalid frequencies
    TEST_ASSERT_EQUAL_INT(-1, simulith_pwm_set_frequency(0, 0));
    TEST_ASSERT_EQUAL_INT(-1, simulith_pwm_set_frequency(0, SIMULITH_PWM_MAX_FREQ_HZ + 1));

    // Test uninitialized channel
    TEST_ASSERT_EQUAL_INT(-1, simulith_pwm_set_frequency(1, 1000));

    // Test invalid channel
    TEST_ASSERT_EQUAL_INT(-1, simulith_pwm_set_frequency(SIMULITH_PWM_MAX_CHANNELS, 1000));
}

void test_pwm_close(void)
{
    simulith_pwm_config_t config = {.frequency_hz = 1000, .duty_cycle = 50};

    // Initialize and start channel
    TEST_ASSERT_EQUAL_INT(0, simulith_pwm_init(0, &config));
    TEST_ASSERT_EQUAL_INT(0, simulith_pwm_start(0));

    // Close channel
    TEST_ASSERT_EQUAL_INT(0, simulith_pwm_close(0));

    // Verify channel can be reinitialized
    TEST_ASSERT_EQUAL_INT(0, simulith_pwm_init(0, &config));

    // Test closing uninitialized channel
    TEST_ASSERT_EQUAL_INT(-1, simulith_pwm_close(1));

    // Test invalid channel
    TEST_ASSERT_EQUAL_INT(-1, simulith_pwm_close(SIMULITH_PWM_MAX_CHANNELS));
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_pwm_init);
    RUN_TEST(test_pwm_start_stop);
    RUN_TEST(test_pwm_duty_cycle);
    RUN_TEST(test_pwm_frequency);
    RUN_TEST(test_pwm_close);

    return UNITY_END();
}