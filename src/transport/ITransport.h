#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <functional>

enum class TransportState {
    Disconnected,
    Connecting,
    Connected,
    Error
};

struct DeviceInfo {
    uint16_t vendorId;
    uint16_t productId;
    std::string serial;
    std::string manufacturer;
    std::string product;
};

class ITransport {
public:
    virtual ~ITransport() = default;

    virtual bool open(const DeviceInfo& device) = 0;
    virtual void close() = 0;
    virtual TransportState state() const = 0;

    // Synchronous I/O (used in acquisition thread)
    virtual int write(const uint8_t* data, size_t length,
                      unsigned int timeoutMs = 1000) = 0;
    virtual int read(uint8_t* buffer, size_t maxLength,
                     unsigned int timeoutMs = 1000) = 0;

    // Device enumeration
    virtual std::vector<DeviceInfo> enumerate() = 0;

    // Error reporting
    virtual std::string lastError() const = 0;
};
