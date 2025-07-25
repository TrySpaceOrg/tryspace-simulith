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

static int test_i2c_read_cb(uint8_t addr, uint8_t reg, uint8_t *data, size_t len)
{
    if (addr == 0x50 && reg == 0x00 && len == 2)
    {
        data[0] = 0xAA;
        data[1] = 0xBB;
        return 0;
    }
    return -1;
}

static int test_i2c_write_cb(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len)
{
    if (addr == 0x50 && reg == 0x00 && len == 2)
    {
        return (data[0] == 0xCC && data[1] == 0xDD) ? 0 : -1;
    }
    return -1;
}

void test_i2c_init(void)
{
    // Test invalid bus ID
    TEST_ASSERT_EQUAL_INT(-1, simulith_i2c_init(8, test_i2c_read_cb, test_i2c_write_cb));

    // Test NULL callbacks
    TEST_ASSERT_EQUAL_INT(-1, simulith_i2c_init(0, NULL, test_i2c_write_cb));
    TEST_ASSERT_EQUAL_INT(-1, simulith_i2c_init(0, test_i2c_read_cb, NULL));

    // Test successful initialization
    TEST_ASSERT_EQUAL_INT(0, simulith_i2c_init(0, test_i2c_read_cb, test_i2c_write_cb));
}

void test_i2c_read_write(void)
{
    // Initialize I2C bus first
    TEST_ASSERT_EQUAL_INT(0, simulith_i2c_init(0, test_i2c_read_cb, test_i2c_write_cb));

    // Test read operation
    uint8_t read_data[2];
    TEST_ASSERT_EQUAL_INT(0, simulith_i2c_read(0, 0x50, 0x00, read_data, 2));
    TEST_ASSERT_EQUAL_HEX8(0xAA, read_data[0]);
    TEST_ASSERT_EQUAL_HEX8(0xBB, read_data[1]);

    // Test write operation
    uint8_t write_data[] = {0xCC, 0xDD};
    TEST_ASSERT_EQUAL_INT(0, simulith_i2c_write(0, 0x50, 0x00, write_data, 2));
}

void test_i2c_invalid_params(void)
{
    // Initialize I2C bus first
    TEST_ASSERT_EQUAL_INT(0, simulith_i2c_init(0, test_i2c_read_cb, test_i2c_write_cb));

    uint8_t data[2];

    // Test invalid read parameters
    TEST_ASSERT_EQUAL_INT(-1, simulith_i2c_read(8, 0x50, 0x00, data, 2)); // Invalid bus
    TEST_ASSERT_EQUAL_INT(-1, simulith_i2c_read(0, 0x50, 0x00, NULL, 2)); // NULL buffer
    TEST_ASSERT_EQUAL_INT(-1, simulith_i2c_read(0, 0x50, 0x00, data, 0)); // Zero length

    // Test invalid write parameters
    TEST_ASSERT_EQUAL_INT(-1, simulith_i2c_write(8, 0x50, 0x00, data, 2)); // Invalid bus
    TEST_ASSERT_EQUAL_INT(-1, simulith_i2c_write(0, 0x50, 0x00, NULL, 2)); // NULL buffer
    TEST_ASSERT_EQUAL_INT(-1, simulith_i2c_write(0, 0x50, 0x00, data, 0)); // Zero length
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_i2c_init);
    RUN_TEST(test_i2c_read_write);
    RUN_TEST(test_i2c_invalid_params);

    return UNITY_END();
}