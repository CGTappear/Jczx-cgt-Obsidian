![[Pasted image 20260605115825.png]]
## CANoe Application Programming Language Reference Guide

**版本**：适用于 CANoe 16.x / 17.x / 18.x  
**适用硬件**：CANcaseXL, VN16xx, VN56xx, VN76xx 等 Vector 接口  
**文档性质**：系统级编程参考手册

---

## 目录

1. [CAPL 系统定位与概述](#第1章-capl-系统定位与概述)
2. [开发环境与文件结构](#第2章-开发环境与文件结构)
3. [程序结构三段式](#第3章-程序结构三段式)
4. [数据类型体系](#第4章-数据类型体系)
5. [运算符与流程控制](#第5章-运算符与流程控制)
6. [事件驱动模型](#第6章-事件驱动模型)
7. [CAN 报文操作](#第7章-can-报文操作)
8. [LIN 报文操作](#第8章-lin-报文操作)
9. [定时器与任务调度](#第9章-定时器与任务调度)
10. [系统变量与环境变量](#第10章-系统变量与环境变量)
11. [诊断协议编程](#第11章-诊断协议编程)
12. [测试功能集 TFS](#第12章-测试功能集-tfs)
13. [函数与代码复用](#第13章-函数与代码复用)
14. [调试技术与优化](#第14章-调试技术与优化)
15. [工程级综合示例](#第15章-工程级综合示例)

---

## 第1章 CAPL 系统定位与概述

### 1.1 什么是 CAPL
CAPL（CAN Access Programming Language）是 Vector Informatik 为 CANoe / CANalyzer 开发的**事件驱动型嵌入式脚本语言**。它直接编译为字节码，由 CANoe 运行时内核调度执行，与 Vector XL Driver 硬件驱动层无缝衔接。

### 1.2 在系统中的位置
```
┌─────────────────────────────────────────────┐
│              CANoe / CANalyzer               │
│  ┌─────────┐  ┌─────────┐  ┌─────────────┐ │
│  │  Panel  │  │  Trace  │  │   Graphics  │ │  ← 人机交互层
│  └────┬────┘  └─────────┘  └─────┬───────┘ │
│       │                           │         │
│  ┌────┴───────────────────────────┴─────┐   │
│  │        System Variables (SV)          │   │  ← 全局数据总线
│  └────┬───────────────────────────┬─────┘   │
│       │                           │         │
│  ┌────┴────┐  ┌────────┐  ┌──────┴─────┐   │
│  │Simulation│  │  Test  │  │ Diagnostics│   │
│  │ Setup   │  │ Module │  │   Panel    │   │
│  │ (CAPL)  │  │ (CAPL) │  │  (CAPL)    │   │  ← CAPL 执行层
│  └────┬────┘  └───┬────┘  └─────┬──────┘   │
│       └───────────┴─────────────┘           │
│              XL Driver Library              │  ← 硬件抽象层
│                   ↓                         │
│           CANcaseXL / VN1630A ...            │  ← 物理层
└─────────────────────────────────────────────┘
```

### 1.3 核心特征
| 特征 | 说明 |
|------|------|
| **事件驱动** | 无 `main()`，由总线事件、时间事件、系统事件触发 |
| **总线原生** | `message`、`linFrame` 等类型直接映射物理帧结构 |
| **数据库耦合** | 原生支持 DBC、LDF、CDD/ODX 符号化访问 |
| **C 语言风格** | 语法接近 ANSI C，支持预处理器 `#include`、`#define` |
| **实时调度** | 毫秒级定时器精度，由 CANoe 内核事件队列调度 |

---

## 第2章 开发环境与文件结构

### 2.1 文件类型
| 扩展名 | 用途 | 关联组件 |
|--------|------|----------|
| `.can` | CAPL 源代码 | Simulation Setup / Test Module |
| `.cin` | CAPL 头文件（Include） | 公共常量、结构体声明 |
| `.cbf` | 编译后字节码 | CANoe 运行时加载 |

### 2.2 编译与关联
1. 在 **Simulation Setup** 中右键节点 → **Configuration** → **File** 关联 `.can`
2. 点击工具栏 **Compile** 或快捷键 `F9`
3. 编译错误在 **CAPL Browser** 下方 **Compile** 窗口显示

### 2.3 CAPL Browser 界面分区
- **Navigator**：文件、数据库、系统变量树
- **Editor**：代码编辑区（支持语法高亮、自动补全）
- **Symbols**：当前作用域内可用的符号（报文、信号、变量）
- **Compile**：编译日志与错误定位

---

## 第3章 程序结构三段式

所有 `.can` 文件必须严格按以下顺序组织：

```capl
/* 0. 文件头注释与预处理 */
includes
{
  #include "my_header.cin"    // 头文件包含
}

variables
{
  /* 1. 全局变量声明区 */
  message 0x123 txMsg;       // CAN 报文对象
  msTimer cyclicTimer;         // 毫秒定时器
  int gCounter;                // 全局计数器
}

/* 2. 用户自定义函数区 */
void initGlobals()
{
  gCounter = 0;
}

byte calcChecksum(byte data[])
{
  byte sum = 0;
  int i;
  for (i = 0; i < 8; i++) sum += data[i];
  return sum;
}

/* 3. 事件处理程序区 */
on start
{
  initGlobals();
  setTimer(cyclicTimer, 100);
}

on stop
{
  cancelTimer(cyclicTimer);
}

on msTimer cyclicTimer
{
  gCounter++;
  output(txMsg);
  setTimer(cyclicTimer, 100);   // 重装定时器
}
```

### 3.1 区段规则
| 区段 | 出现次数 | 说明 |
|------|----------|------|
| `includes` | 0 或 1 | 必须在最开头 |
| `variables` | 0 或 1 | 必须在 includes 之后 |
| `functions` | 隐式区段 | 事件处理程序之外的函数体 |
| `on *` | 任意多个 | 事件处理程序，顺序无关 |

---

## 第4章 数据类型体系

### 4.1 基础标量类型
| 类型 | 位宽 | 范围 | 说明 |
|------|------|------|------|
| `byte` | 8 bit | 0 ~ 255 | 无符号，总线数据操作最常用 |
| `word` | 16 bit | 0 ~ 65535 | 无符号 |
| `dword`| 32 bit | 0 ~ 2³²-1 | 无符号 |
| `qword`| 64 bit | 0 ~ 2⁶⁴-1 | 无符号 |
| `int` | 32 bit | -2³¹ ~ 2³¹-1 | 有符号 |
| `long` | 32 bit | 同 `int` | 有符号 |
| `long long` | 64 bit | 有符号 | 大整数运算 |
| `float` | 32 bit | IEEE 754 | 浮点 |
| `double`| 64 bit | IEEE 754 | 双精度浮点 |
| `char` | 8 bit | -128 ~ 127 | ASCII 字符，可组成字符串 |

### 4.2 字符串类型
```capl
char gText[64];          // 字符数组即字符串
strcpy(gText, "Hello");  // 字符串拷贝
strlen(gText);          // 长度
```

### 4.3 总线对象类型（核心）

#### CAN 报文对象：`message`
```capl
variables
{
  /* 方式1：裸 ID 定义 */
  message 0x123 rawMsg;
  message 0x456 rxMsg;

  /* 方式2：DBC 符号名定义（推荐） */
  message EngineData engineTx;
  message ABS_Status absRx;

  /* 方式3：CAN FD */
  message 0x100 fdMsg;
}
```

**报文对象属性**：
| 属性 | 类型 | 说明 |
|------|------|------|
| `.id` | `dword` | 报文标识符（标准 11bit / 扩展 29bit） |
| `.DLC` / `.dlc` | `byte` | 数据长度代码（CAN: 0-8, CAN FD: 0-64） |
| `.byte(index)` | `byte` | 按字节索引读写（0-7 或 0-63） |
| `.word(index)` | `word` | 按字索引读写（Motorola 大端序） |
| `.long(index)` | `dword`| 按双字索引读写 |
| `.dir` | `byte` | 方向：0=RX, 1=TX |
| `.rtr` | `byte` | 远程帧标志 |
| `.FDF` | `byte` | CAN FD 数据帧标志 |
| `.BRS` | `byte` | 波特率切换标志 |
| `.channel` | `byte` | 发送通道号 |

**信号级访问（DBC 绑定）**：
```capl
on message EngineData
{
  int rpm = this.EngineRPM;        // 自动按 DBC 因子偏移量转换物理值
  double temp = this.EngineTemp;   // 浮点信号

  // 写入发送报文
  engineTx.EngineRPM = 3000;
  engineTx.EngineTemp = 85.5;
}
```

#### LIN 帧对象：`linFrame`
```capl
variables
{
  /* 裸 ID 定义（0-63） */
  linFrame 0x05 doorLock;

  /* LDF 符号名定义 */
  linFrame DoorState doorStateTx;
}
```

**LIN 帧属性**：
| 属性 | 说明 |
|------|------|
| `.id` | 原始 ID（0-63） |
| `.pid` | Protected Identifier（含校验位） |
| `.dlc` | 数据长度（1-8） |
| `.byte(index)` | 字节访问 |
| `.checksumModel` | 校验模型：0=Classic, 1=Enhanced, 2=Autosar |

### 4.4 定时器类型
```capl
variables
{
  timer t1;        // 秒级定时器
  msTimer t2;      // 毫秒级定时器（最常用）
}
```

### 4.5 诊断对象类型
```capl
variables
{
  diagRequest Door.UDS_ReadDTC req;     // 请求对象（绑定 CDD）
  diagResponse Door.UDS_ReadDTC resp;   // 响应对象
}
```

### 4.6 数组与结构体
```capl
variables
{
  byte buffer[8];           // 一维数组
  int matrix[4][4];        // 二维数组

  struct MyStruct           // 结构体
  {
    int id;
    byte data[8];
  };

  MyStruct myVar;
}
```

---

## 第5章 运算符与流程控制

### 5.1 运算符（与 C 语言一致）
| 类别   | 运算符                    | 示例                      |
| ---- | ---------------------- | ----------------------- |
| 算术   | `+ - * / %`            | `a = b % 3`             |
| 位运算  | `& \| ^ ~ << >>`       | `flags = flags \| 0x01` |
| 逻辑   | `&& \|\| !`            | `if (a > 0 && b < 10)`  |
| 关系   | `== != > < >= <=`      | `if (msg.id == 0x123)`  |
| 赋值   | `= += -= *= /= &= \|=` | `count += 1`            |
| 自增减  | `++ --`                | `i++`                   |
| 条件   | `? :`                  | `max = (a > b) ? a : b` |
| 成员   | `.`                    | `msg.byte(0)`           |
| 数组索引 | `[]`                   | `data[3]`               |

### 5.2 流程控制
```capl
/* 条件分支 */
if (gState == 0)
{
  initSequence();
}
else if (gState == 1)
{
  normalSequence();
}
else
{
  errorHandler();
}

/* 多路分支 */
switch(msg.id)
{
  case 0x123:
    handleEngineData();
    break;
  case 0x456:
    handleABS();
    break;
  default:
    break;
}

/* 循环 */
for (i = 0; i < 8; i++)
{
  sum += msg.byte(i);
}

while (gCounter < 100)
{
  gCounter++;
}

/* 跳转 */
break;      // 跳出循环或 switch
continue;   // 跳过本次循环剩余代码
return;     // 从函数返回
```

---

## 第6章 事件驱动模型

CAPL 是**纯事件驱动**语言。理解事件类型等于理解 CAPL 的执行架构。

### 6.1 事件分类总览
```
事件源
├── 总线事件
│   ├── on message          (CAN 报文到达)
│   ├── on linFrame         (LIN 帧到达)
│   ├── on signal           (信号值变化)
│   ├── on signal_update    (报文到达即触发，值可能未变)
│   ├── on errorFrame       (错误帧)
│   └── on busOff           (总线关闭)
├── 时间事件
│   ├── on timer            (秒级定时器到期)
│   └── on msTimer          (毫秒级定时器到期)
├── 系统事件
│   ├── on start            (Measurement 启动)
│   ├── on stop             (Measurement 停止)
│   ├── on preStart         (Measurement 启动前，预初始化)
│   ├── on postStop         (Measurement 停止后，清理)
│   ├── on key 'x'          (键盘按键)
│   └── on envVar           (环境变量变化，Legacy)
├── 变量事件
│   ├── on sysvar           (系统变量值变化触发)
│   └── on sysvar_update    (系统变量写操作触发)
└── 诊断事件
    ├── on diagRequest      (诊断请求到达，ECU 仿真)
    └── on diagResponse     (诊断响应到达，Tester 端)
```

### 6.2 CAN 报文事件
```capl
/* 按十六进制 ID 捕获 */
on message 0x123
{
  write("收到 ID 0x123, DLC=%d", this.dlc);
}

/* 按 DBC 符号名捕获（推荐） */
on message EngineData
{
  int rpm = this.EngineRPM;
  write("Engine RPM = %d", rpm);
}

/* 捕获所有报文（监控/网关场景） */
on message *
{
  if (this.id >= 0x100 && this.id <= 0x1FF)
  {
    write("捕获到报文: 0x%X", this.id);
  }
}

/* 捕获指定通道的报文 */
on message CAN2.*           // 仅捕获 CAN2 通道
{
  // 多通道仿真时区分通道
}

/* 使用 this 关键字 */
on message 0x123
{
  // this 指代当前事件关联的报文对象
  byte b0 = this.byte(0);
  byte dlc = this.dlc;
}
```

### 6.3 LIN 帧事件
```capl
/* 按 ID 捕获 */
on linFrame 0x05
{
  write("LIN 帧 0x05 到达, 数据: %02X", this.byte(0));
}

/* 按 LDF 符号名捕获 */
on linFrame DoorState
{
  // 处理车门状态帧
}

/* LIN 错误帧 */
on linFrameError *
{
  write("LIN 错误帧: ID=0x%02X", this.id);
}
```

### 6.4 信号事件
```capl
/* 信号值变化时触发 */
on signal EngineData::EngineRPM
{
  // 仅当 EngineRPM 值改变时执行
  if ($EngineRPM > 4000)     // $ 是信号引用简写
  {
    write("转速过高警告!");
  }
}

/* 每次报文到达都触发（即使信号值未变） */
on signal_update EngineData::EngineRPM
{
  // 适用于需要严格周期处理的场景
}
```

### 6.5 时间事件
```capl
variables
{
  msTimer t100ms;
  timer t1s;
}

on start
{
  setTimer(t100ms, 100);       // 100ms 后触发
  setTimerCyclic(t1s, 1);      // 每 1 秒周期性触发（CANoe 8.0+）
}

on msTimer t100ms
{
  output(cyclicMsg);
  setTimer(t100ms, 100);       // 重装，实现周期发送
}

on timer t1s
{
  write("1秒定时器触发");
  // setTimerCyclic 模式下无需重装
}
```

### 6.6 系统事件
```capl
on start
{
  // Measurement 启动时执行一次
  // 用于初始化变量、启动定时器、发送首帧
  initSequence();
}

on stop
{
  // Measurement 停止时执行一次
  // 用于清理资源、取消定时器
  cancelTimer(t100ms);
}

on preStart
{
  // 在总线硬件初始化之后、Measurement 启动之前执行
  // 用于预加载数据库配置
}

on key 'a'
{
  // 按键盘 'a' 键触发（调试注入）
  output(testMsg);
  write("手动发送测试报文");
}

on key '0'
{
  // 数字键 '0'
}
```

### 6.7 系统变量事件
```capl
/* 值变化触发 */
on sysvar Panel::SendButton
{
  if (@Panel::SendButton == 1)   // @ 运算符读取系统变量
  {
    output(txMsg);
    @Panel::SendButton = 0;        // 写入系统变量（复位按钮）
  }
}

/* 每次写操作都触发（即使值相同） */
on sysvar_update Panel::Counter
{
  // 适用于高频计数器场景
}
```

### 6.8 诊断事件
```capl
/* Tester 端：收到诊断响应 */
on diagResponse Door.UDS_ReadDTC
{
  byte count = this.GetParameter("DTCCount");
  write("收到 %d 个 DTC", count);
}

/* ECU 仿真端：收到诊断请求 */
on diagRequest Door.UDS_ReadDTC
{
  // 构造响应
  diagResponse Door.UDS_ReadDTC resp;
  resp.SetParameter("DTCCount", 2);
  diagSendResponse(resp);
}
```

### 6.9 事件组合（多事件共享处理体）
```capl
on message 0x123
on message 0x456
on sysvar Panel::Emergency
{
  // 多个事件源共享同一处理逻辑
  output(alertMsg);
}
```

---

## 第7章 CAN 报文操作

### 7.1 发送基础
```capl
variables
{
  message 0x123 txMsg;
}

on start
{
  txMsg.dlc = 8;
  txMsg.byte(0) = 0x55;
  txMsg.byte(1) = 0xAA;
  // ...
  output(txMsg);               // 发送到默认通道
}
```

### 7.2 按字节/字/长字填充
```capl
on key 's'
{
  message 0x200 msg;
  msg.dlc = 8;

  // 字节级（最常用）
  msg.byte(0) = 0x11;
  msg.byte(1) = 0x22;

  // 字级（16-bit, Motorola 大端）
  msg.word(2) = 0x3344;        // byte[2]=0x33, byte[3]=0x44

  // 长字级（32-bit）
  msg.long(4) = 0x55667788;    // byte[4..7]

  output(msg);
}
```

### 7.3 DBC 符号化发送
```capl
variables
{
  message EngineData txEngine;
}

on msTimer engineCycle
{
  txEngine.EngineRPM = 3000;           // 物理值自动转换
  txEngine.VehicleSpeed = 60.5;        // 浮点信号
  txEngine.EngineTemp = 85;            // 整型信号
  txEngine.ErrorFlag = 1;              // 枚举/开关信号

  output(txEngine);
}
```

### 7.4 多通道发送
```capl
/* 方式1：修改报文 channel 属性 */
message 0x100 gatewayMsg;
gatewayMsg.channel = 2;              // 发送到 CAN2
output(gatewayMsg);

/* 方式2：使用 outputToBus */
outputToBus(gatewayMsg, "CAN2");     // 按名称
outputToBus(gatewayMsg, 2);          // 按通道索引
```

### 7.5 CAN FD 发送
```capl
variables
{
  message 0x100 fdMsg;
}

on start
{
  fdMsg.FDF = 1;                     // 使能 CAN FD
  fdMsg.BRS = 1;                     // 使能波特率切换
  fdMsg.dlc = 12;                    // CAN FD DLC: 12/16/20/24/32/48/64
  fdMsg.byte(0) = 0x01;
  // ...
  output(fdMsg);
}
```

### 7.6 远程帧发送
```capl
variables
{
  message 0x123 rtrMsg;
}

on key 'r'
{
  rtrMsg.rtr = 1;                    // 设置为远程帧
  rtrMsg.dlc = 0;                    // 远程帧无数据
  output(rtrMsg);
}
```

### 7.7 接收与解析
```capl
on message 0x123
{
  // this 指向接收到的报文实例
  byte dlc = this.dlc;
  byte b0 = this.byte(0);
  byte b1 = this.byte(1);

  // 多字节解析（Motorola 大端）
  word rpmRaw = (this.byte(0) << 8) + this.byte(1);

  // 条件过滤
  if (this.byte(7) == calcChecksum(this))
  {
    write("校验通过");
  }
}
```

---

## 第8章 LIN 报文操作

### 8.1 LIN 架构回顾
LIN 是**单主多从**架构：
- **Master**：发送 Header（Break + Sync + PID），决定调度时序
- **Slave**：在 Master 发送 Header 后，于响应时隙填充 Data + Checksum

### 8.2 Master 发送（Header + Data）
```capl
variables
{
  linFrame 0x05 doorStatus;      // Master 负责发送此帧
  msTimer linSchedule;            // LIN 调度定时器
}

on start
{
  setTimer(linSchedule, 10);    // 10ms 调度周期
}

on msTimer linSchedule
{
  doorStatus.dlc = 8;
  doorStatus.byte(0) = 0x00;
  doorStatus.byte(1) = 0x01;
  doorStatus.byte(2) = 0x02;
  doorStatus.byte(3) = 0x03;
  doorStatus.byte(4) = 0xF2;     // 你的数据
  doorStatus.byte(5) = 0x00;
  doorStatus.byte(6) = 0x00;
  doorStatus.byte(7) = 0x00;

  output(doorStatus);            // Master 发送完整帧
  setTimer(linSchedule, 10);
}
```

### 8.3 Slave 响应（仅 Data）
```capl
variables
{
  linFrame 0x10 sensorResp;       // Slave 负责此 ID 的响应数据
}

on linFrame 0x10                  // Master 发出 Header 后触发
{
  // 在 Slave 响应时隙填充数据
  sensorResp.dlc = 4;
  sensorResp.byte(0) = 0x11;
  sensorResp.byte(1) = 0x22;
  sensorResp.byte(2) = 0x33;
  sensorResp.byte(3) = 0x44;

  output(sensorResp);             // 发送 Data 段（不重复发 Header）
}
```

### 8.4 LDF 符号化操作
```capl
variables
{
  linFrame DoorState doorTx;
}

on msTimer doorCycle
{
  doorTx.DoorLock = 1;            // 锁定
  doorTx.WindowPos = 50;          // 窗户位置 50%
  doorTx.ChildLock = 0;           // 儿童锁关闭

  output(doorTx);
}
```

### 8.5 LIN 调度表控制
```capl
on start
{
  // 切换到指定调度表（名称来自 LDF）
  linChangeSched(0, "Normal");         // 通道0, Normal表
  linChangeSched(0, "Diagnostic");     // 诊断调度表
  linChangeSched(0, "MasterRequest");  // MRF 表
}

on key 'n'
{
  linChangeSched(0, "Normal");
}

on key 'd'
{
  linChangeSched(0, "Diagnostic");
}
```

### 8.6 LIN 校验模型设置
```capl
variables
{
  linFrame 0x05 frame;
}

on start
{
  frame.checksumModel = 1;       // 0=Classic, 1=Enhanced, 2=Autosar
}
```

### 8.7 LIN 唤醒与休眠
```capl
on key 'w'
{
  linWakeup(0);                   // 通道0发送唤醒信号
}

on key 's'
{
  linSleep(0);                    // 通道0进入休眠
}
```

---

## 第9章 定时器与任务调度

### 9.1 定时器类型
| 类型 | 精度 | 声明 | 适用场景 |
|------|------|------|----------|
| `timer` | 1 秒 | `timer t1;` | 长周期任务、心跳超时 |
| `msTimer` | 1 毫秒 | `msTimer t2;` | 周期报文发送、调度 |

### 9.2 定时器控制函数
```capl
/* 单次触发 */
setTimer(t100ms, 100);           // 100ms 后触发一次 on msTimer t100ms

/* 循环触发（CANoe 8.0+） */
setTimerCyclic(t1s, 1);          // 每 1 秒触发，无需手动重装

/* 取消 */
cancelTimer(t100ms);             // 从内核移除待触发事件

/* 查询状态 */
if (isTimerActive(t100ms))
{
  write("定时器正在运行");
}
```

### 9.3 周期报文发送模式
```capl
variables
{
  message 0x100 cyclicMsg;
  msTimer t100ms;
}

on start
{
  cyclicMsg.dlc = 8;
  setTimer(t100ms, 100);
}

on msTimer t100ms
{
  cyclicMsg.byte(0)++;
  output(cyclicMsg);
  setTimer(t100ms, 100);         // 必须重装！
}
```

### 9.4 动态周期调整
```capl
variables
{
  int gCycleTime;
  msTimer dynamicTimer;
}

on start
{
  gCycleTime = 100;
  setTimer(dynamicTimer, gCycleTime);
}

on sysvar Panel::CycleTime
{
  gCycleTime = @Panel::CycleTime;
  cancelTimer(dynamicTimer);
  setTimer(dynamicTimer, gCycleTime);
}
```

### 9.5 一次性延时任务
```capl
variables
{
  msTimer oneShot;
}

on message 0x123
{
  // 收到某报文后，延时 500ms 执行动作
  setTimer(oneShot, 500);
}

on msTimer oneShot
{
  // 500ms 后执行
  output(responseMsg);
  // 无需重装，单次任务完成
}
```

---

## 第10章 系统变量与环境变量

### 10.1 系统变量（System Variables）
系统变量是 **CANoe 全局数据总线**，用于：
- Panel 控件与 CAPL 数据交换
- 不同 CAPL 节点间数据共享
- Python / MATLAB 外部程序与 CANoe 交互

**定义位置**：Environment → System Variables

**CAPL 访问语法**：
```capl
/* 读取 */
value = @Namespace::VariableName;

/* 写入 */
@Namespace::VariableName = newValue;
```

**示例**：
```capl
on sysvar Panel::EngineSpeedSlider
{
  int sliderVal = @Panel::EngineSpeedSlider;
  txMsg.EngineRPM = sliderVal * 40;   // 映射到物理值
  output(txMsg);
}
```

### 10.2 环境变量（Environment Variables，Legacy）
旧版 CANoe 使用，现已被 System Variables 取代。语法：
```capl
on envVar EngineSpeed
{
  int val = getValue(this);
  setValue(this, val + 100);
}
```

### 10.3 与 Python 的协同
```capl
/* CAPL 侧 */
on sysvar Python::TriggerSend
{
  if (@Python::TriggerSend == 1)
  {
    linFrame 0x05 f;
    f.byte(4) = @Python::DataByte;
    output(f);
    @Python::TriggerSend = 0;    // 复位，等待下次触发
  }
}
```

```python
# Python 侧
import win32com.client
canoe = win32com.client.Dispatch("CANoe.Application")
env = canoe.Configuration.Environment
sv = env.GetVariable("Python::TriggerSend")
sv.Value = 1   # 触发 CAPL 发送 LIN 帧
```

---

## 第11章 诊断协议编程

### 11.1 诊断对象声明
诊断编程依赖 **CDD**（CANdela）或 **ODX** 数据库文件。

```capl
variables
{
  /* Tester 端请求 */
  diagRequest Door.UDS_ReadDTC readDtcReq;

  /* ECU 端响应 */
  diagResponse Door.UDS_ReadDTC readDtcResp;
}
```

### 11.2 Tester 端：发送请求并处理响应
```capl
on key 'r'
{
  // 填充诊断参数
  readDtcReq.SetParameter("service", 0x19);
  readDtcReq.SetParameter("subFunction", 0x02);
  readDtcReq.SetParameter("DTCStatusMask", 0xFF);

  // 发送请求（自动处理 ISO-TP 分包）
  diagSendRequest(readDtcReq);
  write("已发送 ReadDTC 请求");
}

on diagResponse Door.UDS_ReadDTC
{
  // 响应到达
  byte count = this.GetParameter("DTCCount");
  write("收到 %d 个 DTC", count);

  // 遍历 DTC
  int i;
  for (i = 0; i < count; i++)
  {
    dword dtc = this.GetParameter("DTC", i);
    byte status = this.GetParameter("DTCStatus", i);
    write("  DTC[%d]: 0x%06X, Status: 0x%02X", i, dtc, status);
  }
}
```

### 11.3 ECU 仿真端：接收请求并构造响应
```capl
on diagRequest Door.UDS_ReadDTC
{
  // 读取请求参数
  byte subFunc = this.GetParameter("subFunction");
  write("收到 ReadDTC 请求, subFunction=0x%02X", subFunc);

  // 构造响应
  diagResponse Door.UDS_ReadDTC resp;
  resp.SetParameter("DTCCount", 2);
  resp.SetParameter("DTC", 0, 0xC1234);
  resp.SetParameter("DTCStatus", 0, 0x09);
  resp.SetParameter("DTC", 1, 0xC5678);
  resp.SetParameter("DTCStatus", 1, 0x08);

  // 发送响应
  diagSendResponse(resp);
}
```

### 11.4 诊断会话控制
```capl
on key '1'
{
  // 切换到 Default Session (10 01)
  diagRequest Door.UDS_SessionControl sessionReq;
  sessionReq.SetParameter("sessionType", 0x01);
  diagSendRequest(sessionReq);
}

on key '2'
{
  // 切换到 Extended Session (10 03)
  diagRequest Door.UDS_SessionControl sessionReq;
  sessionReq.SetParameter("sessionType", 0x03);
  diagSendRequest(sessionReq);
}
```

### 11.5 诊断超时处理
```capl
variables
{
  msTimer diagTimeout;
  int diagPending;
}

on diagRequest Door.UDS_ReadDTC
{
  diagPending = 1;
  setTimer(diagTimeout, 5000);    // 5秒超时
}

on diagResponse Door.UDS_ReadDTC
{
  diagPending = 0;
  cancelTimer(diagTimeout);
  write("诊断响应正常");
}

on msTimer diagTimeout
{
  if (diagPending)
  {
    write("诊断请求超时！");
    diagPending = 0;
  }
}
```

---

## 第12章 测试功能集 TFS

TFS（Test Feature Set）是 CAPL 的**测试自动化扩展**，提供阻塞式/等待式原语，用于编写可重复执行的测试序列。

### 12.1 测试用例结构
```capl
testcase CheckEngineRPM()
{
  // 1. 发送请求或触发条件
  txMsg.EngineRPM = 3000;
  output(txMsg);

  // 2. 等待响应（阻塞直到条件满足或超时）
  if (testWaitForMessage(0x123, 2000) == 1)
  {
    // 3. 验证信号
    if ($EngineRPM == 3000)
    {
      testStepPass("Engine RPM 验证通过");
    }
    else
    {
      testStepFail("Engine RPM 不匹配");
    }
  }
  else
  5.  {
    testStepFail("超时未收到响应报文");
  }
}
```

### 12.2 等待类函数
```capl
/* 等待指定报文到达 */
long result = testWaitForMessage(0x123, 5000);       // 超时 5000ms

/* 等待信号满足条件 */
testWaitForSignalMatch(EngineData::EngineRPM, 3000, 3000);

/* 等待信号在范围内 */
testWaitForSignalInRange(EngineData::EngineRPM, 2900, 3100, 3000);

/* 等待一段时间 */
testWait(1000);                                      // 等待 1000ms

/* 等待多个事件中的任意一个 */
long eventId1 = testWaitForMessage(0x123, 0);        // 0 = 不阻塞，仅注册
long eventId2 = testWaitForMessage(0x456, 0);
testWaitForAnyEvents(5000, 2, eventId1, eventId2); // 等待任意一个
```

### 12.3 验证类函数
```capl
testStepPass("描述");           // 记录通过步骤
testStepFail("描述");           // 记录失败步骤
testStepWait("描述", 1000);     // 等待步骤，带超时

testAssertEquals("RPM检查", $EngineRPM, 3000);       // 断言相等
testAssertInRange("温度范围", $Temp, 80, 90);       // 断言在范围内
```

### 12.4 故障注入
```capl
testcase BusOffTest()
{
  // 禁用某报文
  testDisableMsg(0x123);

  // 发送错误帧
  testInjectErrorFrame(CAN1, 10);   // 通道1注入10个错误帧

  // 等待一段时间
  testWait(5000);

  // 恢复报文
  testEnableMsg(0x123);

  // 验证总线恢复
  if (testWaitForMessage(0x123, 2000))
  {
    testStepPass("总线恢复成功");
  }
}
```

### 12.5 报告输出
```capl
testSupplyTextEvent("开始测试序列");
testWriteStepResultToReport(1, "步骤1", "通过");
testWriteStepResultToReport(0, "步骤2", "失败：信号不匹配");
```

---

## 第13章 函数与代码复用

### 13.1 自定义函数
```capl
/* 函数声明（必须在事件处理程序之前） */
byte calcXOR(byte data[], int len)
{
  byte result = 0;
  int i;
  for (i = 0; i < len; i++)
  {
    result ^= data[i];
  }
  return result;
}

void printMessageInfo(message *msg)   // 指针传递（引用）
{
  write("ID=0x%X, DLC=%d", msg->id, msg->dlc);
}

on message *
{
  printMessageInfo(this);             // 传递当前报文
}
```

### 13.2 头文件复用
**common.cin**：
```capl
/* 常量定义 */
const int ENGINE_ID = 0x123;
const int ABS_ID = 0x456;

/* 结构体 */
struct VehicleState
{
  int rpm;
  double speed;
  byte gear;
};

/* 公共函数 */
byte calcChecksum(byte data[], int len);
```

**node.can**：
```capl
includes
{
  #include "common.cin"
}

variables
{
  VehicleState gState;
}

on start
{
  gState.rpm = 0;
}
```

### 13.3 作用域规则
| 变量类型 | 声明位置 | 生命周期 | 可见范围 |
|----------|----------|----------|----------|
| 全局变量 | `variables` | Measurement 期间 | 整个节点 |
| 静态局部 | 函数内 `static` | Measurement 期间 | 仅函数内 |
| 普通局部 | 函数内 | 函数调用期间 | 仅函数内 |
| 事件参数 | `on message` 等 | 事件处理期间 | 事件体内 |

---

## 第14章 调试技术与优化

### 14.1 调试输出
```capl
write("普通文本");                          // 输出到 Write 窗口
write("格式化: %d, 0x%X, %f", 100, 255, 1.5); // 格式化输出

/* 格式化符号 */
// %d / %i : 十进制整数
// %x / %X : 十六进制
// %f      : 浮点
// %s      : 字符串
// %c      : 字符
// %ld     : 长整型
// %lu     : 无符号长整型
// %e      : 科学计数法
```

### 14.2 断点与单步
- 在 CAPL Browser 代码行左侧点击设置断点
- Measurement 运行时命中断点会自动暂停
- 支持单步执行（Step Over / Step Into）

### 14.3 性能优化原则
| 原则 | 说明 |
|------|------|
| **事件非阻塞** | `on *` 内禁止 `while(1)` 或长循环，会饿死调度器 |
| **避免频繁分配** | 报文对象在 `variables` 预声明，不要在事件内重复声明 |
| **减少浮点运算** | CAPL 浮点性能较低，尽量用整数运算 |
| **批量输出** | 高频报文场景下，`write()` 会显著拖慢性能，必要时关闭 |
| **数据库符号缓存** | 频繁访问 DBC 信号时，先缓存到局部变量 |

### 14.4 常见编译错误
| 错误 | 原因 | 解决 |
|------|------|------|
| `unknown symbol` | 使用了未声明的变量/函数 | 检查拼写，确认在 `variables` 或头文件中声明 |
| `type mismatch` | 赋值类型不匹配 | 检查变量类型与表达式类型 |
| `array index out of bounds` | 数组越界 | 检查索引范围 |
| `this not allowed here` | 在非报文事件中使用 `this` | `this` 仅在 `on message/linFrame` 等事件中有效 |
| `missing on handler` | 缺少事件处理程序 | 检查 `on` 关键字拼写 |

---

## 第15章 工程级综合示例

### 15.1 场景：车门控制网络仿真
**网络拓扑**：
- CAN1：动力总线（Engine, ABS）
- LIN1：车身总线（DoorLock, Window, Mirror）
- 网关：CAN-LIN 路由

**CAPL 节点设计**：

**Node: Gateway_CAN_LIN.can**
```capl
includes
{
  #include "gateway_defs.cin"
}

variables
{
  message 0x200 canDoorStatus;
  linFrame 0x05 linDoorCmd;
}

/* CAN → LIN 路由 */
on message 0x200                 // 收到 CAN 车门控制报文
{
  linDoorCmd.dlc = 4;
  linDoorCmd.byte(0) = this.byte(0);   // LockCmd
  linDoorCmd.byte(1) = this.byte(1);   // WindowCmd
  linDoorCmd.byte(2) = this.byte(2);   // MirrorCmd
  linDoorCmd.byte(3) = calcChecksum(linDoorCmd.byte, 3);

  outputToBus(linDoorCmd, "LIN1");
}

/* LIN → CAN 路由 */
on linFrame 0x05
{
  canDoorStatus.dlc = 8;
  canDoorStatus.byte(0) = this.byte(0);
  canDoorStatus.byte(1) = this.byte(1);
  canDoorStatus.byte(2) = this.byte(2);
  canDoorStatus.byte(3) = this.byte(3);
  canDoorStatus.channel = 1;           // 发送到 CAN1

  output(canDoorStatus);
}
```

**Node: LIN_Master_BCM.can**
```capl
variables
{
  linFrame 0x05 doorStatus;
  linFrame 0x10 windowStatus;
  msTimer linSchedule;
  int slotIndex;
}

on start
{
  slotIndex = 0;
  setTimer(linSchedule, 10);    // 10ms 调度粒度
}

on msTimer linSchedule
{
  switch(slotIndex)
  {
    case 0:
      doorStatus.dlc = 8;
      doorStatus.byte(4) = 0xF2;
      output(doorStatus);
      break;
    case 1:
      windowStatus.dlc = 4;
      output(windowStatus);
      break;
    case 2:
      // 保留槽位 / 诊断帧
      break;
  }

  slotIndex = (slotIndex + 1) % 3;
  setTimer(linSchedule, 10);
}
```

**Node: Panel_Controller.can**
```capl
/* 响应 Panel 控件，控制仿真状态 */
on sysvar Panel::DoorLockCmd
{
  if (@Panel::DoorLockCmd == 1)
  {
    write("用户点击：上锁");
    @System::DoorLocked = 1;
  }
  else
  {
    write("用户点击：解锁");
    @System::DoorLocked = 0;
  }
}

on sysvar Panel::EmergencyStop
{
  if (@Panel::EmergencyStop == 1)
  {
    cancelTimer(linSchedule);     // 停止 LIN 调度
    write("紧急停止！");
  }
}
```

---

## 附录 A：关键字速查表

| 关键字 | 类别 | 说明 |
|--------|------|------|
| `byte`, `word`, `dword`, `qword` | 类型 | 无符号整型 |
| `int`, `long`, `long long` | 类型 | 有符号整型 |
| `float`, `double` | 类型 | 浮点 |
| `char` | 类型 | 字符/字符串 |
| `message` | 类型 | CAN 报文对象 |
| `linFrame` | 类型 | LIN 帧对象 |
| `timer`, `msTimer` | 类型 | 定时器 |
| `struct` | 类型 | 结构体 |
| `const` | 修饰符 | 常量 |
| `static` | 修饰符 | 静态局部变量 |
| `if`, `else`, `switch`, `case`, `default` | 流程 | 条件分支 |
| `for`, `while`, `do` | 流程 | 循环 |
| `break`, `continue`, `return` | 流程 | 跳转 |
| `on` | 事件 | 事件处理程序声明 |
| `this` | 对象 | 当前事件关联对象 |
| `includes` | 预处理 | 头文件包含区段 |
| `variables` | 区段 | 全局变量声明区段 |
| `output()` | 函数 | 发送报文/帧 |
| `outputToBus()` | 函数 | 指定通道发送 |
| `setTimer()` | 函数 | 设置单次定时器 |
| `setTimerCyclic()` | 函数 | 设置循环定时器 |
| `cancelTimer()` | 函数 | 取消定时器 |
| `isTimerActive()` | 函数 | 查询定时器状态 |
| `write()` | 函数 | 调试输出 |
| `strlen()`, `strcpy()` | 函数 | 字符串操作 |
| `testWaitForMessage()` | TFS | 测试等待 |
| `testStepPass()` / `testStepFail()` | TFS | 测试结果记录 |
| `diagSendRequest()` | 诊断 | 发送诊断请求 |
| `diagSendResponse()` | 诊断 | 发送诊断响应 |
| `linChangeSched()` | LIN | 切换调度表 |
| `linWakeup()` / `linSleep()` | LIN | 唤醒/休眠 |

---

## 附录 B：事件优先级参考

CANoe 内核按以下优先级调度事件（高→低）：

1. `on preStart`
2. `on start`
3. `on key` / `on sysvar` / `on envVar`
4. `on timer` / `on msTimer`
5. `on message` / `on linFrame` / `on signal`
6. `on diagRequest` / `on diagResponse`
7. `on errorFrame`
8. `on stop`
9. `on postStop`

---

**文档结束**

*本手册基于 Vector CANoe 16.x-18.x 版本编写，部分语法在不同版本中可能存在细微差异，请以实际编译器为准。*
