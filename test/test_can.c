#include "simulith_can.h"
#include "unity.h"
#include <string.h>

static int                    test_rx_count = 0;
static simulith_can_message_t last_received_msg;

void setUp(void)
{
    test_rx_count = 0;
    memset(&last_received_msg, 0, sizeof(last_received_msg));
}

void tearDown(void)
{
    // Cleanup code if needed
}

static int test_can_rx_cb(uint8_t bus_id, const simulith_can_message_t *msg)
{
    test_rx_count++;
    memcpy(&last_received_msg, msg, sizeof(simulith_can_message_t));
    return 0;
}

void test_can_init(void)
{
    simulith_can_config_t config = {.bitrate = SIMULITH_CAN_BITRATE_500K, .sample_point = 75, .sync_jump = 1};

    // Test invalid bus ID
    TEST_ASSERT_EQUAL_INT(-1, simulith_can_init(8, &config, test_can_rx_cb));

    // Test NULL config
    TEST_ASSERT_EQUAL_INT(-1, simulith_can_init(0, NULL, test_can_rx_cb));

    // Test invalid bitrate
    config.bitrate = 50000; // Below minimum
    TEST_ASSERT_EQUAL_INT(-1, simulith_can_init(0, &config, test_can_rx_cb));

    // Test invalid sample point
    config.bitrate      = SIMULITH_CAN_BITRATE_500K;
    config.sample_point = 95; // Above maximum
    TEST_ASSERT_EQUAL_INT(-1, simulith_can_init(0, &config, test_can_rx_cb));

    // Test invalid sync jump
    config.sample_point = 75;
    config.sync_jump    = 5; // Above maximum
    TEST_ASSERT_EQUAL_INT(-1, simulith_can_init(0, &config, test_can_rx_cb));

    // Test successful initialization
    config.sync_jump = 1;
    TEST_ASSERT_EQUAL_INT(0, simulith_can_init(0, &config, test_can_rx_cb));

    // Test duplicate initialization
    TEST_ASSERT_EQUAL_INT(-1, simulith_can_init(0, &config, test_can_rx_cb));

    // Clean up
    simulith_can_close(0);
}

void test_can_filters(void)
{
    simulith_can_config_t config = {.bitrate = SIMULITH_CAN_BITRATE_500K, .sample_point = 75, .sync_jump = 1};

    // Initialize CAN bus
    TEST_ASSERT_EQUAL_INT(0, simulith_can_init(0, &config, test_can_rx_cb));

    // Test adding filters
    simulith_can_filter_t filter = {.id = 0x123, .mask = 0x7FF, .is_extended = 0};

    int filter_id = simulith_can_add_filter(0, &filter);
    TEST_ASSERT_GREATER_OR_EQUAL_INT(0, filter_id);

    // Test removing filters
    TEST_ASSERT_EQUAL_INT(0, simulith_can_remove_filter(0, filter_id));

    // Test invalid filter removal
    TEST_ASSERT_EQUAL_INT(-1, simulith_can_remove_filter(0, -1));
    TEST_ASSERT_EQUAL_INT(-1, simulith_can_remove_filter(0, 16));

    // Clean up
    simulith_can_close(0);
}

void test_can_send_receive(void)
{
    simulith_can_config_t config = {.bitrate = SIMULITH_CAN_BITRATE_500K, .sample_point = 75, .sync_jump = 1};

    // Initialize CAN bus
    TEST_ASSERT_EQUAL_INT(0, simulith_can_init(0, &config, test_can_rx_cb));

    // Add filter for ID 0x123
    simulith_can_filter_t filter = {.id = 0x123, .mask = 0x7FF, .is_extended = 0};
    TEST_ASSERT_GREATER_OR_EQUAL_INT(0, simulith_can_add_filter(0, &filter));

    // Test sending standard ID message
    simulith_can_message_t tx_msg = {
        .id = 0x123, .is_extended = 0, .is_rtr = 0, .dlc = 8, .data = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88}};

    TEST_ASSERT_EQUAL_INT(0, simulith_can_send(0, &tx_msg));

    // Test receiving message
    simulith_can_message_t rx_msg;
    TEST_ASSERT_EQUAL_INT(1, simulith_can_receive(0, &rx_msg));

    // Verify received message
    TEST_ASSERT_EQUAL_UINT32(tx_msg.id, rx_msg.id);
    TEST_ASSERT_EQUAL_UINT8(tx_msg.is_extended, rx_msg.is_extended);
    TEST_ASSERT_EQUAL_UINT8(tx_msg.is_rtr, rx_msg.is_rtr);
    TEST_ASSERT_EQUAL_UINT8(tx_msg.dlc, rx_msg.dlc);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(tx_msg.data, rx_msg.data, tx_msg.dlc);

    // Test sending extended ID message
    tx_msg.id          = 0x12345678;
    tx_msg.is_extended = 1;
    TEST_ASSERT_EQUAL_INT(0, simulith_can_send(0, &tx_msg));

    // Test receiving extended ID message
    TEST_ASSERT_EQUAL_INT(1, simulith_can_receive(0, &rx_msg));
    TEST_ASSERT_EQUAL_UINT32(tx_msg.id, rx_msg.id);
    TEST_ASSERT_EQUAL_UINT8(1, rx_msg.is_extended);

    // Test invalid message parameters
    tx_msg.id = SIMULITH_CAN_ID_EXT_MAX + 1;
    TEST_ASSERT_EQUAL_INT(-1, simulith_can_send(0, &tx_msg));

    tx_msg.id  = 0x123;
    tx_msg.dlc = 9; // Invalid DLC
    TEST_ASSERT_EQUAL_INT(-1, simulith_can_send(0, &tx_msg));

    // Clean up
    simulith_can_close(0);
}

void test_can_multiple_buses(void)
{
    simulith_can_config_t config = {.bitrate = SIMULITH_CAN_BITRATE_500K, .sample_point = 75, .sync_jump = 1};

    // Initialize multiple CAN buses
    for (uint8_t i = 0; i < 3; i++)
    {
        TEST_ASSERT_EQUAL_INT(0, simulith_can_init(i, &config, test_can_rx_cb));
    }

    // Test sending messages on each bus
    simulith_can_message_t tx_msg = {.id = 0x123, .is_extended = 0, .is_rtr = 0, .dlc = 2, .data = {0xAA, 0xBB}};

    for (uint8_t i = 0; i < 3; i++)
    {
        TEST_ASSERT_EQUAL_INT(0, simulith_can_send(i, &tx_msg));

        simulith_can_message_t rx_msg;
        TEST_ASSERT_EQUAL_INT(1, simulith_can_receive(i, &rx_msg));
        TEST_ASSERT_EQUAL_UINT32(tx_msg.id, rx_msg.id);
    }

    // Clean up
    for (uint8_t i = 0; i < 3; i++)
    {
        simulith_can_close(i);
    }
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_can_init);
    RUN_TEST(test_can_filters);
    RUN_TEST(test_can_send_receive);
    RUN_TEST(test_can_multiple_buses);

    return UNITY_END();
}