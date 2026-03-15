#include "MockTransport.h"
#include <algorithm>
#include <cstring>

bool MockTransport::open(const DeviceInfo& /*device*/) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_state = TransportState::Connected;
    return true;
}

void MockTransport::close() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_state = TransportState::Disconnected;
    // Clear queued responses
    while (!m_responseQueue.empty())
        m_responseQueue.pop();
}

TransportState MockTransport::state() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_state;
}

int MockTransport::write(const uint8_t* data, size_t length, unsigned int /*timeoutMs*/) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_state != TransportState::Connected)
        return -1;

    m_lastWrite.assign(data, data + length);
    return static_cast<int>(length);
}

int MockTransport::read(uint8_t* buffer, size_t maxLength, unsigned int /*timeoutMs*/) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_state != TransportState::Connected)
        return -1;

    if (m_responseQueue.empty())
        return 0;

    const auto& front = m_responseQueue.front();
    size_t copyLen = std::min(maxLength, front.size());
    std::memcpy(buffer, front.data(), copyLen);
    m_responseQueue.pop();

    return static_cast<int>(copyLen);
}

std::vector<DeviceInfo> MockTransport::enumerate() {
    // Return a single mock device for demo/testing
    DeviceInfo mock;
    mock.vendorId = 0xFFFF;
    mock.productId = 0x0001;
    mock.serial = "MOCK-001";
    mock.manufacturer = "CRS Instruments (Mock)";
    mock.product = "CRS-DSO Mock Device";
    return {mock};
}

std::string MockTransport::lastError() const {
    return {};
}

void MockTransport::injectResponse(const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_responseQueue.push(data);
}

std::vector<uint8_t> MockTransport::lastWritten() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_lastWrite;
}
