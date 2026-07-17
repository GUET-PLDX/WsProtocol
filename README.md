# WsProtocol

收发 `pldx_ws` 自定义串口协议 V3。接收端缓存完整的 `ID_ROBOT_CMD` 载荷，并将
经过 CRC 校验的导航速度发布到现有 `HostData` 控制链路；发送端覆盖所有既有
MCU-to-PC 命令。

云台本地 `sentry_ref` 由 `DualBoard` 产生。模块按 `source_command_id` 发送对应的
`EVENT_DATA`、`ALL_ROBOT_HP`、`GAME_STATUS`、`RFID_STATUS`、`ROBOT_STATUS`
和 `BUFF`，并在启动、状态变化及每 1000 ms 发送 `PROTOCOL_STATUS (0x0F)`。
V3 的 HP 使用友敌相对字段，RFID 为 5 字节原始位，Buff 冷却增益为 16 位。

仅 `speed_vector.vx`、`vy` 和 `wz` 当前会被发布。其他接收字段保持缓存以保持
线协议兼容。最后一个有效机器人命令超过 50 ms 后，模块通过独立 Timer 任务每 50 ms
发布零底盘速度，直到链路恢复。

## Required Hardware

- `uart_name`: 连接导航上位机的 UART。哨兵配置使用 `uart_ext_controller`
  (`USART6`)。

## Constructor Arguments

- `task_stack_depth_uart`: 串口接收线程栈大小。
- `uart`: UART 硬件别名。
- `baudrate`: UART 波特率。
- `chassis_topic_name`: `HostData::HostChassisTarget` Topic 名称。
- `thread_priority_uart`: 生成构造契约保留的线程优先级参数。
- `referee_topic_name`: 云台本地裁判摘要 Topic，哨兵配置为 `sentry_ref`。

## Template Arguments
None

## Depends

- `pldx/HostData`
