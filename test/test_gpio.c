#include "simulith_gpio.h"
#include "unity.h"
#include <string.h>

void setUp(void)
{
    // Nothing to do
}

void tearDown(void)
{
    // Clean up any initialized pins
    for (uint8_t port = 0; port < SIMULITH_GPIO_MAX_PORTS; port++)
    {
        for (uint8_t pin = 0; pin < SIMULITH_GPIO_MAX_PINS; pin++)
        {
            simulith_gpio_close(port, pin);
        }
    }
}

void test_gpio_init(void)
{
    simulith_gpio_config_t config = {.mode = SIMULITH_GPIO_MODE_OUTPUT, .initial_state = 0};

    // Test invalid port/pin
    TEST_ASSERT_EQUAL_INT(-1, simulith_gpio_init(8, 0, &config));
    TEST_ASSERT_EQUAL_INT(-1, simulith_gpio_init(0, 32, &config));

    // Test NULL config
    TEST_ASSERT_EQUAL_INT(-1, simulith_gpio_init(0, 0, NULL));

    // Test invalid mode
    config.mode = 5; // Invalid mode
    TEST_ASSERT_EQUAL_INT(-1, simulith_gpio_init(0, 0, &config));

    // Test invalid initial state
    config.mode          = SIMULITH_GPIO_MODE_OUTPUT;
    config.initial_state = 2; // Invalid state
    TEST_ASSERT_EQUAL_INT(-1, simulith_gpio_init(0, 0, &config));

    // Test successful initialization
    config.initial_state = 0;
    TEST_ASSERT_EQUAL_INT(0, simulith_gpio_init(0, 0, &config));

    // Test duplicate initialization
    TEST_ASSERT_EQUAL_INT(-1, simulith_gpio_init(0, 0, &config));
}

void test_gpio_output(void)
{
    simulith_gpio_config_t config = {.mode = SIMULITH_GPIO_MODE_OUTPUT, .initial_state = 0};

    // Initialize pin
    TEST_ASSERT_EQUAL_INT(0, simulith_gpio_init(0, 0, &config));

    // Test writing values
    TEST_ASSERT_EQUAL_INT(0, simulith_gpio_write(0, 0, 1));
    TEST_ASSERT_EQUAL_INT(0, simulith_gpio_write(0, 0, 0));

    // Test invalid value
    TEST_ASSERT_EQUAL_INT(-1, simulith_gpio_write(0, 0, 2));

    // Test toggle
    TEST_ASSERT_EQUAL_INT(0, simulith_gpio_toggle(0, 0));
    uint8_t value;
    TEST_ASSERT_EQUAL_INT(0, simulith_gpio_read(0, 0, &value));
    TEST_ASSERT_EQUAL_UINT8(1, value);

    TEST_ASSERT_EQUAL_INT(0, simulith_gpio_toggle(0, 0));
    TEST_ASSERT_EQUAL_INT(0, simulith_gpio_read(0, 0, &value));
    TEST_ASSERT_EQUAL_UINT8(0, value);
}

void test_gpio_input(void)
{
    simulith_gpio_config_t config = {
        .mode          = SIMULITH_GPIO_MODE_INPUT_PULLUP,
        .initial_state = 0 // Should be ignored for inputs
    };

    // Initialize pin
    TEST_ASSERT_EQUAL_INT(0, simulith_gpio_init(0, 0, &config));

    // Test reading pullup value
    uint8_t value;
    TEST_ASSERT_EQUAL_INT(0, simulith_gpio_read(0, 0, &value));
    TEST_ASSERT_EQUAL_UINT8(1, value);

    // Test input with pulldown
    config.mode = SIMULITH_GPIO_MODE_INPUT_PULLDOWN;
    TEST_ASSERT_EQUAL_INT(0, simulith_gpio_init(0, 1, &config));
    TEST_ASSERT_EQUAL_INT(0, simulith_gpio_read(0, 1, &value));
    TEST_ASSERT_EQUAL_UINT8(0, value);

    // Test floating input (should default to low)
    config.mode = SIMULITH_GPIO_MODE_INPUT;
    TEST_ASSERT_EQUAL_INT(0, simulith_gpio_init(0, 2, &config));
    TEST_ASSERT_EQUAL_INT(0, simulith_gpio_read(0, 2, &value));
    TEST_ASSERT_EQUAL_UINT8(0, value);

    // Test writing to input (should fail)
    TEST_ASSERT_EQUAL_INT(-1, simulith_gpio_write(0, 0, 1));
    TEST_ASSERT_EQUAL_INT(-1, simulith_gpio_toggle(0, 0));
}

void test_gpio_multiple_ports(void)
{
    simulith_gpio_config_t config = {.mode = SIMULITH_GPIO_MODE_OUTPUT, .initial_state = 0};

    // Initialize pins on different ports
    for (uint8_t port = 0; port < 3; port++)
    {
        TEST_ASSERT_EQUAL_INT(0, simulith_gpio_init(port, 0, &config));
    }

    // Test writing to each port
    uint8_t value;
    for (uint8_t port = 0; port < 3; port++)
    {
        TEST_ASSERT_EQUAL_INT(0, simulith_gpio_write(port, 0, 1));
        TEST_ASSERT_EQUAL_INT(0, simulith_gpio_read(port, 0, &value));
        TEST_ASSERT_EQUAL_UINT8(1, value);
    }
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_gpio_init);
    RUN_TEST(test_gpio_output);
    RUN_TEST(test_gpio_input);
    RUN_TEST(test_gpio_multiple_ports);

    return UNITY_END();
}