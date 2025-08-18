#include "simulith_spi.h"
#include "unity.h"
#include <string.h>

void setUp(void)
{
    // Setup code if needed
}

void tearDown(void)
{
    // Cleanup code if needed
}

// Test device that echoes back data with each byte incremented by 1
static int test_spi_transfer_cb(uint8_t bus_id, uint8_t cs_id, const uint8_t *tx_data, uint8_t *rx_data, size_t len)
{
    if (cs_id == 0 && rx_data)
    { // Echo device on CS0
        if (tx_data)
        {
            for (size_t i = 0; i < len; i++)
            {
                rx_data[i] = tx_data[i] + 1;
            }
        }
        else
        {
            memset(rx_data, 0xFF, len); // Return all 1's for read-only
        }
        return len;
    }
    return -1; // Unknown device
}

void test_spi_init(void)
{
    simulith_spi_config_t config = {.clock_hz    = 1000000, // 1 MHz
                                    .mode        = SIMULITH_SPI_MODE_0,
                                    .bit_order   = SIMULITH_SPI_MSB_FIRST,
                                    .cs_polarity = SIMULITH_SPI_CS_ACTIVE_LOW,
                                    .data_bits   = 8};

    // Test invalid bus ID
    TEST_ASSERT_EQUAL_INT(-1, simulith_spi_init(8, &config, test_spi_transfer_cb));

    // Test NULL config
    TEST_ASSERT_EQUAL_INT(-1, simulith_spi_init(0, NULL, test_spi_transfer_cb));

    // Test NULL callback
    TEST_ASSERT_EQUAL_INT(-1, simulith_spi_init(0, &config, NULL));

    // Test invalid clock frequency
    config.clock_hz = 500; // Below 1kHz
    TEST_ASSERT_EQUAL_INT(-1, simulith_spi_init(0, &config, test_spi_transfer_cb));

    // Test invalid mode
    config.clock_hz = 1000000;
    config.mode     = 4; // Invalid mode
    TEST_ASSERT_EQUAL_INT(-1, simulith_spi_init(0, &config, test_spi_transfer_cb));

    // Test invalid data bits
    config.mode      = SIMULITH_SPI_MODE_0;
    config.data_bits = 17; // Above maximum
    TEST_ASSERT_EQUAL_INT(-1, simulith_spi_init(0, &config, test_spi_transfer_cb));

    // Test successful initialization
    config.data_bits = 8;
    TEST_ASSERT_EQUAL_INT(0, simulith_spi_init(0, &config, test_spi_transfer_cb));

    // Test duplicate initialization
    TEST_ASSERT_EQUAL_INT(-1, simulith_spi_init(0, &config, test_spi_transfer_cb));

    // Clean up
    simulith_spi_close(0);
}

void test_spi_transfer(void)
{
    simulith_spi_config_t config = {.clock_hz    = 1000000,
                                    .mode        = SIMULITH_SPI_MODE_0,
                                    .bit_order   = SIMULITH_SPI_MSB_FIRST,
                                    .cs_polarity = SIMULITH_SPI_CS_ACTIVE_LOW,
                                    .data_bits   = 8};

    // Initialize SPI
    TEST_ASSERT_EQUAL_INT(0, simulith_spi_init(0, &config, test_spi_transfer_cb));

    // Test transmit and receive
    const uint8_t tx_data[] = {0x12, 0x34, 0x56, 0x78};
    uint8_t       rx_data[4];

    TEST_ASSERT_EQUAL_INT(sizeof(tx_data), simulith_spi_transfer(0, 0, tx_data, rx_data, sizeof(tx_data)));

    // Verify received data (each byte should be incremented by 1)
    for (size_t i = 0; i < sizeof(tx_data); i++)
    {
        TEST_ASSERT_EQUAL_HEX8(tx_data[i] + 1, rx_data[i]);
    }

    // Test receive-only (tx_data = NULL)
    TEST_ASSERT_EQUAL_INT(4, simulith_spi_transfer(0, 0, NULL, rx_data, 4));

    // Verify all 0xFF was received
    for (size_t i = 0; i < 4; i++)
    {
        TEST_ASSERT_EQUAL_HEX8(0xFF, rx_data[i]);
    }

    // Clean up
    simulith_spi_close(0);
}

void test_spi_invalid_operations(void)
{
    simulith_spi_config_t config = {.clock_hz    = 1000000,
                                    .mode        = SIMULITH_SPI_MODE_0,
                                    .bit_order   = SIMULITH_SPI_MSB_FIRST,
                                    .cs_polarity = SIMULITH_SPI_CS_ACTIVE_LOW,
                                    .data_bits   = 8};

    uint8_t data[4] = {0x00, 0x00, 0x00, 0x00};

    // Test operations on uninitialized bus
    TEST_ASSERT_EQUAL_INT(-1, simulith_spi_transfer(0, 0, data, data, sizeof(data)));
    TEST_ASSERT_EQUAL_INT(-1, simulith_spi_close(0));

    // Initialize bus
    TEST_ASSERT_EQUAL_INT(0, simulith_spi_init(0, &config, test_spi_transfer_cb));

    // Test invalid parameters
    TEST_ASSERT_EQUAL_INT(-1, simulith_spi_transfer(0, 8, data, data, sizeof(data))); // Invalid CS
    TEST_ASSERT_EQUAL_INT(-1, simulith_spi_transfer(0, 0, NULL, NULL, sizeof(data))); // Both buffers NULL
    TEST_ASSERT_EQUAL_INT(0, simulith_spi_transfer(0, 0, data, data, 0));             // Zero length

    // Test transfer to non-existent device
    TEST_ASSERT_EQUAL_INT(-1, simulith_spi_transfer(0, 1, data, data, sizeof(data)));

    // Clean up
    simulith_spi_close(0);
}

void test_spi_multiple_buses(void)
{
    simulith_spi_config_t config = {.clock_hz    = 1000000,
                                    .mode        = SIMULITH_SPI_MODE_0,
                                    .bit_order   = SIMULITH_SPI_MSB_FIRST,
                                    .cs_polarity = SIMULITH_SPI_CS_ACTIVE_LOW,
                                    .data_bits   = 8};

    // Initialize multiple buses
    for (uint8_t i = 0; i < 3; i++)
    {
        TEST_ASSERT_EQUAL_INT(0, simulith_spi_init(i, &config, test_spi_transfer_cb));
    }

    // Test transfer on each bus
    const uint8_t tx_data[] = {0xAA, 0xBB};
    uint8_t       rx_data[2];

    for (uint8_t i = 0; i < 3; i++)
    {
        TEST_ASSERT_EQUAL_INT(sizeof(tx_data), simulith_spi_transfer(i, 0, tx_data, rx_data, sizeof(tx_data)));
    }

    // Clean up
    for (uint8_t i = 0; i < 3; i++)
    {
        simulith_spi_close(i);
    }
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_spi_init);
    RUN_TEST(test_spi_transfer);
    RUN_TEST(test_spi_invalid_operations);
    RUN_TEST(test_spi_multiple_buses);

    return UNITY_END();
}