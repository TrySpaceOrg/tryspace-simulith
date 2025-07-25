#ifndef SIMULITH_I2C_H
#define SIMULITH_I2C_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Callback function type for I2C read operations
     * @param addr Device address
     * @param reg Register address
     * @param data Buffer to store read data
     * @param len Number of bytes to read
     * @return 0 on success, -1 on failure
     */
    typedef int (*simulith_i2c_read_callback)(uint8_t addr, uint8_t reg, uint8_t *data, size_t len);

    /**
     * @brief Callback function type for I2C write operations
     * @param addr Device address
     * @param reg Register address
     * @param data Data to write
     * @param len Number of bytes to write
     * @return 0 on success, -1 on failure
     */
    typedef int (*simulith_i2c_write_callback)(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len);

    /**
     * @brief Initialize an I2C bus
     * @param bus_id Bus identifier (0-7)
     * @param read_cb Callback function for read operations
     * @param write_cb Callback function for write operations
     * @return 0 on success, -1 on failure
     */
    int simulith_i2c_init(uint8_t bus_id, simulith_i2c_read_callback read_cb, simulith_i2c_write_callback write_cb);

    /**
     * @brief Read data from an I2C device
     * @param bus_id Bus identifier
     * @param addr Device address
     * @param reg Register address
     * @param data Buffer to store read data
     * @param len Number of bytes to read
     * @return 0 on success, -1 on failure
     */
    int simulith_i2c_read(uint8_t bus_id, uint8_t addr, uint8_t reg, uint8_t *data, size_t len);

    /**
     * @brief Write data to an I2C device
     * @param bus_id Bus identifier
     * @param addr Device address
     * @param reg Register address
     * @param data Data to write
     * @param len Number of bytes to write
     * @return 0 on success, -1 on failure
     */
    int simulith_i2c_write(uint8_t bus_id, uint8_t addr, uint8_t reg, const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* SIMULITH_I2C_H */