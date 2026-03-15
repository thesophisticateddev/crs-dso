#pragma once
#include <vector>
#include <cstdint>

struct WaveformBuffer {
    uint8_t  channelMask = 0;
    uint32_t sampleRate = 0;
    uint32_t triggerIndex = 0;
    float    voltageScale = 1.0f;      // V per LSB
    uint16_t bitsPerSample = 16;

    // Raw samples per channel (channel index -> sample vector)
    std::vector<std::vector<int16_t>> channels;

    // Convert raw sample to voltage
    float toVoltage(int16_t raw) const {
        return raw * voltageScale;
    }

    // Convenience: get voltage-converted data for a channel
    std::vector<float> voltageData(uint8_t ch) const {
        std::vector<float> out;
        if (ch < channels.size()) {
            out.reserve(channels[ch].size());
            for (auto s : channels[ch])
                out.push_back(toVoltage(s));
        }
        return out;
    }
};
