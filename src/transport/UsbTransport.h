#pragma once
#include "ITransport.h"
#include <libusb-1.0/libusb.h>

class UsbTransport : public ITransport {
public:
    UsbTransport();
    ~UsbTransport() override;

    bool open(const DeviceInfo& device) override;
    void close() override;
    TransportState state() const override;

    int write(const uint8_t* data, size_t length,
              unsigned int timeoutMs = 1000) override;
    int read(uint8_t* buffer, size_t maxLength,
             unsigned int timeoutMs = 1000) override;

    std::vector<DeviceInfo> enumerate() override;
    std::string lastError() const override;

private:
    libusb_context*       m_ctx = nullptr;
    libusb_device_handle* m_handle = nullptr;
    TransportState        m_state = TransportState::Disconnected;
    std::string           m_lastError;

    // Endpoint addresses — set these to match your firmware
    static constexpr uint8_t EP_BULK_OUT = 0x02;  // Commands to device
    static constexpr uint8_t EP_BULK_IN  = 0x86;  // Data from device

    bool claimInterface();
};
