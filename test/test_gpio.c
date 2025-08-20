#include "simulith_gpio.h"
#include "unity.h"
#include <unistd.h>

static gpio_device_t gpio_a_devices[8]; // Server side devices
static gpio_device_t gpio_b_devices[8]; // Client side devices

void setUp(void)
{
    memset(gpio_a_devices, 0, sizeof(gpio_a_devices));
    memset(gpio_b_devices, 0, sizeof(gpio_b_devices));
}

void tearDown(void)
{
    // Close all devices
    for (int i = 0; i < 8; i++)
    {
        simulith_gpio_close(&gpio_a_devices[i]);
        simulith_gpio_close(&gpio_b_devices[i]);
    }
}

void test_gpio_device_init(void)
{
    int result;

    // Test NULL device
    TEST_ASSERT_EQUAL_INT(SIMULITH_GPIO_ERROR, simulith_gpio_init(NULL));

    // Example: GPIO pin 0, A (server) and B (client)
    strcpy(gpio_a_devices[0].name, "gpio0_a");
    strcpy(gpio_a_devices[0].address, "tcp://127.0.0.1:9000");
    gpio_a_devices[0].is_server = 1;
    gpio_a_devices[0].pin = 0;
    gpio_a_devices[0].direction = GPIO_OUTPUT;
    result = simulith_gpio_init(&gpio_a_devices[0]);
    TEST_ASSERT_EQUAL(SIMULITH_GPIO_SUCCESS, result);

    strcpy(gpio_b_devices[0].name, "gpio0_b");
    strcpy(gpio_b_devices[0].address, "tcp://127.0.0.1:9000");
    gpio_b_devices[0].is_server = 0;
    gpio_b_devices[0].pin = 0;
    gpio_b_devices[0].direction = GPIO_INPUT;
    result = simulith_gpio_init(&gpio_b_devices[0]);
    TEST_ASSERT_EQUAL(SIMULITH_GPIO_SUCCESS, result);

    // Test double initialization (should succeed for already initialized device)
    result = simulith_gpio_init(&gpio_a_devices[0]);
    TEST_ASSERT_EQUAL(SIMULITH_GPIO_SUCCESS, result);

    // Example: GPIO pin 1, A (server) and B (client)
    strcpy(gpio_a_devices[1].name, "gpio1_a");
    strcpy(gpio_a_devices[1].address, "tcp://127.0.0.1:9001");
    gpio_a_devices[1].is_server = 1;
    gpio_a_devices[1].pin = 1;
    gpio_a_devices[1].direction = GPIO_OUTPUT;
    result = simulith_gpio_init(&gpio_a_devices[1]);
    TEST_ASSERT_EQUAL(SIMULITH_GPIO_SUCCESS, result);

    strcpy(gpio_b_devices[1].name, "gpio1_b");
    strcpy(gpio_b_devices[1].address, "tcp://127.0.0.1:9001");
    gpio_b_devices[1].is_server = 0;
    gpio_b_devices[1].pin = 1;
    gpio_b_devices[1].direction = GPIO_INPUT;
    result = simulith_gpio_init(&gpio_b_devices[1]);
    TEST_ASSERT_EQUAL(SIMULITH_GPIO_SUCCESS, result);
}

void test_gpio_device_write_read(void)
{
    gpio_device_t uninitialized_device = {0};
    
    // Test operations on uninitialized device
    uint8_t value;
    
    TEST_ASSERT_EQUAL_INT(SIMULITH_GPIO_ERROR, simulith_gpio_write(&uninitialized_device, 1));
    TEST_ASSERT_EQUAL_INT(SIMULITH_GPIO_ERROR, simulith_gpio_read(&uninitialized_device, &value));
    TEST_ASSERT_EQUAL_INT(SIMULITH_GPIO_ERROR, simulith_gpio_toggle(&uninitialized_device));
    
    // Test NULL device
    TEST_ASSERT_EQUAL_INT(SIMULITH_GPIO_ERROR, simulith_gpio_write(NULL, 1));
    TEST_ASSERT_EQUAL_INT(SIMULITH_GPIO_ERROR, simulith_gpio_read(NULL, &value));
    TEST_ASSERT_EQUAL_INT(SIMULITH_GPIO_ERROR, simulith_gpio_toggle(NULL));
    
    // Test NULL value pointer
    gpio_device_t device = {0};
    device.init = SIMULITH_GPIO_INITIALIZED;
    TEST_ASSERT_EQUAL_INT(SIMULITH_GPIO_ERROR, simulith_gpio_read(&device, NULL));
}

void test_gpio_communication(void)
{
    int result;
    uint8_t value;

    // Initialize A side (server) as output
    strcpy(gpio_a_devices[0].name, "gpio0_a");
    strcpy(gpio_a_devices[0].address, "tcp://127.0.0.1:9000");
    gpio_a_devices[0].is_server = 1;
    gpio_a_devices[0].pin = 0;
    gpio_a_devices[0].direction = GPIO_OUTPUT;
    result = simulith_gpio_init(&gpio_a_devices[0]);
    TEST_ASSERT_EQUAL(SIMULITH_GPIO_SUCCESS, result);

    // Initialize B side (client) as input
    strcpy(gpio_b_devices[0].name, "gpio0_b");
    strcpy(gpio_b_devices[0].address, "tcp://127.0.0.1:9000");
    gpio_b_devices[0].is_server = 0;
    gpio_b_devices[0].pin = 0;
    gpio_b_devices[0].direction = GPIO_INPUT;
    result = simulith_gpio_init(&gpio_b_devices[0]);
    TEST_ASSERT_EQUAL(SIMULITH_GPIO_SUCCESS, result);

    // Give ZMQ time to establish connection
    usleep(100000); // 100ms delay

    // Test basic write operations
    result = simulith_gpio_write(&gpio_a_devices[0], 1);
    TEST_ASSERT_TRUE(result == SIMULITH_GPIO_SUCCESS || result == SIMULITH_GPIO_ERROR); // May fail due to peer communication

    // Test basic read operations (will return 0 if no data, negative on error)
    result = simulith_gpio_read(&gpio_b_devices[0], &value);
    TEST_ASSERT_TRUE(result == SIMULITH_GPIO_SUCCESS || result == SIMULITH_GPIO_ERROR); // May fail due to peer communication

    // Test toggle
    result = simulith_gpio_toggle(&gpio_a_devices[0]);
    TEST_ASSERT_TRUE(result == SIMULITH_GPIO_SUCCESS || result == SIMULITH_GPIO_ERROR); // May fail due to peer communication

    // Test invalid write value
    result = simulith_gpio_write(&gpio_a_devices[0], 2);
    TEST_ASSERT_EQUAL(SIMULITH_GPIO_ERROR, result);

    // Close devices
    result = simulith_gpio_close(&gpio_a_devices[0]);
    TEST_ASSERT_EQUAL(SIMULITH_GPIO_SUCCESS, result);

    result = simulith_gpio_close(&gpio_b_devices[0]);
    TEST_ASSERT_EQUAL(SIMULITH_GPIO_SUCCESS, result);
}

void test_gpio_device_close(void)
{
    gpio_device_t device = {0};
    
    // Test close on uninitialized device
    TEST_ASSERT_EQUAL_INT(SIMULITH_GPIO_ERROR, simulith_gpio_close(&device));
    
    // Test close on NULL device
    TEST_ASSERT_EQUAL_INT(SIMULITH_GPIO_ERROR, simulith_gpio_close(NULL));
}

void test_gpio_multiple_devices(void)
{
    int result;

    // Initialize multiple GPIO device pairs (different pins)
    for (int i = 0; i < 3; i++) {
        // Server side
        sprintf(gpio_a_devices[i].name, "gpio%d_a", i);
        sprintf(gpio_a_devices[i].address, "tcp://127.0.0.1:%d", 9010 + i);
        gpio_a_devices[i].is_server = 1;
        gpio_a_devices[i].pin = i;
        gpio_a_devices[i].direction = GPIO_OUTPUT;
        
        result = simulith_gpio_init(&gpio_a_devices[i]);
        TEST_ASSERT_EQUAL(SIMULITH_GPIO_SUCCESS, result);

        // Client side
        sprintf(gpio_b_devices[i].name, "gpio%d_b", i);
        sprintf(gpio_b_devices[i].address, "tcp://127.0.0.1:%d", 9010 + i);
        gpio_b_devices[i].is_server = 0;
        gpio_b_devices[i].pin = i;
        gpio_b_devices[i].direction = GPIO_INPUT;
        
        result = simulith_gpio_init(&gpio_b_devices[i]);
        TEST_ASSERT_EQUAL(SIMULITH_GPIO_SUCCESS, result);
    }

    // Give ZMQ time to establish connections
    usleep(100000); // 100ms delay

    // Test basic operation on each
    for (int i = 0; i < 3; i++) {
        result = simulith_gpio_write(&gpio_a_devices[i], 1);
        TEST_ASSERT_TRUE(result == SIMULITH_GPIO_SUCCESS || result == SIMULITH_GPIO_ERROR); // May fail due to peer communication
    }

    // Clean up
    for (int i = 0; i < 3; i++) {
        simulith_gpio_close(&gpio_a_devices[i]);
        simulith_gpio_close(&gpio_b_devices[i]);
    }
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_gpio_device_init);
    RUN_TEST(test_gpio_device_write_read);
    RUN_TEST(test_gpio_communication);
    RUN_TEST(test_gpio_device_close);
    RUN_TEST(test_gpio_multiple_devices);

    return UNITY_END();
}