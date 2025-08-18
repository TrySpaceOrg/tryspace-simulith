#include "simulith_pwm.h"
#include "simulith.h"
#include <string.h>

typedef struct
{
    bool                  initialized;
    bool                  running;
    simulith_pwm_config_t config;
    uint32_t              period_ns; // Period in nanoseconds
    uint32_t              duty_ns;   // Duty cycle in nanoseconds
} pwm_channel_t;

static pwm_channel_t pwm_channels[SIMULITH_PWM_MAX_CHANNELS] = {0};

static bool is_valid_channel(uint8_t channel)
{
    return channel < SIMULITH_PWM_MAX_CHANNELS;
}

static bool is_valid_frequency(uint32_t freq_hz)
{
    return (freq_hz >= SIMULITH_PWM_MIN_FREQ_HZ && freq_hz <= SIMULITH_PWM_MAX_FREQ_HZ);
}

static bool is_valid_duty_cycle(uint8_t duty_cycle)
{
    return duty_cycle <= 100;
}

static void update_timing(pwm_channel_t *channel)
{
    // Convert frequency to period in nanoseconds
    channel->period_ns = 1000000000UL / channel->config.frequency_hz;
    // Calculate duty cycle in nanoseconds
    channel->duty_ns = (channel->period_ns * channel->config.duty_cycle) / 100;
}

int simulith_pwm_init(uint8_t channel, const simulith_pwm_config_t *config)
{
    if (!is_valid_channel(channel))
    {
        simulith_log("Invalid PWM channel: %d\n", channel);
        return -1;
    }

    if (!config)
    {
        simulith_log("NULL PWM configuration\n");
        return -1;
    }

    if (!is_valid_frequency(config->frequency_hz))
    {
        simulith_log("Invalid PWM frequency: %lu Hz\n", config->frequency_hz);
        return -1;
    }

    if (!is_valid_duty_cycle(config->duty_cycle))
    {
        simulith_log("Invalid PWM duty cycle: %d%%\n", config->duty_cycle);
        return -1;
    }

    pwm_channel_t *pwm = &pwm_channels[channel];

    if (pwm->initialized)
    {
        simulith_log("PWM channel %d already initialized\n", channel);
        return -1;
    }

    // Store configuration
    memcpy(&pwm->config, config, sizeof(simulith_pwm_config_t));
    pwm->initialized = true;
    pwm->running     = false;

    // Calculate timing parameters
    update_timing(pwm);

    simulith_log("PWM channel %d initialized: %lu Hz, %d%% duty cycle\n", channel, config->frequency_hz,
                 config->duty_cycle);

    return 0;
}

int simulith_pwm_start(uint8_t channel)
{
    if (!is_valid_channel(channel))
    {
        return -1;
    }

    pwm_channel_t *pwm = &pwm_channels[channel];

    if (!pwm->initialized)
    {
        simulith_log("PWM channel %d not initialized\n", channel);
        return -1;
    }

    pwm->running = true;
    simulith_log("PWM channel %d started\n", channel);

    return 0;
}

int simulith_pwm_stop(uint8_t channel)
{
    if (!is_valid_channel(channel))
    {
        return -1;
    }

    pwm_channel_t *pwm = &pwm_channels[channel];

    if (!pwm->initialized)
    {
        simulith_log("PWM channel %d not initialized\n", channel);
        return -1;
    }

    pwm->running = false;
    simulith_log("PWM channel %d stopped\n", channel);

    return 0;
}

int simulith_pwm_set_duty(uint8_t channel, uint8_t duty_cycle)
{
    if (!is_valid_channel(channel))
    {
        return -1;
    }

    if (!is_valid_duty_cycle(duty_cycle))
    {
        simulith_log("Invalid PWM duty cycle: %d%%\n", duty_cycle);
        return -1;
    }

    pwm_channel_t *pwm = &pwm_channels[channel];

    if (!pwm->initialized)
    {
        simulith_log("PWM channel %d not initialized\n", channel);
        return -1;
    }

    pwm->config.duty_cycle = duty_cycle;
    update_timing(pwm);

    simulith_log("PWM channel %d duty cycle set to %d%%\n", channel, duty_cycle);

    return 0;
}

int simulith_pwm_set_frequency(uint8_t channel, uint32_t frequency_hz)
{
    if (!is_valid_channel(channel))
    {
        return -1;
    }

    if (!is_valid_frequency(frequency_hz))
    {
        simulith_log("Invalid PWM frequency: %lu Hz\n", frequency_hz);
        return -1;
    }

    pwm_channel_t *pwm = &pwm_channels[channel];

    if (!pwm->initialized)
    {
        simulith_log("PWM channel %d not initialized\n", channel);
        return -1;
    }

    pwm->config.frequency_hz = frequency_hz;
    update_timing(pwm);

    simulith_log("PWM channel %d frequency set to %lu Hz\n", channel, frequency_hz);

    return 0;
}

int simulith_pwm_close(uint8_t channel)
{
    if (!is_valid_channel(channel))
    {
        return -1;
    }

    pwm_channel_t *pwm = &pwm_channels[channel];

    if (!pwm->initialized)
    {
        simulith_log("PWM channel %d not initialized\n", channel);
        return -1;
    }

    // Stop the channel if it's running
    if (pwm->running)
    {
        simulith_pwm_stop(channel);
    }

    pwm->initialized = false;
    simulith_log("PWM channel %d closed\n", channel);

    return 0;
}