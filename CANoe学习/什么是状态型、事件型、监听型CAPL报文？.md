## 一、三种类型的定义与本质


| 类型      | 核心逻辑                    | 总线角色            | 发送驱动力                                       |
| :------ | :---------------------- | :-------------- | :------------------------------------------ |
| **状态型** | 内部维护状态机，周期性广播当前状态       | 主动节点 / 仿真 ECU   | 定时器到期（`on msTimer`）                         |
| **事件型** | 收到特定触发后才产生并发送报文         | 响应节点 / 触发执行器    | 外部事件（`on message` / `on key` / `on sysvar`） |
| **监听型** | 只接收、解析、记录或转发，本身不主动发送该报文 | 监控节点 / 网关 / 诊断仪 | 无发送，纯接收处理                                   |

---

## 二、状态型报文（State-Based）

**本质**：节点内部有状态变量，定时器周期性地把状态"广播"到总线上。

**典型场景**：仿真发动机 ECU，每 10ms 广播一次转速、温度。

```capl
variables
{
  message EngineData txEngine;    // DBC 绑定的发送报文
  msTimer t10ms;
  
  // 内部状态
  int gRPM;
  double gTemp;
  byte gErrorCode;
}

on start
{
  gRPM = 800;       // 怠速
  gTemp = 85.5;
  gErrorCode = 0;
  setTimer(t10ms, 10);
}

on msTimer t10ms
{
  // === 状态更新（可以是算法、查表、渐变逻辑）===
  if (gRPM < 3000) gRPM += 10;    // 模拟转速爬升
  
  // === 将状态装入报文 ===
  txEngine.EngineRPM = gRPM;       // DBC 信号级赋值
  txEngine.EngineTemp = gTemp;
  txEngine.ErrorState = gErrorCode;
  
  // === 周期性广播 ===
  output(txEngine);
  setTimer(t10ms, 10);
}
```

**关键特征**：

- 发送周期由 `setTimer` 严格保证
    
- 报文内容是**内部全局变量**的镜像
    
- 即使没人监听，也持续发送（真实 ECU 的行为）
    

---

## 三、事件型报文（Event-Based）

**本质**：平时不发，收到某个"刺激"后才发送一次或 burst 发送。

**典型场景**：收到诊断请求后回复响应；收到按键指令后执行门锁动作。

```capl
variables
{
  message 0x123 doorLockAck;      // 门锁响应报文
  linFrame 0x05 doorLinResp;      // LIN 响应帧
}

/* 事件源1：收到 CAN 控制指令 */
on message 0x200                  // 收到车身控制报文
{
  if (this.DoorLockCmd == 1)      // 指令：上锁
  {
    // 执行动作...
    doorLockAck.DoorLocked = 1;
    doorLockAck.Result = 0x00;    // OK
    
    // === 事件触发发送 ===
    output(doorLockAck);          // 只发一次，非周期
  }
}

/* 事件源2：Panel 按钮被按下 */
on sysvar Panel::EmergencyUnlock
{
  if (@Panel::EmergencyUnlock == 1)
  {
    doorLinResp.dlc = 4;
    doorLinResp.byte(0) = 0xFF;   // 紧急解锁指令
    output(doorLinResp);
    
    @Panel::EmergencyUnlock = 0; // 复位，等待下次事件
  }
}

/* 事件源3：键盘调试触发 */
on key 's'
{
  write("手动触发发送");
  output(testMsg);
}
```

**关键特征**：

- 发送是**离散**的，没有固定周期
    
- 发送体通常位于 `on message` / `on sysvar` / `on key` 内部
    
- 常伴随**状态转换**（如收到 A → 发送 B → 进入等待状态）
    

---

## 四、监听型报文（Monitoring / Passive）

**本质**：节点只"看"不"说"，用于监控总线、做网关转发、记录数据或验证时序。

**典型场景**：诊断仪监听 UDS 响应；网关捕获 CAN 报文转发到 LIN；测试脚本验证某报文是否在规定时间内到达。

```capl
variables
{
  dword gLastEngineID_Time;
  int gEngineRxCount;
  byte gLastData[8];
}

/* 监听1：纯记录 */
on message EngineData             // 监听发动机报文
{
  gEngineRxCount++;
  gLastEngineID_Time = timeNow(); // 记录时间戳
  
  // 解析并记录
  write("监听到 EngineData: RPM=%d, Count=%d", 
        this.EngineRPM, gEngineRxCount);
}

/* 监听2：条件告警（被动响应，但不发原报文） */
on message 0x123
{
  // 只监控，不发送 0x123
  if (this.byte(7) != calcChecksum(this))
  {
    write("警告：ID 0x123 校验和错误！");
  }
}

/* 监听3：网关转发（收到 CAN → 转发到 LIN） */
on message CAN2.DoorStatus        // 监听 CAN2 通道
{
  // 这是监听型节点，自己不产生 DoorStatus，
  // 只是把监听到的内容转发出去
  linFrame 0x10 linDoor;
  linDoor.byte(0) = this.DoorLockState;
  linDoor.byte(1) = this.WindowPos;
  
  output(linDoor);                // 转发到 LIN（发的是新报文，不是原报文）
}

/* 监听4：测试验证（TFS） */
testcase VerifyEngineAlive()
{
  // 监听是否收到报文
  if (testWaitForMessage(EngineData, 2000))
  {
    testStepPass("2秒内收到 EngineData");
  }
  else
  {
    testStepFail("EngineData 丢失");
  }
}
```

**关键特征**：

- 代码里有 `on message`，但**没有该报文的 `output()`**
    
- 可能产生**衍生报文**（网关场景），但不回发原报文
    
- 常用于诊断、测试、监控、分析
    

---

## 五、三者的系统级对比

表格

|维度|状态型|事件型|监听型|
|:--|:--|:--|:--|
|**代码标志**|`on msTimer` + `output()`|`on message/sysvar/key` + `output()`|`on message`，无 `output()` 或转发|
|**发送周期**|固定周期|离散/随机|不发送（或只发衍生报文）|
|**全局变量**|大量状态变量|少量标志位|计数器、时间戳、缓存|
|**DBC/LDF 角色**|发送节点（Tx）|响应节点（Rx→Tx）|接收节点（Rx）|
|**典型节点名**|EngineECU, DoorECU|Gateway, Tester|Logger, Analyzer, TestModule|
|**与总线耦合度**|高（必须按时发送）|中（依赖外部刺激）|低（可插拔不影响总线）|
|**CAPL 节点颜色**|蓝色（仿真节点）|蓝色/绿色|绿色（测试模块）或蓝色|

---

## 六、工程中的混合设计

真实节点往往是**混合型**：

```capl
variables
{
  message 0x100 txStatus;         // === 状态型 ===
  message 0x200 txEvent;          // === 事件型 ===
  msTimer t100ms;
  
  int gSystemState;   // 0=Init, 1=Normal, 2=Error
}

/* 状态型：周期性广播心跳 */
on msTimer t100ms
{
  txStatus.byte(0) = gSystemState;
  txStatus.byte(1) = gErrorCount;
  output(txStatus);
  setTimer(t100ms, 100);
}

/* 事件型：收到指令后回复 */
on message 0x701                  // 诊断请求
{
  if (this.byte(0) == 0x10)       // Session Control
  {
    gSystemState = 1;
    txEvent.byte(0) = 0x50;       // Positive Response
    txEvent.byte(1) = 0x01;       // Default Session
    output(txEvent);              // 事件触发发送
  }
}

/* 监听型：监控总线错误 */
on errorFrame
{
  gErrorCount++;
  write("监听到错误帧 #%d", gErrorCount);
}
```