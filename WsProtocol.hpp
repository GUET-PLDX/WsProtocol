#pragma once

// clang-format off
/* === MODULE MANIFEST V2 ===
module_description: pldx_ws navigation command UART protocol receiver
constructor_args:
  - uart_name: "uart_ext_controller"
  - chassis_topic_name: "chassis_data"
  - task_stack_depth: 1024
  - thread_priority: LibXR::Thread::Priority::MEDIUM
template_args: []
required_hardware: uart_name
depends:
  - pldx/HostData
=== END MANIFEST === */
// clang-format on

#include <cstdint>

#include "HostData.hpp"
#include "WsProtocolParser.hpp"
#include "app_framework.hpp"
#include "libxr_def.hpp"
#include "semaphore.hpp"
#include "thread.hpp"
#include "uart.hpp"

class WsProtocol : public LibXR::Application {
 public:
  WsProtocol(LibXR::HardwareContainer& hw, LibXR::ApplicationManager& app,
             const char* uart_name, const char* chassis_topic_name,
             uint32_t task_stack_depth, LibXR::Thread::Priority thread_priority)
      : uart_(hw.template FindOrExit<LibXR::UART>({uart_name})),
        read_op_(read_sem_),
        chassis_topic_(LibXR::Topic::CreateTopic<HostData::HostChassisTarget>(
            chassis_topic_name)) {
    uart_->SetConfig({115200U, LibXR::UART::Parity::NO_PARITY, 8U, 1U});
    thread_.Create(this, ThreadFunc, "ws_protocol", task_stack_depth,
                   thread_priority);
    app.Register(*this);
  }

  void OnMonitor() override {}

 private:
  static void ThreadFunc(WsProtocol* self) { self->Run(); }

  void Run() {
    while (true) {
      uint8_t byte = 0U;
      if (uart_->Read({&byte, 1U}, read_op_) != LibXR::ErrorCode::OK) {
        continue;
      }

      WsProtocolParser::NavigationCommand command{};
      if (!parser_.Push(byte, command)) {
        continue;
      }

      HostData::HostChassisTarget chassis{command.vx, command.vy, command.wz};
      chassis_topic_.Publish(chassis);
    }
  }

  LibXR::UART* uart_;
  LibXR::Semaphore read_sem_;
  LibXR::ReadOperation read_op_;
  LibXR::Thread thread_;
  LibXR::Topic chassis_topic_;
  WsProtocolParser::Parser parser_;
};
