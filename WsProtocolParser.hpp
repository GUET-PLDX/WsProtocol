#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "crc.hpp"

namespace WsProtocolParser {

constexpr uint8_t SOF = 0x5AU;
constexpr uint8_t ROBOT_COMMAND_ID = 0x01U;
constexpr size_t HEADER_SIZE = 4U;
constexpr uint8_t PAYLOAD_LENGTH = 60U;
constexpr size_t FRAME_SIZE = HEADER_SIZE + PAYLOAD_LENGTH + 2U;
constexpr size_t VX_OFFSET = 8U;
constexpr size_t VY_OFFSET = 12U;
constexpr size_t WZ_OFFSET = 16U;

struct NavigationCommand {
  float vx;
  float vy;
  float wz;
};

class Parser {
 public:
  bool Push(uint8_t byte, NavigationCommand& command) {
    switch (state_) {
      case State::WAITING_SOF:
        if (byte == SOF) {
          frame_[0] = byte;
          bytes_read_ = 1U;
          state_ = State::READING_HEADER;
        }
        return false;

      case State::READING_HEADER:
        frame_[bytes_read_++] = byte;
        if (bytes_read_ != HEADER_SIZE) {
          return false;
        }
        if (!IsHeaderValid()) {
          Restart(byte);
          return false;
        }
        state_ = State::READING_BODY;
        return false;

      case State::READING_BODY:
        frame_[bytes_read_++] = byte;
        if (bytes_read_ != FRAME_SIZE) {
          return false;
        }
        if (!IsFrameValid()) {
          Restart(byte);
          return false;
        }
        Decode(command);
        Reset();
        return true;
    }

    Reset();
    return false;
  }

  void Reset() {
    state_ = State::WAITING_SOF;
    bytes_read_ = 0U;
  }

 private:
  enum class State : uint8_t { WAITING_SOF, READING_HEADER, READING_BODY };

  bool IsHeaderValid() const {
    return frame_[1] == PAYLOAD_LENGTH && frame_[2] == ROBOT_COMMAND_ID &&
           LibXR::CRC8::Verify(frame_.data(), HEADER_SIZE);
  }

  bool IsFrameValid() const {
    return LibXR::CRC16::Verify(frame_.data(), FRAME_SIZE);
  }

  void Decode(NavigationCommand& command) const {
    std::memcpy(&command.vx, frame_.data() + VX_OFFSET, sizeof(command.vx));
    std::memcpy(&command.vy, frame_.data() + VY_OFFSET, sizeof(command.vy));
    std::memcpy(&command.wz, frame_.data() + WZ_OFFSET, sizeof(command.wz));
  }

  void Restart(uint8_t byte) {
    Reset();
    if (byte == SOF) {
      frame_[0] = SOF;
      bytes_read_ = 1U;
      state_ = State::READING_HEADER;
    }
  }

  State state_ = State::WAITING_SOF;
  std::array<uint8_t, FRAME_SIZE> frame_{};
  size_t bytes_read_ = 0U;
};

}  // namespace WsProtocolParser
