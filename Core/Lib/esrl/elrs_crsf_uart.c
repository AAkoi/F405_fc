#include <string.h>
#include "elrs_crsf_uart.h"

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

// Defaults
#define FRAME_TIMEOUT_US_DEFAULT 1750u

// CRC8 DVB-S2 (poly 0xD5)
static uint8_t crc8_dvb_s2_update(uint8_t crc, uint8_t data)
{
    crc ^= data;
    for (uint8_t i = 0; i < 8; i++) {
        crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0xD5) : (uint8_t)(crc << 1);
    }
    return crc;
}

// CRC8 poly 0xBA (used by CRSF command subpayloads)
static uint8_t crc8_poly_0xBA_update(uint8_t crc, uint8_t data)
{
    crc ^= data;
    for (uint8_t i = 0; i < 8; i++) {
        crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0xBA) : (uint8_t)(crc << 1);
    }
    return crc;
}

static uint8_t crsf_compute_crc(uint8_t type, const uint8_t *payload, uint8_t payload_len)
{
    uint8_t crc = crc8_dvb_s2_update(0, type);
    for (uint8_t i = 0; i < payload_len; i++) {
        crc = crc8_dvb_s2_update(crc, payload[i]);
    }
    return crc;
}

static uint16_t bit_extract_11(const uint8_t *buf, uint16_t bitIndex)
{
    // Extract 11 bits starting at bitIndex from buffer
    uint16_t byteIndex = bitIndex >> 3;      // /8
    uint8_t bitOffset = bitIndex & 7;        // %8
    uint32_t val = (uint32_t)buf[byteIndex]
                 | ((uint32_t)buf[byteIndex + 1] << 8)
                 | ((uint32_t)buf[byteIndex + 2] << 16);
    val >>= bitOffset;
    return (uint16_t)(val & 0x7FFu);
}

static void handle_rc_channels(elrs_crsf_t *ctx, const uint8_t *payload, uint8_t payload_len, uint32_t now)
{
    (void)payload_len;
    if (!ctx->cfg.on_rc_channels) return;
    uint16_t ch[16];
    for (uint8_t i = 0; i < 16; i++) {
        ch[i] = bit_extract_11(payload, (uint16_t)(i * 11));
    }
    ctx->cfg.on_rc_channels(ctx, ch, 16, now);
}

static void handle_link_stats(elrs_crsf_t *ctx, const uint8_t *payload, uint8_t payload_len, uint32_t now)
{
    if (!ctx->cfg.on_link_stats) return;
    if (payload_len < 10) return;
    elrs_crsf_link_stats_t s;
    s.uplink_rssi1    = payload[0];
    s.uplink_rssi2    = payload[1];
    s.uplink_lq       = payload[2];
    s.uplink_snr      = (int8_t)payload[3];
    s.active_antenna  = payload[4];
    s.rf_mode         = payload[5];
    s.uplink_tx_power = payload[6];
    s.downlink_rssi   = payload[7];
    s.downlink_lq     = payload[8];
    s.downlink_snr    = (int8_t)payload[9];
    ctx->cfg.on_link_stats(ctx, &s, now);
}

void elrs_crsf_init(elrs_crsf_t *ctx, const elrs_crsf_config_t *cfg)
{
    memset(ctx, 0, sizeof(*ctx));
    if (cfg) ctx->cfg = *cfg;
    if (ctx->cfg.frame_timeout_us == 0) {
        ctx->cfg.frame_timeout_us = FRAME_TIMEOUT_US_DEFAULT;
    }
}

void elrs_crsf_input_byte(elrs_crsf_t *ctx, uint8_t byte)
{
    if (!ctx || !ctx->cfg.now_us) return;
    const uint32_t now = ctx->cfg.now_us(ctx->cfg.user);

    if (ctx->pos == 0) {
        ctx->frame_start_us = now;
    } else {
        // If inter-byte gap exceeds a full-frame time, reset and treat as new frame
        if ((now - ctx->frame_start_us) > ctx->cfg.frame_timeout_us) {
            ctx->pos = 0;
            ctx->frame_start_us = now;
        }
    }

    // Assume min 5 bytes until we get a valid length
    uint8_t full_len = (ctx->pos < 2)
        ? 5u
        : (uint8_t)MIN((uint16_t)(ctx->buf[1] + 2u), (uint16_t)ELRS_CRSF_FRAME_MAX);

    if (ctx->pos < full_len) {
        ctx->buf[ctx->pos++] = byte;
        if (ctx->pos >= full_len) {
            // Frame complete
            uint8_t address = ctx->buf[0];
            uint8_t frame_len = ctx->buf[1];
            uint8_t type = ctx->buf[2];
            uint8_t payload_len = (uint8_t)(frame_len - 2u); // excludes type+crc from payload_len? No: CRC calc uses (len-2)
            const uint8_t *payload = &ctx->buf[3];
            uint8_t crc_expected = ctx->buf[full_len - 1];
            uint8_t crc_calc = crsf_compute_crc(type, payload, (uint8_t)(frame_len - 2u));

            ctx->pos = 0; // ready for next frame

            if (crc_calc != crc_expected) {
                return; // drop
            }

            switch (type) {
            case ELRS_CRSF_FRAMETYPE_RC_CHANNELS_PACKED:
                handle_rc_channels(ctx, payload, payload_len, now);
                break;
            case ELRS_CRSF_FRAMETYPE_LINK_STATISTICS:
                handle_link_stats(ctx, payload, payload_len, now);
                break;
            default:
                if (ctx->cfg.on_frame) ctx->cfg.on_frame(ctx, address, type, payload, payload_len, now);
                break;
            }
        }
    } else {
        // overflow defensive reset
        ctx->pos = 0;
    }
}

void elrs_crsf_send_frame(elrs_crsf_t *ctx, uint8_t address, uint8_t type, const uint8_t *payload, uint8_t payload_len)
{
    if (!ctx || !ctx->cfg.tx_write) return;
    uint8_t out[ELRS_CRSF_FRAME_MAX];
    uint8_t len = (uint8_t)(payload_len + 2u); // type + payload + crc
    out[0] = address;
    out[1] = len;
    out[2] = type;
    if (payload_len && payload) {
        memcpy(&out[3], payload, payload_len);
    }
    uint8_t crc = crsf_compute_crc(type, &out[3], (uint8_t)(len - 2u));
    out[3 + payload_len] = crc;
    ctx->cfg.tx_write(ctx->cfg.user, out, (uint16_t)(3 + payload_len + 1));
}

void elrs_crsf_send_bind(elrs_crsf_t *ctx)
{
    if (!ctx || !ctx->cfg.tx_write) return;

    // Extended header command payload: <DEST><ORIGIN><SUBCMD_GROUP><SUBCMD> <CMD_CRC>
    uint8_t ext_payload[5];
    ext_payload[0] = (uint8_t)ELRS_CRSF_ADDRESS_CRSF_RECEIVER;     // DEST
    ext_payload[1] = (uint8_t)ELRS_CRSF_ADDRESS_FLIGHT_CONTROLLER; // ORIGIN
    ext_payload[2] = 0x10; // CRSF_COMMAND_SUBCMD_RX
    ext_payload[3] = 0x01; // CRSF_COMMAND_SUBCMD_RX_BIND
    // Command CRC over type + first 4 bytes of payload?
    // In CRSF v3, cmd CRC (poly 0xBA) is over: <type><payload[0..N-1]>
    uint8_t cmd_crc = crc8_poly_0xBA_update(0, (uint8_t)ELRS_CRSF_FRAMETYPE_COMMAND);
    for (int i = 0; i < 4; i++) cmd_crc = crc8_poly_0xBA_update(cmd_crc, ext_payload[i]);
    ext_payload[4] = cmd_crc;

    // Now send standard CRSF frame with DVB-S2 CRC
    elrs_crsf_send_frame(ctx, (uint8_t)ELRS_CRSF_ADDRESS_FLIGHT_CONTROLLER, (uint8_t)ELRS_CRSF_FRAMETYPE_COMMAND, ext_payload, sizeof(ext_payload));
}

