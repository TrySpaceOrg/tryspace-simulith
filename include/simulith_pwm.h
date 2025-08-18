#ifndef SIMULITH_PWM_H
#define SIMULITH_PWM_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief PWM channel configuration structure
     */
    typedef struct
    {
        uint32_t frequency_hz; /**< PWM frequency in Hz */
        uint8_t  duty_cycle;   /**< Initial duty cycle (0-100) */
    } simulith_pwm_config_t;

    /**
     * @brief Initialize a PWM channel
     * @param channel Channel number (0-15)
     * @param config Channel configuration
     * @return 0 on success, -1 on failure
     */
    int simulith_pwm_init(uint8_t channel, const simulith_pwm_config_t *config);

    /**
     * @brief Start PWM output on a channel
     * @param channel Channel number
     * @return 0 on success, -1 on failure
     */
    int simulith_pwm_start(uint8_t channel);

    /**
     * @brief Stop PWM output on a channel
     * @param channel Channel number
     * @return 0 on success, -1 on failure
     */
    int simulith_pwm_stop(uint8_t channel);

    /**
     * @brief Update PWM duty cycle
     * @param channel Channel number
     * @param duty_cycle New duty cycle (0-100)
     * @return 0 on success, -1 on failure
     */
    int simulith_pwm_set_duty(uint8_t channel, uint8_t duty_cycle);

    /**
     * @brief Update PWM frequency
     * @param channel Channel number
     * @param frequency_hz New frequency in Hz
     * @return 0 on success, -1 on failure
     */
    int simulith_pwm_set_frequency(uint8_t channel, uint32_t frequency_hz);

    /**
     * @brief Close a PWM channel
     * @param channel Channel number
     * @return 0 on success, -1 on failure
     */
    int simulith_pwm_close(uint8_t channel);

// Constants
#define SIMULITH_PWM_MAX_CHANNELS 16
#define SIMULITH_PWM_MIN_FREQ_HZ  1
#define SIMULITH_PWM_MAX_FREQ_HZ  1000000 // 1MHz max frequency

#ifdef __cplusplus
}
#endif

#endif /* SIMULITH_PWM_H */