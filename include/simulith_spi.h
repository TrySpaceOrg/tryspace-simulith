#ifndef SIMULITH_SPI_H
#define SIMULITH_SPI_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief SPI configuration structure
     */
    typedef struct
    {
        uint32_t clock_hz;    /**< Clock frequency in Hz */
        uint8_t  mode;        /**< SPI mode (0-3) */
        uint8_t  bit_order;   /**< Bit order (MSB/LSB first) */
        uint8_t  cs_polarity; /**< Chip select polarity (active high/low) */
        uint8_t  data_bits;   /**< Data bits per transfer (4-16) */
    } simulith_spi_config_t;

    /**
     * @brief Callback function type for SPI transfer operations
     * @param bus_id Bus identifier
     * @param cs_id Chip select identifier
     * @param tx_data Data to transmit (can be NULL for receive-only)
     * @param rx_data Buffer for received data (can be NULL for transmit-only)
     * @param len Number of bytes to transfer
     * @return Number of bytes transferred, -1 on error
     */
    typedef int (*simulith_spi_transfer_callback)(uint8_t bus_id, uint8_t cs_id, const uint8_t *tx_data,
                                                  uint8_t *rx_data, size_t len);

    /**
     * @brief Initialize an SPI bus
     * @param bus_id Bus identifier (0-7)
     * @param config SPI configuration structure
     * @param transfer_cb Callback function for transfer operations
     * @return 0 on success, -1 on failure
     */
    int simulith_spi_init(uint8_t bus_id, const simulith_spi_config_t *config,
                          simulith_spi_transfer_callback transfer_cb);

    /**
     * @brief Perform an SPI transfer
     * @param bus_id Bus identifier
     * @param cs_id Chip select identifier (0-7)
     * @param tx_data Data to transmit (can be NULL for receive-only)
     * @param rx_data Buffer for received data (can be NULL for transmit-only)
     * @param len Number of bytes to transfer
     * @return Number of bytes transferred, -1 on failure
     */
    int simulith_spi_transfer(uint8_t bus_id, uint8_t cs_id, const uint8_t *tx_data, uint8_t *rx_data, size_t len);

    /**
     * @brief Close an SPI bus
     * @param bus_id Bus identifier
     * @return 0 on success, -1 on failure
     */
    int simulith_spi_close(uint8_t bus_id);

// Constants for SPI configuration
#define SIMULITH_SPI_MODE_0 0 /**< CPOL=0, CPHA=0 */
#define SIMULITH_SPI_MODE_1 1 /**< CPOL=0, CPHA=1 */
#define SIMULITH_SPI_MODE_2 2 /**< CPOL=1, CPHA=0 */
#define SIMULITH_SPI_MODE_3 3 /**< CPOL=1, CPHA=1 */

#define SIMULITH_SPI_MSB_FIRST 0
#define SIMULITH_SPI_LSB_FIRST 1

#define SIMULITH_SPI_CS_ACTIVE_LOW  0
#define SIMULITH_SPI_CS_ACTIVE_HIGH 1

#ifdef __cplusplus
}
#endif

#endif /* SIMULITH_SPI_H */