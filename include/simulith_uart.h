/*
 * Simulith UART - Requirements
 *
 * Shall utilize ZMQ to communicate between nodes
 * Shall have functions to initialize, send, receive, check available, and flush data
 * Shall communicate directly to the other end of the node
 * Shall not block on any function
 * Shall fail gracefully if peer is unavailable and return error codes (non-zero) instead of crashing.
 * Shall not rely on a server as each node will be initialized with its name and destination.
 */

#ifndef SIMULITH_UART_H
#define SIMULITH_UART_H

#include "simulith.h"

#define SIMULITH_UART_SUCCESS 0
#define SIMULITH_UART_ERROR  -1

#define SIMULITH_UART_INITIALIZED 255

#define SIMULITH_UART_BASE_PORT 6000

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
    char name[64];
    char address[128];
    int is_server;
    void* zmq_ctx;
    void* zmq_sock;
    int init;
    // RX buffer for incoming data
    uint8_t rx_buf[1024];
    size_t rx_buf_len;
} uart_port_t;

    int simulith_uart_init(uart_port_t *port);
    int simulith_uart_send(uart_port_t *port, const uint8_t *data, size_t len);
    int simulith_uart_receive(uart_port_t *port, uint8_t *data, size_t max_len);
    int simulith_uart_available(uart_port_t *port);
    int simulith_uart_flush(uart_port_t *port);
    int simulith_uart_close(uart_port_t *port);

#ifdef __cplusplus
}
#endif

#endif /* SIMULITH_UART_H */