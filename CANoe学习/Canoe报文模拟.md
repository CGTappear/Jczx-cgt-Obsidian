## CANoe 报文发送模拟方式详解

### 概述

CANoe 提供多种报文发送仿真机制，涵盖从手动交互到全自动测试的各类场景。除 Interactive Generator（IG）外，以下是常用的八种方式。

---

### 一、CAPL 脚本发送

**适用场景：** 复杂仿真逻辑、自动化测试、事件驱动响应

CAPL（Communication Access Programming Language）是 CANoe 最核心、最灵活的报文发送方式。在 Simulation Setup 中为仿真节点编写 CAPL 程序，可实现几乎任意逻辑的报文收发控制。

**常用函数：**

| 函数                 | 说明        |
| ------------------ | --------- |
| `output()`         | 发送单帧报文    |
| `setTimer()`       | 单次定时触发    |
| `setTimerCyclic()` | 周期性定时发送   |
| `on message`       | 收到指定报文时触发 |
| `on key`           | 按键事件触发    |
| `on start`         | 仿真启动时自动执行 |

**示例：周期发送车速报文**

capl

```capl
variables {
  msTimer tSpeed;
  message VehicleSpeed msg_speed;
}

on start {
  setTimerCyclic(tSpeed, 10);  // 每 10ms 触发一次
}

on timer tSpeed {
  msg_speed.Speed = 80;        // 设置车速信号为 80 km/h
  output(msg_speed);           // 发送报文
}
```


---

### 二、面板（Panel）发送

**适用场景：** 半自动交互测试、Demo 演示、手动触发信号变化

通过 Panel Designer 创建图形化操作界面，将按钮、滑块、旋钮等控件与信号或报文绑定。运行时点击控件即可触发报文发送，无需编写代码。

**示例：** 创建一个"点火开关"按钮，绑定 `IgnitionStatus` 信号，点击后发送 `PowerMode` 报文。

---

### 三、重放块（Replay Block）

**适用场景：** 复现现场问题、回归测试、基于真实录制数据仿真

在 Simulation Setup 中添加 Replay Block，导入事先录制的 `.blf` 或 `.asc` 日志文件，CANoe 将按原始时间顺序将报文重新注入总线。

**典型流程：**

```
整车路测录制 .blf → 导入 Replay Block → 配置循环/触发条件 → 运行复现
```

**支持配置：**

- 循环播放
- 时间映射（加速/减速）
- 触发条件（手动 / 自动 / 事件触发）
---

### 四、信号发生器（Signal Generator）

**适用场景：** 模拟传感器渐变信号、压力测试、边界值扫描

在 IG 或独立模块中为信号配置波形，CANoe 自动周期性更新信号值并发送对应报文。

**支持波形类型：**

|波形|典型应用|
|---|---|
|正弦波|模拟发动机转速平滑变化|
|三角波|模拟油门踏板线性变化|
|锯齿波|模拟周期性递增信号|
|阶梯波|模拟挡位切换等离散变化|

**示例：** 对 `EngineSpeed` 信号配置 0~6000 rpm 的正弦波，CANoe 自动按周期更新并发送 `EngineData` 报文。


---

### 五、仿真节点（Simulation Node）

**适用场景：** 完整 ECU 行为仿真、节点替代、网络完整性验证

在 Simulation Setup 中添加模拟 ECU 节点，关联 CAPL 程序或 .NET 程序，节点可完整模拟真实 ECU 的请求-响应行为。

**示例：模拟 BCM 响应门控请求**

capl

```capl
on message DoorRequest {
  message DoorStatus resp;
  resp.Status = (this.Cmd == 1) ? OPEN : CLOSED;
  output(resp);  // 根据请求内容自动应答
}
```


---

### 六、测试模块（Test Module）

**适用场景：** 自动化测试流程、一致性测试、批量回归测试

使用 Test Module 或 Test Service Library（TSL）编写结构化测试用例，通过专用 API 函数发送报文并验证响应。

**示例：发送诊断请求并等待响应**

capl

```capl
testcase TC_ReadDTC() {
  message DiagRequest req;
  req.ServiceID = 0x19;
  req.SubFunction = 0x02;
  output(req);

  // 等待 500ms 内收到 DiagResponse 报文
  if (TestWaitForMessage(DiagResponse, 500) == 1) {
    TestStepPass("DTC Response received");
  } else {
    TestStepFail("No response within timeout");
  }
}
```

---

### 七、系统变量 / 环境变量联动触发

**适用场景：** 跨模块联动、状态机驱动仿真

在 CAPL 中监听系统变量（System Variable）或环境变量的变化事件，当变量值改变时触发报文发送。可与 Panel、测试模块等多种模块联动。

**示例：面板切换挡位时触发发送**

capl

```capl
on sysvar Gearbox::CurrentGear {
  message GearStatus msg;
  msg.Gear = @Gearbox::CurrentGear;
  output(msg);
}
```

---

### 八、数据库配置周期发送（Cycle Time）

**适用场景：** 快速建立基础网络帧，周期报文自动上线

直接在 CANdb++ 数据库中为报文配置 `Cycle Time` 属性，CANoe 仿真启动后可自动按周期发送该报文，无需额外编程。也可在 CAPL 中通过 `setTimerCyclic()` 实现相同效果。


---

### 综合对比

| 方式          | 适用场景       | 灵活度    | 自动化程度  | 编程要求 |
| ----------- | ---------- | ------ | ------ | ---- |
| IG          | 手动快速发送     | 中      | 低      | 无    |
| **CAPL 脚本** | 复杂仿真 / 自动化 | **最高** | 高      | 高    |
| 面板 Panel    | 半手动交互测试    | 中      | 中      | 低    |
| 重放块         | 日志回放复现     | 低      | 高      | 无    |
| 信号发生器       | 模拟渐变信号     | 中      | 高      | 无    |
| **仿真节点**    | 完整网络仿真     | **高**  | 高      | 高    |
| **测试模块**    | 自动化测试      | 高      | **最高** | 高    |
| 系统变量联动      | 跨模块状态驱动    | 中      | 中      | 中    |
| 数据库周期配置     | 快速周期帧上线    | 低      | 高      | 无    |