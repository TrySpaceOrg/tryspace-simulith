#include "simulith_can.h"
#include "simulith.h"
#include <string.h>

#define MAX_CAN_BUSES 8
#define MAX_FILTERS   16

typedef struct
{
    simulith_can_filter_t filter;
    bool                  active;
} filter_slot_t;

typedef struct
{
    bool                     initialized;
    simulith_can_config_t    config;
    simulith_can_rx_callback rx_callback;
    filter_slot_t            filters[MAX_FILTERS];
    simulith_can_message_t   rx_buffer[32]; // Simple circular buffer for received messages
    size_t                   rx_head;
    size_t                   rx_tail;
} can_bus_t;

static can_bus_t can_buses[MAX_CAN_BUSES] = {0};

static bool is_valid_config(const simulith_can_config_t *config)
{
    if (!config)
        return false;

    // Validate bitrate (125kbps to 1Mbps)
    if (config->bitrate < SIMULITH_CAN_BITRATE_125K || config->bitrate > SIMULITH_CAN_BITRATE_1M)
        return false;

    // Validate sample point (50-90%)
    if (config->sample_point < 50 || config->sample_point > 90)
        return false;

    // Validate sync jump width (1-4)
    if (config->sync_jump < 1 || config->sync_jump > 4)
        return false;

    return true;
}

static bool is_valid_message(const simulith_can_message_t *msg)
{
    if (!msg)
        return false;

    // Validate ID based on type
    if (msg->is_extended)
    {
        if (msg->id > SIMULITH_CAN_ID_EXT_MAX)
            return false;
    }
    else
    {
        if (msg->id > SIMULITH_CAN_ID_STD_MAX)
            return false;
    }

    // Validate DLC
    if (msg->dlc > SIMULITH_CAN_MAX_DLC)
        return false;

    return true;
}

static bool message_passes_filter(const simulith_can_message_t *msg, const simulith_can_filter_t *filter)
{
    if (msg->is_extended != filter->is_extended)
        return false;
    return ((msg->id & filter->mask) == (filter->id & filter->mask));
}

int simulith_can_init(uint8_t bus_id, const simulith_can_config_t *config, simulith_can_rx_callback rx_cb)
{
    if (bus_id >= MAX_CAN_BUSES)
    {
        simulith_log("Invalid CAN bus ID: %d\n", bus_id);
        return -1;
    }

    if (!is_valid_config(config))
    {
        simulith_log("Invalid CAN configuration for bus %d\n", bus_id);
        return -1;
    }

    can_bus_t *bus = &can_buses[bus_id];

    if (bus->initialized)
    {
        simulith_log("CAN bus %d already initialized\n", bus_id);
        return -1;
    }

    // Initialize bus structure
    memcpy(&bus->config, config, sizeof(simulith_can_config_t));
    bus->rx_callback = rx_cb;
    bus->initialized = true;
    bus->rx_head     = 0;
    bus->rx_tail     = 0;

    // Clear all filters
    memset(bus->filters, 0, sizeof(bus->filters));

    simulith_log("CAN bus %d initialized: %lu bps, sample point %d%%, SJW %d\n", bus_id, (unsigned long)config->bitrate,
                 config->sample_point, config->sync_jump);

    return 0;
}

int simulith_can_add_filter(uint8_t bus_id, const simulith_can_filter_t *filter)
{
    if (bus_id >= MAX_CAN_BUSES || !can_buses[bus_id].initialized)
    {
        return -1;
    }

    can_bus_t *bus = &can_buses[bus_id];

    // Find free filter slot
    for (int i = 0; i < MAX_FILTERS; i++)
    {
        if (!bus->filters[i].active)
        {
            memcpy(&bus->filters[i].filter, filter, sizeof(simulith_can_filter_t));
            bus->filters[i].active = true;
            simulith_log("Added filter %d to CAN%d: ID=0x%x, mask=0x%x, %s\n", i, bus_id, filter->id, filter->mask,
                         filter->is_extended ? "extended" : "standard");
            return i;
        }
    }

    simulith_log("No free filter slots on CAN%d\n", bus_id);
    return -1;
}

int simulith_can_remove_filter(uint8_t bus_id, int filter_id)
{
    if (bus_id >= MAX_CAN_BUSES || !can_buses[bus_id].initialized || filter_id < 0 || filter_id >= MAX_FILTERS)
    {
        return -1;
    }

    can_bus_t *bus = &can_buses[bus_id];

    if (!bus->filters[filter_id].active)
    {
        return -1;
    }

    bus->filters[filter_id].active = false;
    simulith_log("Removed filter %d from CAN%d\n", filter_id, bus_id);
    return 0;
}

int simulith_can_send(uint8_t bus_id, const simulith_can_message_t *msg)
{
    if (bus_id >= MAX_CAN_BUSES || !can_buses[bus_id].initialized)
    {
        return -1;
    }

    if (!is_valid_message(msg))
    {
        simulith_log("Invalid CAN message\n");
        return -1;
    }

    simulith_log("CAN%d TX: ID=0x%x [%d] %s%s", bus_id, msg->id, msg->dlc, msg->is_extended ? "EXT " : "STD ",
                 msg->is_rtr ? "RTR " : "");

    if (!msg->is_rtr)
    {
        for (int i = 0; i < msg->dlc; i++)
        {
            simulith_log("%02X ", msg->data[i]);
        }
    }
    simulith_log("\n");

    // In simulation, we can directly pass the message to the receive callback
    // In real hardware, this would go through the CAN transceiver
    can_bus_t *bus = &can_buses[bus_id];
    if (bus->rx_callback)
    {
        // Check if message passes any active filters
        bool passes_filter = false;
        for (int i = 0; i < MAX_FILTERS; i++)
        {
            if (bus->filters[i].active && message_passes_filter(msg, &bus->filters[i].filter))
            {
                passes_filter = true;
                break;
            }
        }

        if (passes_filter || bus->rx_callback)
        {
            // Store in receive buffer
            size_t next_head = (bus->rx_head + 1) % 32;
            if (next_head != bus->rx_tail)
            {
                memcpy(&bus->rx_buffer[bus->rx_head], msg, sizeof(simulith_can_message_t));
                bus->rx_head = next_head;
            }
        }
    }

    return 0;
}

int simulith_can_receive(uint8_t bus_id, simulith_can_message_t *msg)
{
    if (bus_id >= MAX_CAN_BUSES || !can_buses[bus_id].initialized || !msg)
    {
        return -1;
    }

    can_bus_t *bus = &can_buses[bus_id];

    if (bus->rx_head == bus->rx_tail)
    {
        return 0; // No messages available
    }

    // Get message from buffer
    memcpy(msg, &bus->rx_buffer[bus->rx_tail], sizeof(simulith_can_message_t));
    bus->rx_tail = (bus->rx_tail + 1) % 32;

    simulith_log("CAN%d RX: ID=0x%x [%d] %s%s", bus_id, msg->id, msg->dlc, msg->is_extended ? "EXT " : "STD ",
                 msg->is_rtr ? "RTR " : "");

    if (!msg->is_rtr)
    {
        for (int i = 0; i < msg->dlc; i++)
        {
            simulith_log("%02X ", msg->data[i]);
        }
    }
    simulith_log("\n");

    return 1;
}

int simulith_can_close(uint8_t bus_id)
{
    if (bus_id >= MAX_CAN_BUSES || !can_buses[bus_id].initialized)
    {
        return -1;
    }

    can_buses[bus_id].initialized = false;
    simulith_log("CAN bus %d closed\n", bus_id);
    return 0;
}