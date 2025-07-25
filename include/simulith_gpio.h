#ifndef SIMULITH_GPIO_H
#define SIMULITH_GPIO_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief GPIO pin modes
     */
    typedef enum
    {
        SIMULITH_GPIO_MODE_INPUT,          /**< Input floating */
        SIMULITH_GPIO_MODE_INPUT_PULLUP,   /**< Input with pull-up */
        SIMULITH_GPIO_MODE_INPUT_PULLDOWN, /**< Input with pull-down */
        SIMULITH_GPIO_MODE_OUTPUT,         /**< Push-pull output */
        SIMULITH_GPIO_MODE_OUTPUT_OD       /**< Open-drain output */
    } simulith_gpio_mode_t;

    /**
     * @brief GPIO pin configuration structure
     */
    typedef struct
    {
        simulith_gpio_mode_t mode;          /**< Pin mode */
        uint8_t              initial_state; /**< Initial pin state for outputs */
    } simulith_gpio_config_t;

    /**
     * @brief Initialize a GPIO pin
     * @param port Port identifier (0-7)
     * @param pin Pin number (0-31)
     * @param config Pin configuration
     * @return 0 on success, -1 on failure
     */
    int simulith_gpio_init(uint8_t port, uint8_t pin, const simulith_gpio_config_t *config);

    /**
     * @brief Set GPIO pin output value
     * @param port Port identifier
     * @param pin Pin number
     * @param value Pin value (0 or 1)
     * @return 0 on success, -1 on failure
     */
    int simulith_gpio_write(uint8_t port, uint8_t pin, uint8_t value);

    /**
     * @brief Read GPIO pin input value
     * @param port Port identifier
     * @param pin Pin number
     * @param value Pointer to store pin value
     * @return 0 on success, -1 on failure
     */
    int simulith_gpio_read(uint8_t port, uint8_t pin, uint8_t *value);

    /**
     * @brief Toggle GPIO pin output value
     * @param port Port identifier
     * @param pin Pin number
     * @return 0 on success, -1 on failure
     */
    int simulith_gpio_toggle(uint8_t port, uint8_t pin);

    /**
     * @brief Close a GPIO pin
     * @param port Port identifier
     * @param pin Pin number
     * @return 0 on success, -1 on failure
     */
    int simulith_gpio_close(uint8_t port, uint8_t pin);

// Constants
#define SIMULITH_GPIO_MAX_PORTS 8
#define SIMULITH_GPIO_MAX_PINS  32

#ifdef __cplusplus
}
#endif

#endif /* SIMULITH_GPIO_H */