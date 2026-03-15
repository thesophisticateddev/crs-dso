#pragma once
#include "ITransport.h"
#include <queue>
#include <mutex>

class MockTransport : public ITransport {
public:
    bool open(const DeviceInfo& device) override;
    void close() override;
    TransportState state() const override;

    int write(const uint8_t* data, size_t length,
              unsigned int timeoutMs = 1000) override;
    int read(uint8_t* buffer, size_t maxLength,
             unsigned int timeoutMs = 1000) override;

    std::vector<DeviceInfo> enumerate() override;
    std::string lastError() const override;

    // Test helpers
    void injectResponse(const std::vector<uint8_t>& data);
    std::vector<uint8_t> lastWritten() const;

private:
    TransportState m_state = TransportState::Disconnected;
    std::queue<std::vector<uint8_t>> m_responseQueue;
    std::vector<uint8_t> m_lastWrite;
    mutable std::mutex m_mutex;
};
