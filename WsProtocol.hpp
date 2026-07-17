#pragma once

// clang-format off
/* === MODULE MANIFEST V2 ===
module_description: pldx_ws bidirectional UART protocol
constructor_args:
  - task_stack_depth_uart: 1024
  - uart: "uart_ext_controller"
  - baudrate: 115200
  - chassis_topic_name: "chassis_data"
  - referee_topic_name: "sentry_ref"
  - thread_priority_uart: LibXR::Thread::Priority::LOW
template_args: []
required_hardware: uart
depends:
  - pldx/HostData
=== END MANIFEST === */
// clang-format on

#include <cstddef>
#include <cstdint>

#include "HostData.hpp"
#include "Referee.hpp"
#include "app_framework.hpp"
#include "crc.hpp"
#include "libxr_def.hpp"
#include "message.hpp"
#include "mutex.hpp"
#include "semaphore.hpp"
#include "thread.hpp"
#include "timebase.hpp"
#include "timer.hpp"
#include "uart.hpp"

#ifdef DEBUG
#pragma push_macro("DEBUG")
#undef DEBUG
#define WSPROTOCOL_RESTORE_DEBUG
#endif

class WsProtocol : public LibXR::Application {
 public:
  static constexpr uint8_t SOF = 0x5AU;
  static constexpr size_t MAX_PAYLOAD_SIZE = 255U;
  static constexpr size_t CRC16_SIZE = sizeof(uint16_t);
  static constexpr size_t MAX_FRAME_SIZE = 4U + MAX_PAYLOAD_SIZE + CRC16_SIZE;
  static constexpr uint32_t RX_TIMEOUT_MS = 50U;
  static constexpr uint32_t TX_TIMEOUT_MS = 5000U;
  static constexpr uint32_t CHASSIS_COMMAND_TIMEOUT_MS = 50U;
  static constexpr uint32_t CHASSIS_WATCHDOG_CHECK_INTERVAL_MS = 1U;
  static constexpr uint32_t CHASSIS_ZERO_PUBLISH_INTERVAL_MS = 50U;
  static constexpr size_t DEBUG_PACKAGE_COUNT = 10U;
  static constexpr size_t DEBUG_PACKAGE_NAME_LENGTH = 10U;
  static constexpr uint16_t PROTOCOL_VERSION = 3U;

  enum class Status : uint8_t { OFFLINE = 0U, RUNNING };
  enum class RxCommandID : uint8_t { ROBOT_COMMAND = 0x01U };
  enum class TxCommandID : uint8_t {
    DEBUG = 0x01U,
    IMU = 0x02U,
    ROBOT_STATE_INFO = 0x03U,
    EVENT_DATA = 0x04U,
    PID_DEBUG = 0x05U,
    ALL_ROBOT_HP = 0x06U,
    GAME_STATUS = 0x07U,
    ROBOT_MOTION = 0x08U,
    GROUND_ROBOT_POSITION = 0x09U,
    RFID_STATUS = 0x0AU,
    ROBOT_STATUS = 0x0BU,
    JOINT_STATE = 0x0CU,
    BUFF = 0x0DU,
    GIMBAL_STATE = 0x0EU,
    PROTOCOL_STATUS = 0x0FU,
  };

  struct [[gnu::packed]] Header {
    uint8_t sof;
    uint8_t length;
    uint8_t id;
    uint8_t crc8;
  };
  struct [[gnu::packed]] SpeedVector {
    float vx;
    float vy;
    float wz;
  };
  struct [[gnu::packed]] ChassisCommand {
    float roll;
    float pitch;
    float yaw;
    float leg_length;
  };
  struct [[gnu::packed]] GimbalCommand {
    uint8_t control;
    float pitch;
    float yaw;
    float pitch_velocity;
    float yaw_velocity;
    float pitch_acceleration;
    float yaw_acceleration;
  };
  struct [[gnu::packed]] ShootCommand {
    uint8_t fire;
    uint8_t friction_wheels_on;
  };
  struct [[gnu::packed]] TrackingCommand {
    uint8_t tracking;
  };
  struct [[gnu::packed]] RobotCommandData {
    SpeedVector speed_vector;
    ChassisCommand chassis;
    GimbalCommand gimbal;
    ShootCommand shoot;
    TrackingCommand tracking;
  };
  struct [[gnu::packed]] RobotCommandPayload {
    uint32_t time_stamp;
    RobotCommandData data;
  };
  struct [[gnu::packed]] DebugPackage {
    uint8_t name[DEBUG_PACKAGE_NAME_LENGTH];
    uint8_t type;
    float data;
  };
  struct [[gnu::packed]] DebugData {
    DebugPackage packages[DEBUG_PACKAGE_COUNT];
  };
  struct [[gnu::packed]] DebugPayload {
    uint32_t time_stamp;
    DebugPackage packages[DEBUG_PACKAGE_COUNT];
  };
  struct [[gnu::packed]] ImuData {
    float yaw;
    float pitch;
    float roll;
    float yaw_velocity;
    float pitch_velocity;
    float roll_velocity;
  };
  struct [[gnu::packed]] ImuPayload {
    uint32_t time_stamp;
    ImuData data;
  };
  struct [[gnu::packed]] RobotPartType {
    uint16_t chassis : 3;
    uint16_t gimbal : 3;
    uint16_t shoot : 3;
    uint16_t arm : 3;
    uint16_t custom_controller : 3;
    uint16_t reserved : 1;
  };
  struct [[gnu::packed]] RobotPartState {
    uint8_t chassis : 1;
    uint8_t gimbal : 1;
    uint8_t shoot : 1;
    uint8_t arm : 1;
    uint8_t custom_controller : 1;
    uint8_t reserved : 3;
  };
  struct [[gnu::packed]] RobotStateInfoData {
    RobotPartType type;
    RobotPartState state;
  };
  struct [[gnu::packed]] RobotStateInfoPayload {
    uint32_t time_stamp;
    RobotStateInfoData data;
  };
  struct [[gnu::packed]] EventData {
    uint32_t event_bits;
  };
  struct [[gnu::packed]] EventPayload {
    uint32_t time_stamp;
    EventData data;
  };
  struct [[gnu::packed]] PidDebugData {
    float feedback;
    float reference;
    float output;
  };
  struct [[gnu::packed]] PidDebugPayload {
    uint32_t time_stamp;
    PidDebugData data;
  };
  struct [[gnu::packed]] AllRobotHpData {
    uint16_t ally_1_robot_hp;
    uint16_t ally_2_robot_hp;
    uint16_t ally_3_robot_hp;
    uint16_t ally_4_robot_hp;
    int16_t damage_difference;
    uint16_t ally_7_robot_hp;
    uint16_t ally_outpost_hp;
    uint16_t ally_base_hp;
    uint16_t enemy_outpost_hp;
    uint16_t enemy_base_hp;
  };
  struct [[gnu::packed]] AllRobotHpPayload {
    uint32_t time_stamp;
    AllRobotHpData data;
  };
  struct [[gnu::packed]] GameStatusData {
    uint8_t game_type;
    uint8_t game_progress;
    uint16_t stage_remaining_time;
  };
  struct [[gnu::packed]] GameStatusPayload {
    uint32_t time_stamp;
    GameStatusData data;
  };
  struct [[gnu::packed]] RobotMotionData {
    SpeedVector speed_vector;
  };
  struct [[gnu::packed]] RobotMotionPayload {
    uint32_t time_stamp;
    RobotMotionData data;
  };
  struct [[gnu::packed]] GroundRobotPositionData {
    float hero_x;
    float hero_y;
    float engineer_x;
    float engineer_y;
    float standard_3_x;
    float standard_3_y;
    float standard_4_x;
    float standard_4_y;
    float reserved_1;
    float reserved_2;
  };
  struct [[gnu::packed]] GroundRobotPositionPayload {
    uint32_t time_stamp;
    GroundRobotPositionData data;
  };
  struct [[gnu::packed]] RfidStatusData {
    uint32_t rfid_status;
    uint8_t rfid_status_2;
  };
  struct [[gnu::packed]] RfidStatusPayload {
    uint32_t time_stamp;
    RfidStatusData data;
  };
  struct [[gnu::packed]] RobotStatusData {
    uint16_t source_valid_mask;
    uint8_t robot_id;
    uint8_t robot_level;
    uint16_t current_hp;
    uint16_t maximum_hp;
    uint16_t shooter_barrel_cooling_value;
    uint16_t shooter_barrel_heat_limit;
    uint16_t chassis_power_limit;
    float bullet_speed_limit;
    uint8_t power_management_output;
    uint16_t shooter_17mm_1_barrel_heat;
    float robot_position_x;
    float robot_position_y;
    float robot_position_angle;
    uint8_t raw_damage_byte;
    uint16_t projectile_allowance_17mm;
    uint16_t projectile_allowance_42mm;
    uint16_t remaining_gold_coin;
    uint16_t projectile_allowance_fortress;
  };
  struct [[gnu::packed]] RobotStatusPayload {
    uint32_t time_stamp;
    RobotStatusData data;
  };
  struct [[gnu::packed]] JointStateData {
    float pitch;
    float yaw;
  };
  struct [[gnu::packed]] JointStatePayload {
    uint32_t time_stamp;
    JointStateData data;
  };
  struct [[gnu::packed]] BuffData {
    uint8_t recovery_buff;
    uint16_t cooling_buff;
    uint8_t defence_buff;
    uint8_t vulnerability_buff;
    uint16_t attack_buff;
    uint8_t remaining_energy;
  };
  struct [[gnu::packed]] BuffPayload {
    uint32_t time_stamp;
    BuffData data;
  };
  struct [[gnu::packed]] ProtocolStatusData {
    uint16_t custom_protocol_version;
    uint8_t referee_protocol_major;
    uint8_t referee_protocol_minor;
    uint8_t referee_protocol_patch;
    uint8_t referee_online;
    uint16_t supported_source_mask;
    uint16_t valid_source_mask;
    uint8_t status_sequence;
  };
  struct [[gnu::packed]] ProtocolStatusPayload {
    uint32_t time_stamp;
    ProtocolStatusData data;
  };

  static_assert(sizeof(AllRobotHpData) == 20,
                "AllRobotHpData V3 size mismatch");
  static_assert(sizeof(GameStatusData) == 4, "GameStatusData V3 size mismatch");
  static_assert(sizeof(RfidStatusData) == 5, "RfidStatusData V3 size mismatch");
  static_assert(sizeof(RobotStatusData) == 42,
                "RobotStatusData V3 size mismatch");
  static_assert(sizeof(BuffData) == 8, "BuffData V3 size mismatch");
  static_assert(sizeof(ProtocolStatusData) == 11,
                "ProtocolStatusData V3 size mismatch");
  struct [[gnu::packed]] GimbalStateData {
    uint8_t mode;
    float pitch;
    float yaw;
    float pitch_velocity;
    float yaw_velocity;
    float bullet_speed;
    uint16_t bullet_count;
  };
  struct [[gnu::packed]] GimbalStatePayload {
    uint32_t time_stamp;
    GimbalStateData data;
  };
  struct Data {
    Status status = Status::OFFLINE;
    RobotCommandPayload robot_command{};
  };

  WsProtocol(LibXR::HardwareContainer& hw, LibXR::ApplicationManager& app,
             uint32_t task_stack_depth_uart, const char* uart,
             uint32_t baudrate, const char* chassis_topic_name,
             const char* referee_topic_name,
             LibXR::Thread::Priority thread_priority_uart =
                 LibXR::Thread::Priority::LOW)
      : uart_(hw.template FindOrExit<LibXR::UART>({uart})),
        read_sem_(0),
        read_op_(read_sem_, RX_TIMEOUT_MS),
        write_sem_(0),
        write_op_(write_sem_, TX_TIMEOUT_MS),
        chassis_topic_(LibXR::Topic::CreateTopic<HostData::HostChassisTarget>(
            chassis_topic_name)) {
    UNUSED(app);
    UNUSED(thread_priority_uart);
    uart_->SetConfig({baudrate, LibXR::UART::Parity::NO_PARITY, 8U, 1U});
    startup_time_ms_ = NowMilliseconds();
    chassis_watchdog_ = LibXR::Timer::CreateTask(
        ChassisWatchdog, this, CHASSIS_WATCHDOG_CHECK_INTERVAL_MS);
    LibXR::Timer::Add(chassis_watchdog_);
    LibXR::Timer::Start(chassis_watchdog_);
    auto referee_topic = LibXR::Topic::Find(referee_topic_name, nullptr);
    ASSERT(referee_topic != nullptr);
    auto referee_callback = LibXR::Topic::Callback::Create(
        [](bool in_isr, WsProtocol* self,
           const Referee::RobotGameRefereePack& data) {
          UNUSED(in_isr);
          self->OnRefereeUpdate(data);
        },
        this);
    LibXR::Topic(referee_topic).RegisterCallback(referee_callback);
    thread_.Create(this, ThreadFunc, "WsProtocol", task_stack_depth_uart,
                   LibXR::Thread::Priority::MEDIUM);
  }

  LibXR::ErrorCode SendFrame(TxCommandID command_id, const void* payload,
                             uint16_t payload_len) {
    LibXR::Mutex::LockGuard lock(tx_mutex_);
    if (payload_len > MAX_PAYLOAD_SIZE ||
        (payload_len > 0U && payload == nullptr)) {
      return LibXR::ErrorCode::ARG_ERR;
    }
    Header header{SOF, static_cast<uint8_t>(payload_len),
                  static_cast<uint8_t>(command_id), 0U};
    header.crc8 = LibXR::CRC8::Calculate(&header, sizeof(Header) - 1U);
    size_t offset = 0U;
    LibXR::Memory::FastCopy(&tx_buffer_[offset], &header, sizeof(header));
    offset += sizeof(header);
    if (payload_len > 0U) {
      LibXR::Memory::FastCopy(&tx_buffer_[offset], payload, payload_len);
      offset += payload_len;
    }
    const uint16_t crc = LibXR::CRC16::Calculate(tx_buffer_, offset);
    tx_buffer_[offset++] = static_cast<uint8_t>(crc & 0x00FFU);
    tx_buffer_[offset++] = static_cast<uint8_t>((crc >> 8U) & 0x00FFU);
    return uart_->Write({tx_buffer_, offset}, write_op_);
  }
  template <typename PayloadType>
  LibXR::ErrorCode SendFrame(TxCommandID command_id,
                             const PayloadType& payload) {
    return sizeof(PayloadType) > MAX_PAYLOAD_SIZE
               ? LibXR::ErrorCode::ARG_ERR
               : SendFrame(command_id, &payload,
                           static_cast<uint16_t>(sizeof(PayloadType)));
  }
  LibXR::ErrorCode SendDebugData(const DebugData& data) {
    DebugPayload payload{};
    payload.time_stamp = NowMilliseconds();
    LibXR::Memory::FastCopy(payload.packages, data.packages,
                            sizeof(payload.packages));
    return SendFrame(TxCommandID::DEBUG, payload);
  }
  LibXR::ErrorCode SendImuData(const ImuData& data) {
    return SendTimestamped<ImuData, ImuPayload>(TxCommandID::IMU, data);
  }
  LibXR::ErrorCode SendRobotStateInfo(const RobotStateInfoData& data) {
    return SendTimestamped<RobotStateInfoData, RobotStateInfoPayload>(
        TxCommandID::ROBOT_STATE_INFO, data);
  }
  LibXR::ErrorCode SendEventData(const EventData& data) {
    return SendTimestamped<EventData, EventPayload>(TxCommandID::EVENT_DATA,
                                                    data);
  }
  LibXR::ErrorCode SendPidDebugData(const PidDebugData& data) {
    return SendTimestamped<PidDebugData, PidDebugPayload>(
        TxCommandID::PID_DEBUG, data);
  }
  LibXR::ErrorCode SendAllRobotHp(const AllRobotHpData& data) {
    return SendTimestamped<AllRobotHpData, AllRobotHpPayload>(
        TxCommandID::ALL_ROBOT_HP, data);
  }
  LibXR::ErrorCode SendGameStatus(const GameStatusData& data) {
    return SendTimestamped<GameStatusData, GameStatusPayload>(
        TxCommandID::GAME_STATUS, data);
  }
  LibXR::ErrorCode SendRobotMotion(const RobotMotionData& data) {
    return SendTimestamped<RobotMotionData, RobotMotionPayload>(
        TxCommandID::ROBOT_MOTION, data);
  }
  LibXR::ErrorCode SendGroundRobotPosition(
      const GroundRobotPositionData& data) {
    return SendTimestamped<GroundRobotPositionData, GroundRobotPositionPayload>(
        TxCommandID::GROUND_ROBOT_POSITION, data);
  }
  LibXR::ErrorCode SendRfidStatus(const RfidStatusData& data) {
    return SendTimestamped<RfidStatusData, RfidStatusPayload>(
        TxCommandID::RFID_STATUS, data);
  }
  LibXR::ErrorCode SendRobotStatus(const RobotStatusData& data) {
    return SendTimestamped<RobotStatusData, RobotStatusPayload>(
        TxCommandID::ROBOT_STATUS, data);
  }
  LibXR::ErrorCode SendJointState(const JointStateData& data) {
    return SendTimestamped<JointStateData, JointStatePayload>(
        TxCommandID::JOINT_STATE, data);
  }
  LibXR::ErrorCode SendBuff(const BuffData& data) {
    return SendTimestamped<BuffData, BuffPayload>(TxCommandID::BUFF, data);
  }
  LibXR::ErrorCode SendProtocolStatus(const ProtocolStatusData& data) {
    return SendTimestamped<ProtocolStatusData, ProtocolStatusPayload>(
        TxCommandID::PROTOCOL_STATUS, data);
  }
  LibXR::ErrorCode SendGimbalState(const GimbalStateData& data) {
    return SendTimestamped<GimbalStateData, GimbalStatePayload>(
        TxCommandID::GIMBAL_STATE, data);
  }
  void OnMonitor() override {}

 private:
  template <typename DataType, typename PayloadType>
  LibXR::ErrorCode SendTimestamped(TxCommandID id, const DataType& data) {
    PayloadType payload{};
    payload.time_stamp = NowMilliseconds();
    payload.data = data;
    return SendFrame(id, payload);
  }

  struct [[gnu::packed]] ReceiveFrame {
    Header header;
    uint8_t body[MAX_PAYLOAD_SIZE + CRC16_SIZE];
  };
  static void ThreadFunc(WsProtocol* self) { self->Run(); }
  static void ChassisWatchdog(WsProtocol* self) {
    self->CheckChassisCommandFreshness();
    self->CheckProtocolStatus();
  }

  void OnRefereeUpdate(const Referee::RobotGameRefereePack& data) {
    {
      LibXR::Mutex::LockGuard lock(referee_mutex_);
      referee_data_ = data;
      protocol_status_dirty_ = true;
    }

    switch (static_cast<Referee::CommandID>(data.source_command_id)) {
      case Referee::CommandID::REF_CMD_ID_GAME_STATUS: {
        GameStatusData output{data.game_status.game_type,
                              data.game_status.game_progress,
                              data.game_status.stage_remain_time};
        SendGameStatus(output);
        break;
      }
      case Referee::CommandID::REF_CMD_ID_GAME_ROBOT_HP: {
        AllRobotHpData output{};
        LibXR::Memory::FastCopy(&output, &data.robot_hp, sizeof(output));
        SendAllRobotHp(output);
        break;
      }
      case Referee::CommandID::REF_CMD_ID_FIELD_EVENTS: {
        EventData output{};
        LibXR::Memory::FastCopy(&output.event_bits, &data.field_event,
                                sizeof(output.event_bits));
        SendEventData(output);
        break;
      }
      case Referee::CommandID::REF_CMD_ID_RFID: {
        RfidStatusData output{data.rfid.rfid_status, data.rfid.rfid_status_2};
        SendRfidStatus(output);
        break;
      }
      case Referee::CommandID::REF_CMD_ID_ROBOT_BUFF: {
        BuffData output{};
        LibXR::Memory::FastCopy(&output, &data.robot_buff, sizeof(output));
        SendBuff(output);
        break;
      }
      case Referee::CommandID::REF_CMD_ID_ROBOT_STATUS:
      case Referee::CommandID::REF_CMD_ID_POWER_HEAT_DATA:
      case Referee::CommandID::REF_CMD_ID_ROBOT_POS:
      case Referee::CommandID::REF_CMD_ID_ROBOT_DMG:
      case Referee::CommandID::REF_CMD_ID_BULLET_REMAINING:
        SendRobotStatus(BuildRobotStatus(data));
        break;
      default:
        break;
    }
  }

  static RobotStatusData BuildRobotStatus(
      const Referee::RobotGameRefereePack& data) {
    RobotStatusData output{};
    output.source_valid_mask = data.source_valid_mask;
    output.robot_id = data.robot_status.robot_id;
    output.robot_level = data.robot_status.robot_level;
    output.current_hp = data.robot_status.remain_hp;
    output.maximum_hp = data.robot_status.max_hp;
    output.shooter_barrel_cooling_value =
        data.robot_status.shooter_cooling_value;
    output.shooter_barrel_heat_limit = data.robot_status.shooter_heat_limit;
    output.chassis_power_limit = data.robot_status.chassis_power_limit;
    output.bullet_speed_limit = data.robot_status.bullet_speed_limit;
    output.power_management_output =
        static_cast<uint8_t>(data.robot_status.power_gimbal_output) |
        static_cast<uint8_t>(data.robot_status.power_chassis_output << 1U) |
        static_cast<uint8_t>(data.robot_status.power_launcher_output << 2U);
    output.shooter_17mm_1_barrel_heat = data.power_heat.launcher_id1_17_heat;
    output.robot_position_x = data.robot_pos.x;
    output.robot_position_y = data.robot_pos.y;
    output.robot_position_angle = data.robot_pos.angle;
    LibXR::Memory::FastCopy(&output.raw_damage_byte, &data.robot_damage,
                            sizeof(output.raw_damage_byte));
    output.projectile_allowance_17mm = data.bullet_remain.bullet_17_remain;
    output.projectile_allowance_42mm = data.bullet_remain.bullet_42_remain;
    output.remaining_gold_coin = data.bullet_remain.coin_remain;
    output.projectile_allowance_fortress = data.bullet_remain.bullet_17_store;
    return output;
  }

  void CheckProtocolStatus() {
    const uint32_t now_ms = NowMilliseconds();
    Referee::RobotGameRefereePack referee_data{};
    bool should_send = false;
    {
      LibXR::Mutex::LockGuard lock(referee_mutex_);
      should_send = protocol_status_dirty_ || next_protocol_status_ms_ == 0U ||
                    now_ms >= next_protocol_status_ms_;
      if (!should_send) {
        return;
      }
      referee_data = referee_data_;
      protocol_status_dirty_ = false;
      next_protocol_status_ms_ = now_ms + 1000U;
    }
    ProtocolStatusData output{};
    output.custom_protocol_version = PROTOCOL_VERSION;
    output.referee_protocol_major = 2U;
    output.referee_online = referee_data.referee_online ? 1U : 0U;
    output.supported_source_mask = Referee::SUPPORTED_SOURCE_MASK;
    output.valid_source_mask = referee_data.source_valid_mask;
    output.status_sequence = protocol_status_sequence_++;
    SendProtocolStatus(output);
  }
  static uint32_t NowMilliseconds() {
    return static_cast<uint32_t>(LibXR::Timebase::GetMilliseconds());
  }
  void Run() {
    while (true) {
      FindHeader();
      last_parse_ = ParseData();
      if (last_parse_) Publish();
    }
  }
  void FindHeader() {
    while (true) {
      if (uart_->Read({&byte_, 1U}, read_op_) != LibXR::ErrorCode::OK) {
        data_.status = Status::OFFLINE;
        continue;
      }
      if (byte_ != SOF) continue;
      rx_frame_.header.sof = byte_;
      uart_->Read({reinterpret_cast<uint8_t*>(&rx_frame_.header) + 1U,
                   sizeof(Header) - 1U},
                  read_op_);
      if (LibXR::CRC8::Verify(reinterpret_cast<uint8_t*>(&rx_frame_.header),
                              sizeof(Header))) {
        data_.status = Status::RUNNING;
        return;
      }
    }
  }
  bool ParseData() {
    const size_t bytes_after_header =
        static_cast<size_t>(rx_frame_.header.length) + CRC16_SIZE;
    if (bytes_after_header > sizeof(rx_frame_.body) ||
        uart_->Read({rx_frame_.body, bytes_after_header}, read_op_) !=
            LibXR::ErrorCode::OK ||
        !LibXR::CRC16::Verify(&rx_frame_,
                              sizeof(Header) + bytes_after_header)) {
      return false;
    }
    data_.status = Status::RUNNING;
    robot_command_parsed_ = false;
    if (static_cast<RxCommandID>(rx_frame_.header.id) !=
            RxCommandID::ROBOT_COMMAND ||
        rx_frame_.header.length < sizeof(RobotCommandPayload)) {
      return false;
    }
    LibXR::Memory::FastCopy(&data_.robot_command, rx_frame_.body,
                            sizeof(data_.robot_command));
    robot_command_parsed_ = true;
    return true;
  }
  bool IsRobotCommandExpiredLocked(uint32_t now_ms) const {
    return now_ms - (robot_command_received_ ? last_robot_command_time_ms_
                                             : startup_time_ms_) >=
           CHASSIS_COMMAND_TIMEOUT_MS;
  }
  void Publish() {
    LibXR::Mutex::LockGuard lock(chassis_mutex_);
    const uint32_t now_ms = NowMilliseconds();
    if (robot_command_parsed_) {
      last_robot_command_time_ms_ = now_ms;
      robot_command_received_ = true;
      stale_zero_published_ = false;
      robot_command_parsed_ = false;
    }
    PublishChassisTargetLocked(now_ms);
  }
  void PublishChassisTargetLocked(uint32_t now_ms) {
    HostData::HostChassisTarget chassis{};
    if (robot_command_received_ && !IsRobotCommandExpiredLocked(now_ms)) {
      chassis.vx = data_.robot_command.data.speed_vector.vx;
      chassis.vy = data_.robot_command.data.speed_vector.vy;
      chassis.w = data_.robot_command.data.speed_vector.wz;
      chassis_topic_.Publish(chassis);
      return;
    }
    chassis_topic_.Publish(chassis);
    last_zero_publish_time_ms_ = now_ms;
    stale_zero_published_ = true;
  }
  void CheckChassisCommandFreshness() {
    LibXR::Mutex::LockGuard lock(chassis_mutex_);
    const uint32_t now_ms = NowMilliseconds();
    if (!IsRobotCommandExpiredLocked(now_ms) ||
        (stale_zero_published_ && now_ms - last_zero_publish_time_ms_ <
                                      CHASSIS_ZERO_PUBLISH_INTERVAL_MS)) {
      return;
    }
    HostData::HostChassisTarget chassis{};
    chassis_topic_.Publish(chassis);
    last_zero_publish_time_ms_ = now_ms;
    stale_zero_published_ = true;
  }
  LibXR::UART* uart_;
  LibXR::Semaphore read_sem_;
  LibXR::ReadOperation read_op_;
  LibXR::Semaphore write_sem_;
  LibXR::WriteOperation write_op_;
  LibXR::Mutex tx_mutex_;
  LibXR::Mutex chassis_mutex_;
  LibXR::Mutex referee_mutex_;
  LibXR::Thread thread_;
  LibXR::Timer::TimerHandle chassis_watchdog_ = nullptr;
  LibXR::Topic chassis_topic_;
  ReceiveFrame rx_frame_{};
  Data data_{};
  Referee::RobotGameRefereePack referee_data_{};
  uint8_t tx_buffer_[MAX_FRAME_SIZE]{};
  uint8_t byte_ = 0U;
  uint32_t startup_time_ms_ = 0U;
  uint32_t last_robot_command_time_ms_ = 0U;
  uint32_t last_zero_publish_time_ms_ = 0U;
  uint32_t next_protocol_status_ms_ = 0U;
  uint8_t protocol_status_sequence_ = 0U;
  bool robot_command_received_ = false;
  bool robot_command_parsed_ = false;
  bool stale_zero_published_ = false;
  bool last_parse_ = false;
  bool protocol_status_dirty_ = true;
};

#ifdef WSPROTOCOL_RESTORE_DEBUG
#pragma pop_macro("DEBUG")
#undef WSPROTOCOL_RESTORE_DEBUG
#endif
