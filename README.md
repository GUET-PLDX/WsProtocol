# WsProtocol

接收 `pldx_ws` 的 `standard_robot_pp_ros2` v2 串口命令帧，并将经过 CRC
校验的导航速度发布到现有 `HostData` 控制链路。

仅使用 `ID_ROBOT_CMD (0x01)` 帧的 `speed_vector.vx`、`vy` 和 `wz` 字段。
云台、发射、底盘姿态和 tracking 字段会被忽略。

## Required Hardware

- `uart_name`: 连接导航上位机的 UART。哨兵配置使用 `uart_ext_controller`
  (`USART6`)。

## Constructor Arguments

- `uart_name`: UART 硬件别名。
- `chassis_topic_name`: `HostData::HostChassisTarget` Topic 名称。
- `task_stack_depth`: 串口接收线程栈大小。
- `thread_priority`: 串口接收线程优先级。

## Template Arguments
None

## Depends

- `pldx/HostData`
