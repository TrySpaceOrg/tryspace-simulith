#include "simulith_spi.h"
#include "simulith.h"
#include <string.h>

#define MAX_SPI_BUSES 8
#define MAX_DATA_BITS 16

typedef struct
{
    bool                           initialized;
    simulith_spi_config_t          config;
    simulith_spi_transfer_callback transfer_callback;
} spi_bus_t;

static spi_bus_t spi_buses[MAX_SPI_BUSES] = {0};

static bool is_valid_config(const simulith_spi_config_t *config)
{
    if (!config)
        return false;

    // Validate clock frequency (1kHz to 100MHz)
    if (config->clock_hz < 1000 || config->clock_hz > 100000000)
        return false;

    // Validate SPI mode (0-3)
    if (config->mode > SIMULITH_SPI_MODE_3)
        return false;

    // Validate bit order
    if (config->bit_order > SIMULITH_SPI_LSB_FIRST)
        return false;

    // Validate CS polarity
    if (config->cs_polarity > SIMULITH_SPI_CS_ACTIVE_HIGH)
        return false;

    // Validate data bits (4-16)
    if (config->data_bits < 4 || config->data_bits > MAX_DATA_BITS)
        return false;

    return true;
}

int simulith_spi_init(uint8_t bus_id, const simulith_spi_config_t *config, simulith_spi_transfer_callback transfer_cb)
{
    if (bus_id >= MAX_SPI_BUSES)
    {
        simulith_log("Invalid SPI bus ID: %d\n", bus_id);
        return -1;
    }

    if (!is_valid_config(config))
    {
        simulith_log("Invalid SPI configuration for bus %d\n", bus_id);
        return -1;
    }

    if (!transfer_cb)
    {
        simulith_log("Transfer callback cannot be NULL\n");
        return -1;
    }

    spi_bus_t *bus = &spi_buses[bus_id];

    if (bus->initialized)
    {
        simulith_log("SPI bus %d already initialized\n", bus_id);
        return -1;
    }

    // Initialize bus structure
    memcpy(&bus->config, config, sizeof(simulith_spi_config_t));
    bus->transfer_callback = transfer_cb;
    bus->initialized       = true;

    simulith_log("SPI bus %d initialized: %lu Hz, mode %d, %d bits %s first\n", bus_id, (unsigned long)config->clock_hz,
                 config->mode, config->data_bits, config->bit_order == SIMULITH_SPI_MSB_FIRST ? "MSB" : "LSB");

    return 0;
}

int simulith_spi_transfer(uint8_t bus_id, uint8_t cs_id, const uint8_t *tx_data, uint8_t *rx_data, size_t len)
{
    if (bus_id >= MAX_SPI_BUSES || !spi_buses[bus_id].initialized)
    {
        return -1;
    }

    if (cs_id >= MAX_SPI_BUSES)
    {
        simulith_log("Invalid CS ID: %d\n", cs_id);
        return -1;
    }

    if (!tx_data && !rx_data)
    {
        simulith_log("At least one of tx_data or rx_data must be non-NULL\n");
        return -1;
    }

    if (len == 0)
    {
        return 0;
    }

    spi_bus_t *bus = &spi_buses[bus_id];

    // Log transfer details
    simulith_log("SPI%d.CS%d transfer: ", bus_id, cs_id);
    if (tx_data)
    {
        simulith_log("TX[");
        for (size_t i = 0; i < len; i++)
        {
            simulith_log("%02X%s", tx_data[i], i < len - 1 ? " " : "");
        }
        simulith_log("] ");
    }

    // Perform transfer through callback
    int result = bus->transfer_callback(bus_id, cs_id, tx_data, rx_data, len);

    if (result > 0 && rx_data)
    {
        simulith_log("RX[");
        for (size_t i = 0; i < (size_t) result; i++)
        {
            simulith_log("%02X%s", rx_data[i], i < (size_t) result - 1 ? " " : "");
        }
        simulith_log("]");
    }
    simulith_log("\n");

    return result;
}

int simulith_spi_close(uint8_t bus_id)
{
    if (bus_id >= MAX_SPI_BUSES || !spi_buses[bus_id].initialized)
    {
        return -1;
    }

    spi_buses[bus_id].initialized = false;
    simulith_log("SPI bus %d closed\n", bus_id);
    return 0;
}