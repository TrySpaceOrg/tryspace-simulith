/*
 * Simulith SPI - Requirements
 *
 * Shall utilize ZMQ to communicate between nodes
 * Shall have functions to initialize, read, write, transaction (both write then read), and close
 * Shall communicate directly to the other end of the node
 * Shall not block on any function
 * Shall fail gracefully if peer is unavailable and return error codes (non-zero) instead of crashing.
 * Shall not rely on a server as each node will be initialized with its name and destination.
 */

#include "simulith_spi.h"

int simulith_spi_init(spi_device_t *device)
{
    if (!device) return SIMULITH_SPI_ERROR;
    if (device->init == SIMULITH_SPI_INITIALIZED) return SIMULITH_SPI_SUCCESS;

    device->zmq_ctx = zmq_ctx_new();
    if (!device->zmq_ctx) {
        simulith_log("simulith_spi_init: Failed to create ZMQ context\n");
        return SIMULITH_SPI_ERROR;
    }
    device->zmq_sock = zmq_socket(device->zmq_ctx, ZMQ_PAIR);
    if (!device->zmq_sock) {
        simulith_log("simulith_spi_init: Failed to create ZMQ socket\n");
        zmq_ctx_term(device->zmq_ctx);
        return SIMULITH_SPI_ERROR;
    }
    if (strlen(device->name) > 0) {
        zmq_setsockopt(device->zmq_sock, ZMQ_IDENTITY, device->name, strlen(device->name));
    }
    int rc;
    if (device->is_server) {
        rc = zmq_bind(device->zmq_sock, device->address);
        if (rc != 0) {
            simulith_log("simulith_spi_init: Failed to bind to %s\n", device->address);
            zmq_close(device->zmq_sock);
            zmq_ctx_term(device->zmq_ctx);
            return SIMULITH_SPI_ERROR;
        }
        simulith_log("simulith_spi_init: Bound to %s as '%s'\n", device->address, device->name);
    } else {
        rc = zmq_connect(device->zmq_sock, device->address);
        if (rc != 0) {
            simulith_log("simulith_spi_init: Failed to connect to %s\n", device->address);
            zmq_close(device->zmq_sock);
            zmq_ctx_term(device->zmq_ctx);
            return SIMULITH_SPI_ERROR;
        }
        simulith_log("simulith_spi_init: Connected to %s as '%s'\n", device->address, device->name);
    }
    device->init = SIMULITH_SPI_INITIALIZED;
    return SIMULITH_SPI_SUCCESS;
}

int simulith_spi_write(spi_device_t *device, const uint8_t *data, size_t len)
{
    if (!device || device->init != SIMULITH_SPI_INITIALIZED) {
        simulith_log("simulith_spi_write: Uninitialized SPI device\n");
        return SIMULITH_SPI_ERROR;
    }
    int rc = zmq_send(device->zmq_sock, data, len, ZMQ_DONTWAIT);
    if (rc < 0) {
        simulith_log("simulith_spi_write: zmq_send failed (peer may be unavailable)\n");
        return SIMULITH_SPI_ERROR;
    }
    simulith_log("SPI TX[%s]: %zu bytes\n", device->name, len);
    return (int)len;
}

int simulith_spi_read(spi_device_t *device, uint8_t *data, size_t len)
{
    if (!device || device->init != SIMULITH_SPI_INITIALIZED) {
        simulith_log("simulith_spi_read: Uninitialized SPI device\n");
        return SIMULITH_SPI_ERROR;
    }
    int rc = zmq_recv(device->zmq_sock, data, len, ZMQ_DONTWAIT);
    if (rc < 0) {
        if (zmq_errno() == EAGAIN) {
            // No data available, non-blocking
            return 0;
        }
        simulith_log("simulith_spi_read: zmq_recv failed (peer may be unavailable)\n");
        return SIMULITH_SPI_ERROR;
    }
    simulith_log("SPI RX[%s]: %d bytes\n", device->name, rc);
    return rc;
}

int simulith_spi_transaction(spi_device_t *device, const uint8_t *tx_data, size_t tx_len, uint8_t *rx_data, size_t rx_len)
{
    if (!device || device->init != SIMULITH_SPI_INITIALIZED) {
        simulith_log("simulith_spi_transaction: Uninitialized SPI device\n");
        return SIMULITH_SPI_ERROR;
    }
    
    // First, perform the write operation
    int write_result = simulith_spi_write(device, tx_data, tx_len);
    if (write_result < 0) {
        simulith_log("simulith_spi_transaction: Write operation failed\n");
        return SIMULITH_SPI_ERROR;
    }
    
    // Then, perform the read operation
    int read_result = simulith_spi_read(device, rx_data, rx_len);
    if (read_result < 0) {
        simulith_log("simulith_spi_transaction: Read operation failed\n");
        return SIMULITH_SPI_ERROR;
    }
    
    simulith_log("SPI Transaction[%s]: wrote %d bytes, read %d bytes\n", device->name, write_result, read_result);
    return SIMULITH_SPI_SUCCESS;
}

int simulith_spi_close(spi_device_t *device)
{
    if (!device || device->init != SIMULITH_SPI_INITIALIZED) {
        return SIMULITH_SPI_ERROR;
    }
    zmq_close(device->zmq_sock);
    zmq_ctx_term(device->zmq_ctx);
    device->init = 0;
    simulith_log("SPI device %s closed\n", device->name);
    return SIMULITH_SPI_SUCCESS;
}