# WsProtocol

收发 `pldx_ws` version 2 串口帧。接收端缓存完整的 `ID_ROBOT_CMD` 载荷，并将
经过 CRC 校验的导航速度发布到现有 `HostData` 控制链路；发送端覆盖所有既有
MCU-to-PC 命令。

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

## Template Arguments
None

## Depends

- `pldx/HostData`
