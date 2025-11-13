// Minimal ExpressLRS/CRSF over UART helper
// Portable C99 library to parse CRSF frames from a UART byte stream and
// send basic command/telemetry frames back to the receiver.

#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// CRSF default baud (ELRS typically negotiates too, but 420000 works broadly)
#ifndef ELRS_CRSF_BAUD_DEFAULT
#define ELRS_CRSF_BAUD_DEFAULT 420000u
#endif

#define ELRS_CRSF_FRAME_MAX 64u

// Common CRSF frame types used by ELRS
typedef enum {
    ELRS_CRSF_FRAMETYPE_LINK_STATISTICS          = 0x14,
    ELRS_CRSF_FRAMETYPE_RC_CHANNELS_PACKED       = 0x16,
    ELRS_CRSF_FRAMETYPE_SUBSET_RC_CHANNELS_PACKED= 0x17,
    ELRS_CRSF_FRAMETYPE_DEVICE_PING              = 0x28,
    ELRS_CRSF_FRAMETYPE_DEVICE_INFO              = 0x29,
    ELRS_CRSF_FRAMETYPE_COMMAND                  = 0x32,
    ELRS_CRSF_FRAMETYPE_MSP_REQ                  = 0x7A,
    ELRS_CRSF_FRAMETYPE_MSP_RESP                 = 0x7B,
    ELRS_CRSF_FRAMETYPE_MSP_WRITE                = 0x7C,
    ELRS_CRSF_FRAMETYPE_DISPLAYPORT_CMD          = 0x7D,
} elrs_crsf_frame_type_e;

// CRSF addresses (subset)
typedef enum {
    ELRS_CRSF_ADDRESS_FLIGHT_CONTROLLER = 0xC8,
    ELRS_CRSF_ADDRESS_CRSF_RECEIVER     = 0xEC,
} elrs_crsf_address_e;

typedef struct {
    // From LinkStatistics (0x14)
    uint8_t uplink_rssi1;      // dBm as positive value to convert: RSSI dBm = -uplink_rssiX
    uint8_t uplink_rssi2;      // dBm
    uint8_t uplink_lq;         // 0..100
    int8_t  uplink_snr;        // dB
    uint8_t active_antenna;    // 0/1
    uint8_t rf_mode;           // enum value from TX
    uint8_t uplink_tx_power;   // index mapping to mW
    uint8_t downlink_rssi;     // dBm as positive value
    uint8_t downlink_lq;       // 0..100
    int8_t  downlink_snr;      // dB
} elrs_crsf_link_stats_t;

typedef struct elrs_crsf_s elrs_crsf_t;

// Callbacks and HAL
typedef void (*elrs_crsf_on_rc_channels_cb)(elrs_crsf_t *ctx, const uint16_t *ch, uint8_t count, uint32_t timestamp_us);
typedef void (*elrs_crsf_on_link_stats_cb)(elrs_crsf_t *ctx, const elrs_crsf_link_stats_t *stats, uint32_t timestamp_us);
typedef void (*elrs_crsf_on_frame_cb)(elrs_crsf_t *ctx, uint8_t address, uint8_t type, const uint8_t *payload, uint8_t payload_len, uint32_t timestamp_us);

typedef uint32_t (*elrs_crsf_now_us_cb)(void *user);
typedef void (*elrs_crsf_tx_write_cb)(void *user, const uint8_t *data, uint16_t len);

typedef struct {
    // HAL
    elrs_crsf_now_us_cb now_us;      // required
    elrs_crsf_tx_write_cb tx_write;  // required
    void *user;                      // user pointer passed back to HAL

    // Callbacks (optional but recommended)
    elrs_crsf_on_rc_channels_cb on_rc_channels; // 0x16
    elrs_crsf_on_link_stats_cb on_link_stats;   // 0x14
    elrs_crsf_on_frame_cb on_frame;             // any other frame

    // Parser timing
    uint32_t frame_timeout_us; // default 1750us if 0
} elrs_crsf_config_t;

struct elrs_crsf_s {
    elrs_crsf_config_t cfg;
    // parser state
    uint8_t buf[ELRS_CRSF_FRAME_MAX];
    uint8_t pos;               // bytes written
    uint32_t frame_start_us;   // timestamp of first byte
};

// Init/reset parser with config
void elrs_crsf_init(elrs_crsf_t *ctx, const elrs_crsf_config_t *cfg);

// Feed one byte from UART RX ISR or polling
void elrs_crsf_input_byte(elrs_crsf_t *ctx, uint8_t byte);

// Send a generic CRSF frame (helper computes CRC8 DVB-S2)
// address: first byte (usually destination; FC uses 0xC8)
void elrs_crsf_send_frame(elrs_crsf_t *ctx, uint8_t address, uint8_t type, const uint8_t *payload, uint8_t payload_len);

// Send a RX bind command frame (crafts extended header with dest/origin and both CRCs)
void elrs_crsf_send_bind(elrs_crsf_t *ctx);

#ifdef __cplusplus
}
#endif

