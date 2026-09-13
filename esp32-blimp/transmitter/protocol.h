#pragma once
#include <stdint.h>

// =====================================================================
//  Shared control protocol for the ESP-NOW blimp link.
//
//  IMPORTANT: transmitter/protocol.h and receiver/protocol.h MUST be
//  byte-for-byte identical. If you change one, copy it to the other.
// =====================================================================

static const uint8_t BLIMP_PROTO_VERSION = 1;

// ESP-NOW WiFi channel used by BOTH boards. Must match on TX and RX.
// (1, 6, or 11 are the non-overlapping 2.4 GHz channels.)
static const uint8_t BLIMP_WIFI_CHANNEL = 1;

// Full-scale value for every control axis. Axes range -AXIS_FS..+AXIS_FS.
static const int16_t AXIS_FS = 1000;

// Control flags (packed into ControlPacket.flags).
enum {
  FLAG_ARMED = 1 << 0,   // motors are allowed to spin
};

// The packet sent ~50x/second from the transmitter to the blimp.
// Kept small and packed so it fits comfortably in one ESP-NOW frame.
typedef struct __attribute__((packed)) {
  uint8_t  version;   // = BLIMP_PROTO_VERSION (receiver rejects mismatches)
  uint8_t  flags;     // bit0: armed (see FLAG_ARMED)
  uint16_t seq;       // increments each packet (link-quality / loss debug)
  int16_t  forward;   // -AXIS_FS..+AXIS_FS   (+ = drive forward)
  int16_t  yaw;       // -AXIS_FS..+AXIS_FS   (+ = turn right)
  int16_t  vertical;  // -AXIS_FS..+AXIS_FS   (+ = ascend)
} ControlPacket;
