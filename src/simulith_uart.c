/*
 * Simulith UART
 *
 * Requirements:
 * - Shall utilize ZMQ to communicate between nodes
 * - Shall have functions to initialize, send, receive, check available, and flush data
 * - Shall communicate directly to the other end of the node
 * - Shall not block on any function
 * - Shall fail gracefully if peer is unavailable and return error codes (non-zero) instead of crashing
 * - Shall not rely on a server as each node will be initialized with its name and destination
*/

#include "simulith_uart.h"

int simulith_uart_init(uart_port_t *port)
{
    if (!port) return SIMULITH_UART_ERROR;
    if (port->init == SIMULITH_UART_INITIALIZED) return SIMULITH_UART_SUCCESS;

    port->zmq_ctx = zmq_ctx_new();
    if (!port->zmq_ctx) {
        simulith_log("simulith_uart_init: Failed to create ZMQ context\n");
        return SIMULITH_UART_ERROR;
    }
    port->zmq_sock = zmq_socket(port->zmq_ctx, ZMQ_PAIR);
    if (!port->zmq_sock) {
        simulith_log("simulith_uart_init: Failed to create ZMQ socket\n");
        zmq_ctx_term(port->zmq_ctx);
        return SIMULITH_UART_ERROR;
    }
    if (strlen(port->name) > 0) {
        zmq_setsockopt(port->zmq_sock, ZMQ_IDENTITY, port->name, strlen(port->name));
    }
    int rc;
    if (port->is_server) {
        rc = zmq_bind(port->zmq_sock, port->address);
        if (rc != 0) {
            simulith_log("simulith_uart_init: Failed to bind to %s\n", port->address);
            zmq_close(port->zmq_sock);
            zmq_ctx_term(port->zmq_ctx);
            return SIMULITH_UART_ERROR;
        }
        simulith_log("simulith_uart_init: Bound to %s as '%s'\n", port->address, port->name);
    } else {
        rc = zmq_connect(port->zmq_sock, port->address);
        if (rc != 0) {
            simulith_log("simulith_uart_init: Failed to connect to %s\n", port->address);
            zmq_close(port->zmq_sock);
            zmq_ctx_term(port->zmq_ctx);
            return SIMULITH_UART_ERROR;
        }
        simulith_log("simulith_uart_init: Connected to %s as '%s'\n", port->address, port->name);
    }
    port->init = SIMULITH_UART_INITIALIZED;

    // Initialize RX buffer
    port->rx_buf_len = 0;
    return SIMULITH_UART_SUCCESS;
}

int simulith_uart_send(uart_port_t *port, const uint8_t *data, size_t len)
{
    if (!port || port->init != SIMULITH_UART_INITIALIZED) {
        simulith_log("simulith_uart_send: Uninitialized UART port\n");
        return SIMULITH_UART_ERROR;
    }
    int rc = zmq_send(port->zmq_sock, data, len, ZMQ_DONTWAIT);
    if (rc < 0) {
        simulith_log("simulith_uart_send: zmq_send failed (peer may be unavailable)\n");
        return SIMULITH_UART_ERROR;
    }
    simulith_log("UART TX[%s]: %zu bytes\n", port->name, len);
    return (int)len;
}

int simulith_uart_receive(uart_port_t *port, uint8_t *data, size_t max_len)
{
    if (!port || port->init != SIMULITH_UART_INITIALIZED) {
        simulith_log("simulith_uart_receive: Uninitialized UART port\n");
        return SIMULITH_UART_ERROR;
    }
    if (port->rx_buf_len == 0) {
        // No buffered data
        return 0;
    }
    size_t to_copy = (port->rx_buf_len < max_len) ? port->rx_buf_len : max_len;
    memcpy(data, port->rx_buf, to_copy);
    // Shift remaining data in buffer
    if (to_copy < port->rx_buf_len) {
        memmove(port->rx_buf, port->rx_buf + to_copy, port->rx_buf_len - to_copy);
    }
    port->rx_buf_len -= to_copy;
    simulith_log("UART RX[%s]: %zu bytes (from buffer)\n", port->name, to_copy);
    return (int)to_copy;
}

int simulith_uart_available(uart_port_t *port)
{
    if (!port || port->init != SIMULITH_UART_INITIALIZED) {
        simulith_log("simulith_uart_available: Uninitialized UART port\n");
        return SIMULITH_UART_ERROR;
    }
    // If buffer already has data, report available
    if (port->rx_buf_len > 0) {
        return 1;
    }
    zmq_pollitem_t items[] = { { port->zmq_sock, 0, ZMQ_POLLIN, 0 } };
    int rc = zmq_poll(items, 1, 0);
    if (rc > 0 && (items[0].revents & ZMQ_POLLIN)) {
        zmq_msg_t msg;
        zmq_msg_init(&msg);
        int size = zmq_msg_recv(&msg, port->zmq_sock, ZMQ_DONTWAIT);
        if (size > 0) {
            size_t space = sizeof(port->rx_buf) - port->rx_buf_len;
            if ((size_t)size > space) {
                simulith_log("UART RX[%s]: Buffer overflow, dropping %d bytes\n", port->name, size);
                zmq_msg_close(&msg);
                return 0;
            }
            memcpy(port->rx_buf + port->rx_buf_len, zmq_msg_data(&msg), size);
            port->rx_buf_len += size;
            zmq_msg_close(&msg);
            simulith_log("UART RX[%s]: %d bytes buffered\n", port->name, size);
            return 1;
        }
        zmq_msg_close(&msg);
    }
    return 0;
}

int simulith_uart_flush(uart_port_t *port)
{
    // For ZMQ, there is no direct flush, so just return success
    if (!port || port->init != SIMULITH_UART_INITIALIZED) {
        simulith_log("simulith_uart_flush: Uninitialized UART port\n");
        return SIMULITH_UART_ERROR;
    }
    return SIMULITH_UART_SUCCESS;
}

int simulith_uart_close(uart_port_t *port)
{
    if (!port || port->init != SIMULITH_UART_INITIALIZED) {
        return SIMULITH_UART_ERROR;
    }
    zmq_close(port->zmq_sock);
    zmq_ctx_term(port->zmq_ctx);
    port->init = 0;
    simulith_log("UART port %s closed\n", port->name);
    return SIMULITH_UART_SUCCESS;
}
