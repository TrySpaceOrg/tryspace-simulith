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

#ifndef SIMULITH_GPIO_H
#define SIMULITH_GPIO_H

#include "simulith.h"

#define SIMULITH_GPIO_SUCCESS 0
#define SIMULITH_GPIO_ERROR  -1

#define SIMULITH_GPIO_INITIALIZED 255

#define SIMULITH_GPIO_BASE_PORT 9000

// GPIO direction constants for HWLIB compatibility
#define GPIO_INPUT  0
#define GPIO_OUTPUT 1

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
    uint8_t pin;
    uint8_t direction; // GPIO_INPUT or GPIO_OUTPUT for HWLIB compatibility
} gpio_device_t;

    /**
     * @brief Initialize a GPIO pin
     * @param device Pointer to GPIO device structure
     * @return SIMULITH_GPIO_SUCCESS on success, SIMULITH_GPIO_ERROR on failure
     */
    int simulith_gpio_init(gpio_device_t *device);

    /**
     * @brief Read GPIO pin value
     * @param device Pointer to GPIO device structure
     * @param value Pointer to store pin value
     * @return SIMULITH_GPIO_SUCCESS on success, SIMULITH_GPIO_ERROR on failure
     */
    int simulith_gpio_read(gpio_device_t *device, uint8_t *value);

    /**
     * @brief Write GPIO pin value
     * @param device Pointer to GPIO device structure
     * @param value Pin value to write (0 or 1)
     * @return SIMULITH_GPIO_SUCCESS on success, SIMULITH_GPIO_ERROR on failure
     */
    int simulith_gpio_write(gpio_device_t *device, uint8_t value);

    /**
     * @brief Toggle GPIO pin value
     * @param device Pointer to GPIO device structure
     * @return SIMULITH_GPIO_SUCCESS on success, SIMULITH_GPIO_ERROR on failure
     */
    int simulith_gpio_toggle(gpio_device_t *device);

    /**
     * @brief Close a GPIO pin
     * @param device Pointer to GPIO device structure
     * @return SIMULITH_GPIO_SUCCESS on success, SIMULITH_GPIO_ERROR on failure
     */
    int simulith_gpio_close(gpio_device_t *device);

#ifdef __cplusplus
}
#endif

#endif /* SIMULITH_GPIO_H */