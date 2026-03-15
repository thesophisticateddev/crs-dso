#pragma once
#include <cstdint>

struct DeviceSettings {
    // Channel 1
    bool     ch1Enabled = true;
    uint8_t  ch1VoltageRange = 0;   // Range index
    uint8_t  ch1Coupling = 1;       // 0=AC, 1=DC, 2=GND
    int16_t  ch1Offset = 0;         // mV

    // Channel 2
    bool     ch2Enabled = false;
    uint8_t  ch2VoltageRange = 0;
    uint8_t  ch2Coupling = 1;
    int16_t  ch2Offset = 0;

    // Timebase
    uint32_t sampleRate = 1000000;   // Samples per second
    uint32_t timebaseNsPerDiv = 1000; // ns per division

    // Trigger
    uint8_t  triggerSource = 0;      // 0=CH1, 1=CH2
    uint8_t  triggerEdge = 0;        // 0=Rising, 1=Falling
    int16_t  triggerLevel = 0;       // mV

    // Acquisition mode
    uint8_t  acquisitionMode = 0;    // 0=Auto, 1=Normal, 2=Single
};
