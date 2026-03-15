#include "ProtocolCodec.h"
#include "ProtocolDefs.h"
#include "Crc16.h"
#include <cstring>

std::vector<uint8_t> ProtocolCodec::encode(const ProtocolFrame& frame) {
    std::vector<uint8_t> out;
    uint16_t payloadLen = static_cast<uint16_t>(frame.payload.size());

    // Reserve space: SYNC1 + SYNC2 + CMD + LEN(2) + PAYLOAD + CRC(2) + END
    out.reserve(FRAME_OVERHEAD + payloadLen);

    // Sync bytes
    out.push_back(SYNC_BYTE_1);
    out.push_back(SYNC_BYTE_2);

    // Command ID
    out.push_back(frame.commandId);

    // Payload length (little-endian)
    out.push_back(static_cast<uint8_t>(payloadLen & 0xFF));
    out.push_back(static_cast<uint8_t>((payloadLen >> 8) & 0xFF));

    // Payload
    out.insert(out.end(), frame.payload.begin(), frame.payload.end());

    // CRC-16 CCITT over CMD + LEN + PAYLOAD
    // That's from index 2 to current end
    uint16_t crc = crc16Ccitt(out.data() + 2, out.size() - 2);
    out.push_back(static_cast<uint8_t>(crc & 0xFF));
    out.push_back(static_cast<uint8_t>((crc >> 8) & 0xFF));

    // End marker
    out.push_back(END_MARKER);

    return out;
}

void ProtocolCodec::feedBytes(const uint8_t* data, size_t length) {
    for (size_t i = 0; i < length; i++) {
        uint8_t byte = data[i];

        switch (m_state) {
        case ParseState::WaitSync1:
            if (byte == SYNC_BYTE_1)
                m_state = ParseState::WaitSync2;
            break;

        case ParseState::WaitSync2:
            if (byte == SYNC_BYTE_2) {
                m_state = ParseState::ReadCmd;
                m_current = ProtocolFrame{};
                m_expectedLen = 0;
                m_receivedCrc = 0;
                m_payloadIdx = 0;
            } else if (byte == SYNC_BYTE_1) {
                // Stay in WaitSync2 (consecutive 0xAA bytes)
            } else {
                m_state = ParseState::WaitSync1;
            }
            break;

        case ParseState::ReadCmd:
            m_current.commandId = byte;
            m_state = ParseState::ReadLenLow;
            break;

        case ParseState::ReadLenLow:
            m_expectedLen = byte;
            m_state = ParseState::ReadLenHigh;
            break;

        case ParseState::ReadLenHigh:
            m_expectedLen |= static_cast<uint16_t>(byte) << 8;
            if (m_expectedLen > MAX_PAYLOAD_SIZE) {
                // Invalid length, reset
                m_state = ParseState::WaitSync1;
            } else if (m_expectedLen == 0) {
                m_state = ParseState::ReadCrcLow;
            } else {
                m_current.payload.resize(m_expectedLen);
                m_payloadIdx = 0;
                m_state = ParseState::ReadPayload;
            }
            break;

        case ParseState::ReadPayload:
            m_current.payload[m_payloadIdx++] = byte;
            if (m_payloadIdx >= m_expectedLen)
                m_state = ParseState::ReadCrcLow;
            break;

        case ParseState::ReadCrcLow:
            m_receivedCrc = byte;
            m_state = ParseState::ReadCrcHigh;
            break;

        case ParseState::ReadCrcHigh: {
            m_receivedCrc |= static_cast<uint16_t>(byte) << 8;

            // Compute CRC over CMD + LEN + PAYLOAD
            std::vector<uint8_t> crcData;
            crcData.reserve(3 + m_expectedLen);
            crcData.push_back(m_current.commandId);
            crcData.push_back(static_cast<uint8_t>(m_expectedLen & 0xFF));
            crcData.push_back(static_cast<uint8_t>((m_expectedLen >> 8) & 0xFF));
            crcData.insert(crcData.end(), m_current.payload.begin(), m_current.payload.end());

            uint16_t computed = crc16Ccitt(crcData.data(), crcData.size());

            if (computed == m_receivedCrc) {
                m_state = ParseState::WaitEnd;
            } else {
                m_crcErrors++;
                m_state = ParseState::WaitSync1;
            }
            break;
        }

        case ParseState::WaitEnd:
            if (byte == END_MARKER) {
                m_completedFrames.push(std::move(m_current));
                m_framesDecoded++;
            }
            // Whether or not end marker matches, reset for next frame
            m_state = ParseState::WaitSync1;
            break;
        }
    }
}

std::optional<ProtocolFrame> ProtocolCodec::nextFrame() {
    if (m_completedFrames.empty())
        return std::nullopt;

    ProtocolFrame frame = std::move(m_completedFrames.front());
    m_completedFrames.pop();
    return frame;
}

void ProtocolCodec::reset() {
    m_state = ParseState::WaitSync1;
    m_current = ProtocolFrame{};
    m_expectedLen = 0;
    m_receivedCrc = 0;
    m_payloadIdx = 0;
    while (!m_completedFrames.empty())
        m_completedFrames.pop();
}
