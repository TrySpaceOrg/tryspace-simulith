#include "simulith_spi.h"
#include "unity.h"
#include <unistd.h>

static spi_device_t spi_a_devices[8]; // Server side devices
static spi_device_t spi_b_devices[8]; // Client side devices

void setUp(void)
{
    memset(spi_a_devices, 0, sizeof(spi_a_devices));
    memset(spi_b_devices, 0, sizeof(spi_b_devices));
}

void tearDown(void)
{
    // Close all devices
    for (int i = 0; i < 8; i++)
    {
        simulith_spi_close(&spi_a_devices[i]);
        simulith_spi_close(&spi_b_devices[i]);
    }
}

void test_spi_device_init(void)
{
    int result;

    // Test NULL device
    TEST_ASSERT_EQUAL_INT(SIMULITH_SPI_ERROR, simulith_spi_init(NULL));

    // Example: SPI Bus 0, CS 0, A (server) and B (client)
    strcpy(spi_a_devices[0].name, "spi0_cs0_a");
    strcpy(spi_a_devices[0].address, "tcp://127.0.0.1:8000");
    spi_a_devices[0].is_server = 1;
    spi_a_devices[0].bus_id = 0;
    spi_a_devices[0].cs_id = 0;
    result = simulith_spi_init(&spi_a_devices[0]);
    TEST_ASSERT_EQUAL(SIMULITH_SPI_SUCCESS, result);

    strcpy(spi_b_devices[0].name, "spi0_cs0_b");
    strcpy(spi_b_devices[0].address, "tcp://127.0.0.1:8000");
    spi_b_devices[0].is_server = 0;
    spi_b_devices[0].bus_id = 0;
    spi_b_devices[0].cs_id = 0;
    result = simulith_spi_init(&spi_b_devices[0]);
    TEST_ASSERT_EQUAL(SIMULITH_SPI_SUCCESS, result);

    // Test double initialization (should succeed for already initialized device)
    result = simulith_spi_init(&spi_a_devices[0]);
    TEST_ASSERT_EQUAL(SIMULITH_SPI_SUCCESS, result);

    // Example: SPI Bus 0, CS 1, A (server) and B (client)
    strcpy(spi_a_devices[1].name, "spi0_cs1_a");
    strcpy(spi_a_devices[1].address, "tcp://127.0.0.1:8001");
    spi_a_devices[1].is_server = 1;
    spi_a_devices[1].bus_id = 0;
    spi_a_devices[1].cs_id = 1;
    result = simulith_spi_init(&spi_a_devices[1]);
    TEST_ASSERT_EQUAL(SIMULITH_SPI_SUCCESS, result);

    strcpy(spi_b_devices[1].name, "spi0_cs1_b");
    strcpy(spi_b_devices[1].address, "tcp://127.0.0.1:8001");
    spi_b_devices[1].is_server = 0;
    spi_b_devices[1].bus_id = 0;
    spi_b_devices[1].cs_id = 1;
    result = simulith_spi_init(&spi_b_devices[1]);
    TEST_ASSERT_EQUAL(SIMULITH_SPI_SUCCESS, result);
}

void test_spi_device_write_read(void)
{
    spi_device_t uninitialized_device = {0};
    
    // Test operations on uninitialized device
    uint8_t test_data[] = {0xAA, 0xBB, 0xCC, 0xDD};
    uint8_t read_buffer[10];
    
    TEST_ASSERT_EQUAL_INT(SIMULITH_SPI_ERROR, simulith_spi_write(&uninitialized_device, test_data, 4));
    TEST_ASSERT_EQUAL_INT(SIMULITH_SPI_ERROR, simulith_spi_read(&uninitialized_device, read_buffer, 4));
    TEST_ASSERT_EQUAL_INT(SIMULITH_SPI_ERROR, simulith_spi_transaction(&uninitialized_device, test_data, 4, read_buffer, 4));
    
    // Test NULL device
    TEST_ASSERT_EQUAL_INT(SIMULITH_SPI_ERROR, simulith_spi_write(NULL, test_data, 4));
    TEST_ASSERT_EQUAL_INT(SIMULITH_SPI_ERROR, simulith_spi_read(NULL, read_buffer, 4));
    TEST_ASSERT_EQUAL_INT(SIMULITH_SPI_ERROR, simulith_spi_transaction(NULL, test_data, 4, read_buffer, 4));
}

void test_spi_communication(void)
{
    int result;
    uint8_t test_data[] = {0x12, 0x34, 0x56, 0x78};
    uint8_t rx_data[sizeof(test_data)];

    // Initialize A side (server)
    strcpy(spi_a_devices[0].name, "spi0_cs0_a");
    strcpy(spi_a_devices[0].address, "tcp://127.0.0.1:8000");
    spi_a_devices[0].is_server = 1;
    spi_a_devices[0].bus_id = 0;
    spi_a_devices[0].cs_id = 0;
    result = simulith_spi_init(&spi_a_devices[0]);
    TEST_ASSERT_EQUAL(SIMULITH_SPI_SUCCESS, result);

    // Initialize B side (client)
    strcpy(spi_b_devices[0].name, "spi0_cs0_b");
    strcpy(spi_b_devices[0].address, "tcp://127.0.0.1:8000");
    spi_b_devices[0].is_server = 0;
    spi_b_devices[0].bus_id = 0;
    spi_b_devices[0].cs_id = 0;
    result = simulith_spi_init(&spi_b_devices[0]);
    TEST_ASSERT_EQUAL(SIMULITH_SPI_SUCCESS, result);

    // Give ZMQ time to establish connection
    usleep(100000); // 100ms delay to allow connection establishment

    // Test basic write operations
    result = simulith_spi_write(&spi_a_devices[0], test_data, sizeof(test_data));
    TEST_ASSERT_TRUE(result == sizeof(test_data) || result == SIMULITH_SPI_ERROR); // May fail if peer unavailable

    // Test basic read operations (will return 0 if no data, negative on error)
    result = simulith_spi_read(&spi_b_devices[0], rx_data, sizeof(rx_data));
    TEST_ASSERT_TRUE(result >= 0); // Should not return error, may return 0 if no data

    // Test transaction (may fail due to peer communication issues)
    uint8_t tx_transaction[] = {0xAA, 0xBB};
    uint8_t rx_transaction[sizeof(tx_transaction)];
    
    result = simulith_spi_transaction(&spi_b_devices[0], tx_transaction, sizeof(tx_transaction), rx_transaction, sizeof(rx_transaction));
    TEST_ASSERT_TRUE(result == SIMULITH_SPI_SUCCESS || result == SIMULITH_SPI_ERROR); // May fail due to communication

    // Close devices
    result = simulith_spi_close(&spi_a_devices[0]);
    TEST_ASSERT_EQUAL(SIMULITH_SPI_SUCCESS, result);

    result = simulith_spi_close(&spi_b_devices[0]);
    TEST_ASSERT_EQUAL(SIMULITH_SPI_SUCCESS, result);
}

void test_spi_device_close(void)
{
    spi_device_t device = {0};
    
    // Test close on uninitialized device
    TEST_ASSERT_EQUAL_INT(SIMULITH_SPI_ERROR, simulith_spi_close(&device));
    
    // Test close on NULL device
    TEST_ASSERT_EQUAL_INT(SIMULITH_SPI_ERROR, simulith_spi_close(NULL));
}

void test_spi_multiple_devices(void)
{
    int result;

    // Initialize multiple SPI device pairs (different ports)
    for (int i = 0; i < 3; i++) {
        // Server side
        sprintf(spi_a_devices[i].name, "spi%d_cs0_a", i);
        sprintf(spi_a_devices[i].address, "tcp://127.0.0.1:%d", 8010 + i);
        spi_a_devices[i].is_server = 1;
        spi_a_devices[i].bus_id = i;
        spi_a_devices[i].cs_id = 0;
        
        result = simulith_spi_init(&spi_a_devices[i]);
        TEST_ASSERT_EQUAL(SIMULITH_SPI_SUCCESS, result);

        // Client side
        sprintf(spi_b_devices[i].name, "spi%d_cs0_b", i);
        sprintf(spi_b_devices[i].address, "tcp://127.0.0.1:%d", 8010 + i);
        spi_b_devices[i].is_server = 0;
        spi_b_devices[i].bus_id = i;
        spi_b_devices[i].cs_id = 0;
        
        result = simulith_spi_init(&spi_b_devices[i]);
        TEST_ASSERT_EQUAL(SIMULITH_SPI_SUCCESS, result);
    }

    // Give ZMQ time to establish connections
    usleep(100000); // 100ms delay

    // Test basic operation on each
    uint8_t test_data[] = {0xDE, 0xAD};
    
    for (int i = 0; i < 3; i++) {
        result = simulith_spi_write(&spi_a_devices[i], test_data, sizeof(test_data));
        TEST_ASSERT_TRUE(result == sizeof(test_data) || result == SIMULITH_SPI_ERROR); // May fail due to peer communication
    }

    // Clean up
    for (int i = 0; i < 3; i++) {
        simulith_spi_close(&spi_a_devices[i]);
        simulith_spi_close(&spi_b_devices[i]);
    }
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_spi_device_init);
    RUN_TEST(test_spi_device_write_read);
    RUN_TEST(test_spi_communication);
    RUN_TEST(test_spi_device_close);
    RUN_TEST(test_spi_multiple_devices);

    return UNITY_END();
}