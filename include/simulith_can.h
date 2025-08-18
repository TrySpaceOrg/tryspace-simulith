#ifndef SIMULITH_CAN_H
#define SIMULITH_CAN_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief CAN message structure
     */
    typedef struct
    {
        uint32_t id;          /**< Message ID (11-bit standard or 29-bit extended) */
        uint8_t  is_extended; /**< 0 for standard ID, 1 for extended ID */
        uint8_t  is_rtr;      /**< Remote Transmission Request */
        uint8_t  dlc;         /**< Data Length Code (0-8 bytes) */
        uint8_t  data[8];     /**< Message data */
    } simulith_can_message_t;

    /**
     * @brief CAN filter structure
     */
    typedef struct
    {
        uint32_t id;          /**< Filter ID */
        uint32_t mask;        /**< Filter mask */
        uint8_t  is_extended; /**< 0 for standard ID, 1 for extended ID */
    } simulith_can_filter_t;

    /**
     * @brief CAN configuration structure
     */
    typedef struct
    {
        uint32_t bitrate;      /**< Bitrate in bits/second */
        uint8_t  sample_point; /**< Sample point in percent (0-100) */
        uint8_t  sync_jump;    /**< Synchronization Jump Width (1-4) */
    } simulith_can_config_t;

    /**
     * @brief Callback function type for CAN receive operations
     * @param bus_id Bus identifier
     * @param msg Pointer to received message
     * @return 0 on success, -1 on failure
     */
    typedef int (*simulith_can_rx_callback)(uint8_t bus_id, const simulith_can_message_t *msg);

    /**
     * @brief Initialize a CAN bus
     * @param bus_id Bus identifier (0-7)
     * @param config CAN configuration structure
     * @param rx_cb Callback function for receive operations (NULL if not used)
     * @return 0 on success, -1 on failure
     */
    int simulith_can_init(uint8_t bus_id, const simulith_can_config_t *config, simulith_can_rx_callback rx_cb);

    /**
     * @brief Add a message filter
     * @param bus_id Bus identifier
     * @param filter Filter configuration
     * @return Filter ID on success (>= 0), -1 on failure
     */
    int simulith_can_add_filter(uint8_t bus_id, const simulith_can_filter_t *filter);

    /**
     * @brief Remove a message filter
     * @param bus_id Bus identifier
     * @param filter_id Filter identifier returned by simulith_can_add_filter
     * @return 0 on success, -1 on failure
     */
    int simulith_can_remove_filter(uint8_t bus_id, int filter_id);

    /**
     * @brief Send a CAN message
     * @param bus_id Bus identifier
     * @param msg Message to send
     * @return 0 on success, -1 on failure
     */
    int simulith_can_send(uint8_t bus_id, const simulith_can_message_t *msg);

    /**
     * @brief Receive a CAN message (non-blocking)
     * @param bus_id Bus identifier
     * @param msg Buffer to store received message
     * @return 1 if message received, 0 if no message available, -1 on failure
     */
    int simulith_can_receive(uint8_t bus_id, simulith_can_message_t *msg);

    /**
     * @brief Close a CAN bus
     * @param bus_id Bus identifier
     * @return 0 on success, -1 on failure
     */
    int simulith_can_close(uint8_t bus_id);

// Constants for CAN configuration
#define SIMULITH_CAN_BITRATE_125K 125000
#define SIMULITH_CAN_BITRATE_250K 250000
#define SIMULITH_CAN_BITRATE_500K 500000
#define SIMULITH_CAN_BITRATE_1M   1000000

#define SIMULITH_CAN_MAX_FILTERS 16
#define SIMULITH_CAN_MAX_DLC     8

// Macros for CAN ID types
#define SIMULITH_CAN_ID_STD_MAX 0x7FF      /**< Maximum 11-bit standard ID */
#define SIMULITH_CAN_ID_EXT_MAX 0x1FFFFFFF /**< Maximum 29-bit extended ID */

#ifdef __cplusplus
}
#endif

#endif /* SIMULITH_CAN_H */