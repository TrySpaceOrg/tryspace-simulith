/*
 * Simulith GPIO - Requirements
 *
 * Shall utilize ZMQ to communicate between nodes
 * Shall have functions to initialize, read, write, toggle, and close
 * Shall communicate directly to the other end of the node
 * Shall not block on any function
 * Shall fail gracefully if peer is unavailable and return error codes (non-zero) instead of crashing.
 * Shall not rely on a server as each node will be initialized with its name and destination.
 */

#include "simulith_gpio.h"

// Internal message format for GPIO communication
typedef struct {
    uint8_t command; // 0=read, 1=write, 2=toggle
    uint8_t value;   // value for write, or current value for read response
} gpio_message_t;

int simulith_gpio_init(gpio_device_t *device)
{
    if (!device) return SIMULITH_GPIO_ERROR;
    if (device->init == SIMULITH_GPIO_INITIALIZED) return SIMULITH_GPIO_SUCCESS;

    device->zmq_ctx = zmq_ctx_new();
    if (!device->zmq_ctx) {
        simulith_log("simulith_gpio_init: Failed to create ZMQ context\n");
        return SIMULITH_GPIO_ERROR;
    }
    device->zmq_sock = zmq_socket(device->zmq_ctx, ZMQ_PAIR);
    if (!device->zmq_sock) {
        simulith_log("simulith_gpio_init: Failed to create ZMQ socket\n");
        zmq_ctx_term(device->zmq_ctx);
        return SIMULITH_GPIO_ERROR;
    }
    if (strlen(device->name) > 0) {
        zmq_setsockopt(device->zmq_sock, ZMQ_IDENTITY, device->name, strlen(device->name));
    }
    int rc;
    if (device->is_server) {
        rc = zmq_bind(device->zmq_sock, device->address);
        if (rc != 0) {
            simulith_log("simulith_gpio_init: Failed to bind to %s\n", device->address);
            zmq_close(device->zmq_sock);
            zmq_ctx_term(device->zmq_ctx);
            return SIMULITH_GPIO_ERROR;
        }
        simulith_log("simulith_gpio_init: Bound to %s as '%s'\n", device->address, device->name);
    } else {
        rc = zmq_connect(device->zmq_sock, device->address);
        if (rc != 0) {
            simulith_log("simulith_gpio_init: Failed to connect to %s\n", device->address);
            zmq_close(device->zmq_sock);
            zmq_ctx_term(device->zmq_ctx);
            return SIMULITH_GPIO_ERROR;
        }
        simulith_log("simulith_gpio_init: Connected to %s as '%s'\n", device->address, device->name);
    }
    device->init = SIMULITH_GPIO_INITIALIZED;
    return SIMULITH_GPIO_SUCCESS;
}

int simulith_gpio_write(gpio_device_t *device, uint8_t value)
{
    if (!device || device->init != SIMULITH_GPIO_INITIALIZED) {
        simulith_log("simulith_gpio_write: Uninitialized GPIO device\n");
        return SIMULITH_GPIO_ERROR;
    }
    
    if (value > 1) {
        simulith_log("simulith_gpio_write: Invalid value %d (must be 0 or 1)\n", value);
        return SIMULITH_GPIO_ERROR;
    }

    gpio_message_t msg = {.command = 1, .value = value};
    int rc = zmq_send(device->zmq_sock, &msg, sizeof(msg), ZMQ_DONTWAIT);
    if (rc < 0) {
        simulith_log("simulith_gpio_write: zmq_send failed (peer may be unavailable)\n");
        return SIMULITH_GPIO_ERROR;
    }
    simulith_log("GPIO TX[%s]: pin=%d, value=%d\n", device->name, device->pin, value);
    return SIMULITH_GPIO_SUCCESS;
}

int simulith_gpio_read(gpio_device_t *device, uint8_t *value)
{
    if (!device || device->init != SIMULITH_GPIO_INITIALIZED || !value) {
        simulith_log("simulith_gpio_read: Invalid parameters\n");
        return SIMULITH_GPIO_ERROR;
    }

    gpio_message_t msg = {.command = 0, .value = 0};
    int rc = zmq_send(device->zmq_sock, &msg, sizeof(msg), ZMQ_DONTWAIT);
    if (rc < 0) {
        simulith_log("simulith_gpio_read: zmq_send failed (peer may be unavailable)\n");
        return SIMULITH_GPIO_ERROR;
    }

    gpio_message_t response;
    rc = zmq_recv(device->zmq_sock, &response, sizeof(response), ZMQ_DONTWAIT);
    if (rc < 0) {
        if (zmq_errno() == EAGAIN) {
            // No response available, return default based on direction
            *value = 0; // Default to low
            return SIMULITH_GPIO_SUCCESS;
        }
        simulith_log("simulith_gpio_read: zmq_recv failed (peer may be unavailable)\n");
        return SIMULITH_GPIO_ERROR;
    }
    
    *value = response.value;
    simulith_log("GPIO RX[%s]: pin=%d, value=%d\n", device->name, device->pin, *value);
    return SIMULITH_GPIO_SUCCESS;
}

int simulith_gpio_toggle(gpio_device_t *device)
{
    if (!device || device->init != SIMULITH_GPIO_INITIALIZED) {
        simulith_log("simulith_gpio_toggle: Uninitialized GPIO device\n");
        return SIMULITH_GPIO_ERROR;
    }

    gpio_message_t msg = {.command = 2, .value = 0};
    int rc = zmq_send(device->zmq_sock, &msg, sizeof(msg), ZMQ_DONTWAIT);
    if (rc < 0) {
        simulith_log("simulith_gpio_toggle: zmq_send failed (peer may be unavailable)\n");
        return SIMULITH_GPIO_ERROR;
    }
    simulith_log("GPIO Toggle[%s]: pin=%d\n", device->name, device->pin);
    return SIMULITH_GPIO_SUCCESS;
}

int simulith_gpio_close(gpio_device_t *device)
{
    if (!device || device->init != SIMULITH_GPIO_INITIALIZED) {
        return SIMULITH_GPIO_ERROR;
    }
    zmq_close(device->zmq_sock);
    zmq_ctx_term(device->zmq_ctx);
    device->init = 0;
    simulith_log("GPIO device %s closed\n", device->name);
    return SIMULITH_GPIO_SUCCESS;
}