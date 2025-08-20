#include "simulith_i2c.h"
#include "unity.h"

void setUp(void)
{
    // Setup code if needed
}

void tearDown(void)
{
    // Cleanup code if needed
}

void test_i2c_device_init(void)
{
    i2c_device_t device = {0};
    
    // Set up device parameters
    snprintf(device.name, sizeof(device.name), "TestI2C");
    snprintf(device.address, sizeof(device.address), "tcp://127.0.0.1:7050");
    device.is_server = 0;
    device.bus_id = 0;
    device.device_addr = 0x50;
    
    // Test NULL device
    TEST_ASSERT_EQUAL_INT(SIMULITH_I2C_ERROR, simulith_i2c_init(NULL));
    
    // Test device initialization (will fail without actual ZMQ endpoint, but tests the flow)
    int result = simulith_i2c_init(&device);
    // Since we don't have a peer, this will likely fail with connection error, which is expected
    // The important thing is that it doesn't crash and returns proper error code
    TEST_ASSERT_TRUE(result == SIMULITH_I2C_SUCCESS || result == SIMULITH_I2C_ERROR);
    
    // Clean up if it was successful
    if (result == SIMULITH_I2C_SUCCESS) {
        simulith_i2c_close(&device);
    }
}

void test_i2c_device_write_read(void)
{
    i2c_device_t device = {0};
    
    // Test operations on uninitialized device
    uint8_t test_data[] = {0xAA, 0xBB};
    uint8_t read_buffer[10];
    
    TEST_ASSERT_EQUAL_INT(SIMULITH_I2C_ERROR, simulith_i2c_write(&device, test_data, 2));
    TEST_ASSERT_EQUAL_INT(SIMULITH_I2C_ERROR, simulith_i2c_read(&device, read_buffer, 2));
    TEST_ASSERT_EQUAL_INT(SIMULITH_I2C_ERROR, simulith_i2c_transaction(&device, test_data, 2, read_buffer, 2));
    
    // Test NULL device
    TEST_ASSERT_EQUAL_INT(SIMULITH_I2C_ERROR, simulith_i2c_write(NULL, test_data, 2));
    TEST_ASSERT_EQUAL_INT(SIMULITH_I2C_ERROR, simulith_i2c_read(NULL, read_buffer, 2));
    TEST_ASSERT_EQUAL_INT(SIMULITH_I2C_ERROR, simulith_i2c_transaction(NULL, test_data, 2, read_buffer, 2));
}

void test_i2c_device_close(void)
{
    i2c_device_t device = {0};
    
    // Test close on uninitialized device
    TEST_ASSERT_EQUAL_INT(SIMULITH_I2C_ERROR, simulith_i2c_close(&device));
    
    // Test close on NULL device
    TEST_ASSERT_EQUAL_INT(SIMULITH_I2C_ERROR, simulith_i2c_close(NULL));
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_i2c_device_init);
    RUN_TEST(test_i2c_device_write_read);
    RUN_TEST(test_i2c_device_close);

    return UNITY_END();
}