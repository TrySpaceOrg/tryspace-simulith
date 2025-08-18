#include "simulith_gpio.h"
#include "simulith.h"
#include <string.h>

typedef struct
{
    bool                 initialized;
    simulith_gpio_mode_t mode;
    uint8_t              state; // Current pin state (0 or 1)
} gpio_pin_t;

typedef struct
{
    gpio_pin_t pins[SIMULITH_GPIO_MAX_PINS];
} gpio_port_t;

static gpio_port_t gpio_ports[SIMULITH_GPIO_MAX_PORTS] = {0};

static bool is_valid_pin_config(const simulith_gpio_config_t *config)
{
    if (!config)
        return false;

    // Validate mode
    if (config->mode > SIMULITH_GPIO_MODE_OUTPUT_OD)
        return false;

    // Validate initial state for outputs
    if ((config->mode == SIMULITH_GPIO_MODE_OUTPUT || config->mode == SIMULITH_GPIO_MODE_OUTPUT_OD) &&
        config->initial_state > 1)
        return false;

    return true;
}

static bool check_pin_initialized(uint8_t port, uint8_t pin)
{
    if (port >= SIMULITH_GPIO_MAX_PORTS || pin >= SIMULITH_GPIO_MAX_PINS)
    {
        return false;
    }
    return gpio_ports[port].pins[pin].initialized;
}

int simulith_gpio_init(uint8_t port, uint8_t pin, const simulith_gpio_config_t *config)
{
    if (port >= SIMULITH_GPIO_MAX_PORTS || pin >= SIMULITH_GPIO_MAX_PINS)
    {
        simulith_log("Invalid GPIO port/pin: %d.%d\n", port, pin);
        return -1;
    }

    if (!is_valid_pin_config(config))
    {
        simulith_log("Invalid GPIO configuration for pin %d.%d\n", port, pin);
        return -1;
    }

    gpio_pin_t *gpio_pin = &gpio_ports[port].pins[pin];

    if (gpio_pin->initialized)
    {
        simulith_log("GPIO pin %d.%d already initialized\n", port, pin);
        return -1;
    }

    // Initialize pin structure
    gpio_pin->mode        = config->mode;
    gpio_pin->initialized = true;

    // Set initial state
    if (config->mode == SIMULITH_GPIO_MODE_OUTPUT || config->mode == SIMULITH_GPIO_MODE_OUTPUT_OD)
    {
        gpio_pin->state = config->initial_state;
    }
    else
    {
        // For inputs, set initial state based on pull resistors
        if (config->mode == SIMULITH_GPIO_MODE_INPUT_PULLUP)
        {
            gpio_pin->state = 1;
        }
        else
        {
            gpio_pin->state = 0; // Default to low for floating or pulldown inputs
        }
    }

    simulith_log("GPIO %d.%d initialized: mode=%d, state=%d\n", port, pin, config->mode, gpio_pin->state);

    return 0;
}

int simulith_gpio_write(uint8_t port, uint8_t pin, uint8_t value)
{
    if (!check_pin_initialized(port, pin))
    {
        return -1;
    }

    gpio_pin_t *gpio_pin = &gpio_ports[port].pins[pin];

    // Check if pin is configured as output
    if (gpio_pin->mode != SIMULITH_GPIO_MODE_OUTPUT && gpio_pin->mode != SIMULITH_GPIO_MODE_OUTPUT_OD)
    {
        simulith_log("Cannot write to GPIO %d.%d: not configured as output\n", port, pin);
        return -1;
    }

    // Validate value
    if (value > 1)
    {
        simulith_log("Invalid GPIO value: %d\n", value);
        return -1;
    }

    gpio_pin->state = value;
    simulith_log("GPIO %d.%d set to %d\n", port, pin, value);

    return 0;
}

int simulith_gpio_read(uint8_t port, uint8_t pin, uint8_t *value)
{
    if (!check_pin_initialized(port, pin) || !value)
    {
        return -1;
    }

    gpio_pin_t *gpio_pin = &gpio_ports[port].pins[pin];
    *value               = gpio_pin->state;

    simulith_log("GPIO %d.%d read: %d\n", port, pin, *value);
    return 0;
}

int simulith_gpio_toggle(uint8_t port, uint8_t pin)
{
    if (!check_pin_initialized(port, pin))
    {
        return -1;
    }

    gpio_pin_t *gpio_pin = &gpio_ports[port].pins[pin];

    // Check if pin is configured as output
    if (gpio_pin->mode != SIMULITH_GPIO_MODE_OUTPUT && gpio_pin->mode != SIMULITH_GPIO_MODE_OUTPUT_OD)
    {
        simulith_log("Cannot toggle GPIO %d.%d: not configured as output\n", port, pin);
        return -1;
    }

    gpio_pin->state = !gpio_pin->state;
    simulith_log("GPIO %d.%d toggled to %d\n", port, pin, gpio_pin->state);

    return 0;
}

int simulith_gpio_close(uint8_t port, uint8_t pin)
{
    if (!check_pin_initialized(port, pin))
    {
        return -1;
    }

    gpio_ports[port].pins[pin].initialized = false;
    simulith_log("GPIO %d.%d closed\n", port, pin);
    return 0;
}