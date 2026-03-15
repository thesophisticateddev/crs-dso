#pragma once
#include <cstdint>
#include <cstddef>

// CRC-16 CCITT (polynomial 0x1021, initial value 0xFFFF)
uint16_t crc16Ccitt(const uint8_t* data, size_t length);
