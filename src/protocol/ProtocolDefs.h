#pragma once
#include <cstdint>

// Frame delimiters
constexpr uint8_t SYNC_BYTE_1 = 0xAA;
constexpr uint8_t SYNC_BYTE_2 = 0x55;
constexpr uint8_t END_MARKER  = 0x0D;

// Maximum payload size
constexpr uint16_t MAX_PAYLOAD_SIZE = 4096;

// Frame overhead: SYNC1 + SYNC2 + CMD + LEN_LO + LEN_HI + CRC_LO + CRC_HI + END
constexpr size_t FRAME_OVERHEAD = 8;

// --- Host → Device Commands ---
namespace Cmd {
    constexpr uint8_t IDENTIFY           = 0x01;
    constexpr uint8_t SET_SAMPLE_RATE    = 0x02;
    constexpr uint8_t SET_VOLTAGE_RANGE  = 0x03;
    constexpr uint8_t SET_TRIGGER        = 0x04;
    constexpr uint8_t SET_TIMEBASE       = 0x05;
    constexpr uint8_t SET_COUPLING       = 0x06;
    constexpr uint8_t START_ACQUISITION  = 0x07;
    constexpr uint8_t STOP_ACQUISITION   = 0x08;
    constexpr uint8_t FORCE_TRIGGER      = 0x09;
    constexpr uint8_t CALIBRATE          = 0x0A;
    constexpr uint8_t SET_CHANNEL_ENABLE = 0x0B;
    constexpr uint8_t SET_CHANNEL_OFFSET = 0x0C;
    constexpr uint8_t GET_DEVICE_STATUS  = 0x0D;

    // --- Device → Host Responses ---
    constexpr uint8_t IDENTIFY_RESP      = 0x80;
    constexpr uint8_t ACK                = 0x81;
    constexpr uint8_t NACK               = 0x82;
    constexpr uint8_t WAVEFORM_DATA      = 0x83;
    constexpr uint8_t TRIGGER_EVENT      = 0x84;
    constexpr uint8_t DEVICE_STATUS_RESP = 0x85;
    constexpr uint8_t ERROR_REPORT       = 0x86;
}

// Waveform data header (inside CMD 0x83 payload)
#pragma pack(push, 1)
struct WaveformHeader {
    uint8_t  channelMask;      // Bitmask: which channels are in this packet
    uint8_t  bitsPerSample;    // 8, 10, 12, or 16
    uint16_t numSamples;       // Per channel
    uint32_t sampleRateHz;     // Actual rate achieved
    uint32_t triggerIndex;     // Sample index of trigger point
    float    voltageScale;     // Volts per LSB
};
#pragma pack(pop)

static_assert(sizeof(WaveformHeader) == 16, "WaveformHeader must be 16 bytes");
