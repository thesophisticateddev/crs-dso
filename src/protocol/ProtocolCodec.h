#pragma once
#include <cstdint>
#include <vector>
#include <optional>
#include <queue>

struct ProtocolFrame {
    uint8_t commandId;
    std::vector<uint8_t> payload;
};

class ProtocolCodec {
public:
    // Encoding: struct -> byte buffer ready for transport
    static std::vector<uint8_t> encode(const ProtocolFrame& frame);

    // Decoding: feed bytes incrementally, returns frame when complete
    void feedBytes(const uint8_t* data, size_t length);
    std::optional<ProtocolFrame> nextFrame();

    // Reset parser state (e.g., after error)
    void reset();

    // Statistics
    uint64_t framesDecoded() const { return m_framesDecoded; }
    uint64_t crcErrors() const { return m_crcErrors; }

private:
    enum class ParseState {
        WaitSync1, WaitSync2, ReadCmd, ReadLenLow, ReadLenHigh,
        ReadPayload, ReadCrcLow, ReadCrcHigh, WaitEnd
    };

    ParseState m_state = ParseState::WaitSync1;
    ProtocolFrame m_current;
    uint16_t m_expectedLen = 0;
    uint16_t m_receivedCrc = 0;
    uint16_t m_payloadIdx = 0;
    uint64_t m_framesDecoded = 0;
    uint64_t m_crcErrors = 0;

    std::queue<ProtocolFrame> m_completedFrames;
};
