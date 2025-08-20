/*
 * Simulith I2C - Requirements
 *
 * Shall utilize ZMQ to communicate between nodes
 * Shall have functions to initialize, read, write, transaction (both write then read), and close
 * Shall communicate directly to the other end of the node
 * Shall not block on any function
 * Shall fail gracefully if peer is unavailable and return error codes (non-zero) instead of crashing.
 * Shall not rely on a server as each node will be initialized with its name and destination.
 */

#ifndef SIMULITH_I2C_H
#define SIMULITH_I2C_H

#include "simulith.h"

#define SIMULITH_I2C_SUCCESS 0
#define SIMULITH_I2C_ERROR  -1

#define SIMULITH_I2C_INITIALIZED 255

#define SIMULITH_I2C_BASE_PORT 7000

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
    uint8_t bus_id;
    uint8_t device_addr;
} i2c_device_t;

    /**
     * @brief Initialize an I2C device
     * @param device Pointer to I2C device structure
     * @return SIMULITH_I2C_SUCCESS on success, SIMULITH_I2C_ERROR on failure
     */
    int simulith_i2c_init(i2c_device_t *device);

    /**
     * @brief Read data from an I2C device
     * @param device Pointer to I2C device structure
     * @param data Buffer to store read data
     * @param len Number of bytes to read
     * @return Number of bytes read on success, SIMULITH_I2C_ERROR on failure
     */
    int simulith_i2c_read(i2c_device_t *device, uint8_t *data, size_t len);

    /**
     * @brief Write data to an I2C device
     * @param device Pointer to I2C device structure
     * @param data Data to write
     * @param len Number of bytes to write
     * @return Number of bytes written on success, SIMULITH_I2C_ERROR on failure
     */
    int simulith_i2c_write(i2c_device_t *device, const uint8_t *data, size_t len);

    /**
     * @brief Perform I2C transaction (write then read)
     * @param device Pointer to I2C device structure
     * @param tx_data Data to write
     * @param tx_len Number of bytes to write
     * @param rx_data Buffer to store read data
     * @param rx_len Number of bytes to read
     * @return SIMULITH_I2C_SUCCESS on success, SIMULITH_I2C_ERROR on failure
     */
    int simulith_i2c_transaction(i2c_device_t *device, const uint8_t *tx_data, size_t tx_len, uint8_t *rx_data, size_t rx_len);

    /**
     * @brief Close an I2C device
     * @param device Pointer to I2C device structure
     * @return SIMULITH_I2C_SUCCESS on success, SIMULITH_I2C_ERROR on failure
     */
    int simulith_i2c_close(i2c_device_t *device);

#ifdef __cplusplus
}
#endif

#endif /* SIMULITH_I2C_H */