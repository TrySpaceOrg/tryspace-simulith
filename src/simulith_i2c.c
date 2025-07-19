#include "simulith.h"
#include <errno.h>

#define MAX_I2C_BUSES 8

typedef struct
{
    bool                        initialized;
    simulith_i2c_read_callback  read_cb;
    simulith_i2c_write_callback write_cb;
} i2c_bus_t;

static i2c_bus_t i2c_buses[MAX_I2C_BUSES] = {0};

int simulith_i2c_init(uint8_t bus_id, simulith_i2c_read_callback read_cb, simulith_i2c_write_callback write_cb)
{
    if (bus_id >= MAX_I2C_BUSES)
    {
        simulith_log("Invalid I2C bus ID: %d\n", bus_id);
        errno = EINVAL;
        return -1;
    }

    if (!read_cb || !write_cb)
    {
        simulith_log("Invalid I2C callbacks\n");
        errno = EINVAL;
        return -1;
    }

    i2c_buses[bus_id].initialized = true;
    i2c_buses[bus_id].read_cb     = read_cb;
    i2c_buses[bus_id].write_cb    = write_cb;

    simulith_log("I2C bus %d initialized\n", bus_id);
    return 0;
}

int simulith_i2c_read(uint8_t bus_id, uint8_t addr, uint8_t reg, uint8_t *data, size_t len)
{
    if (bus_id >= MAX_I2C_BUSES || !i2c_buses[bus_id].initialized)
    {
        simulith_log("Invalid or uninitialized I2C bus: %d\n", bus_id);
        errno = EINVAL;
        return -1;
    }

    if (!data || len == 0)
    {
        simulith_log("Invalid read parameters\n");
        errno = EINVAL;
        return -1;
    }

    return i2c_buses[bus_id].read_cb(addr, reg, data, len);
}

int simulith_i2c_write(uint8_t bus_id, uint8_t addr, uint8_t reg, const uint8_t *data, size_t len)
{
    if (bus_id >= MAX_I2C_BUSES || !i2c_buses[bus_id].initialized)
    {
        simulith_log("Invalid or uninitialized I2C bus: %d\n", bus_id);
        errno = EINVAL;
        return -1;
    }

    if (!data || len == 0)
    {
        simulith_log("Invalid write parameters\n");
        errno = EINVAL;
        return -1;
    }

    return i2c_buses[bus_id].write_cb(addr, reg, data, len);
}