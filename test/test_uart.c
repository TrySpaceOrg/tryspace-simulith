#include <unity.h>
#include "simulith_uart.h"

static uart_port_t uart_a_ports[8];
static uart_port_t uart_b_ports[8];

void setUp(void)
{
    printf("\ntest_uart.c setUp..\n");
    memset(uart_a_ports, 0, sizeof(uart_a_ports));
    memset(uart_b_ports, 0, sizeof(uart_b_ports));
}

void tearDown(void)
{
    printf("\ntest_uart.c tearDown..\n");
    // Close all ports
    for (int i = 0; i < 8; i++)
    {
        simulith_uart_close(&uart_a_ports[i]);
        simulith_uart_close(&uart_b_ports[i]);
    }
}

void test_uart_init(void)
{
    int result;

    // Example: port 0, A (server) and B (client)
    strcpy(uart_a_ports[0].name, "uart0_a");
    strcpy(uart_a_ports[0].address, "tcp://127.0.0.1:6000");
    uart_a_ports[0].is_server = 1;
    result = simulith_uart_init(&uart_a_ports[0]);
    TEST_ASSERT_EQUAL(SIMULITH_UART_SUCCESS, result);

    strcpy(uart_b_ports[0].name, "uart0_b");
    strcpy(uart_b_ports[0].address, "tcp://127.0.0.1:6000");
    uart_b_ports[0].is_server = 0;
    result = simulith_uart_init(&uart_b_ports[0]);
    TEST_ASSERT_EQUAL(SIMULITH_UART_SUCCESS, result);

    // Example: port 1, A (server) and B (client)
    strcpy(uart_a_ports[1].name, "uart1_a");
    strcpy(uart_a_ports[1].address, "tcp://127.0.0.1:6001");
    uart_a_ports[1].is_server = 1;
    result = simulith_uart_init(&uart_a_ports[1]);
    TEST_ASSERT_EQUAL(SIMULITH_UART_SUCCESS, result);

    strcpy(uart_b_ports[1].name, "uart1_b");
    strcpy(uart_b_ports[1].address, "tcp://127.0.0.1:6001");
    uart_b_ports[1].is_server = 0;
    result = simulith_uart_init(&uart_b_ports[1]);
    TEST_ASSERT_EQUAL(SIMULITH_UART_SUCCESS, result);

    // Last port
    int last = 8-1;
    sprintf(uart_a_ports[last].name, "uart%d_a", last);
    sprintf(uart_a_ports[last].address, "tcp://127.0.0.1:%d", 6000+last);
    uart_a_ports[last].is_server = 1;
    result = simulith_uart_init(&uart_a_ports[last]);
    TEST_ASSERT_EQUAL(SIMULITH_UART_SUCCESS, result);

    sprintf(uart_b_ports[last].name, "uart%d_b", last);
    sprintf(uart_b_ports[last].address, "tcp://127.0.0.1:%d", 6000+last);
    uart_b_ports[last].is_server = 0;
    result = simulith_uart_init(&uart_b_ports[last]);
    TEST_ASSERT_EQUAL(SIMULITH_UART_SUCCESS, result);
}

void test_uart_send_receive(void)
{
    int result;
    uint8_t test_data[] = {0x12, 0x34, 0x56};
    uint8_t rx_data[sizeof(test_data)];

    // Initialize A side (server)
    strcpy(uart_a_ports[0].name, "uart0_a");
    strcpy(uart_a_ports[0].address, "tcp://127.0.0.1:6000");
    uart_a_ports[0].is_server = 1;
    result = simulith_uart_init(&uart_a_ports[0]);
    TEST_ASSERT_EQUAL(SIMULITH_UART_SUCCESS, result);

    // Initialize B side (client)
    strcpy(uart_b_ports[0].name, "uart0_b");
    strcpy(uart_b_ports[0].address, "tcp://127.0.0.1:6000");
    uart_b_ports[0].is_server = 0;
    result = simulith_uart_init(&uart_b_ports[0]);
    TEST_ASSERT_EQUAL(SIMULITH_UART_SUCCESS, result);

    // Delay after initialization to allow ZMQ connection to establish
    printf("A and B initialized, waiting a second for ZMQ connection to establish...\n");
    sleep(1);

    // Send from A to B
    result = simulith_uart_send(&uart_a_ports[0], test_data, sizeof(test_data));
    TEST_ASSERT_EQUAL(sizeof(test_data), result);
    usleep(10000); // Allow time to receive

    // Confirm available on A (should be 0)
    result = simulith_uart_available(&uart_a_ports[0]);
    TEST_ASSERT_EQUAL(0, result);

    // Confirm available on B (should be 1 if data is ready)
    result = simulith_uart_available(&uart_b_ports[0]);
    TEST_ASSERT_TRUE(result == 1);

    // Receive data on B
    result = simulith_uart_receive(&uart_b_ports[0], rx_data, sizeof(rx_data));
    TEST_ASSERT_EQUAL(sizeof(test_data), result);

    // Send from B to A
    result = simulith_uart_send(&uart_b_ports[0], rx_data, sizeof(rx_data));
    TEST_ASSERT_EQUAL(sizeof(rx_data), result);
    usleep(10000); // Allow time to receive

    // Confirm available on A
    result = simulith_uart_available(&uart_a_ports[0]);
    TEST_ASSERT_TRUE(result == 1);

    // Confirm available on B
    result = simulith_uart_available(&uart_b_ports[0]);
    TEST_ASSERT_EQUAL(0, result);

    // Receive data on A
    result = simulith_uart_receive(&uart_a_ports[0], rx_data, sizeof(rx_data));
    TEST_ASSERT_EQUAL(sizeof(test_data), result);

    // Confirm available on A
    result = simulith_uart_available(&uart_a_ports[0]);
    TEST_ASSERT_EQUAL(0, result);

    // Confirm available on B
    result = simulith_uart_available(&uart_b_ports[0]);
    TEST_ASSERT_EQUAL(0, result);

    // Close A
    result = simulith_uart_close(&uart_a_ports[0]);
    TEST_ASSERT_EQUAL(SIMULITH_UART_SUCCESS, result);

    // Close B
    result = simulith_uart_close(&uart_b_ports[0]);
    TEST_ASSERT_EQUAL(SIMULITH_UART_SUCCESS, result);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_uart_init);
    RUN_TEST(test_uart_send_receive);
    return UNITY_END();
}