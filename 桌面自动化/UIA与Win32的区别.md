## 一、先纠正一个常见误区

> **"Win32 直接调用内核 API" ❌**


- **内核 API**（如 `NtCreateFile`、`ZwReadFile`）运行在 **Ring 0**（内核态），普通应用程序无法直接调用。
    
- **Win32 API**（如 `FindWindow`、`SendMessage`、`GetWindowText`）运行在 **Ring 3**（用户态），封装在 `user32.dll`、`kernel32.dll` 中。它们最终**可能**通过系统调用陷入内核，但本身不是内核 API。
    

所以 `pywinauto` 的 `win32` backend 操作的是**传统用户层原生窗口接口**，而非内核。

---

## 二、`win32` backend 到底是什么？

`pywinauto` 的 `win32` backend 底层走的是 **Win32 API + MSAA**（Microsoft Active Accessibility）：

|技术栈|说明|
|:--|:--|
|**核心 DLL**|`user32.dll`（窗口消息）、`oleacc.dll`（MSAA 无障碍）|
|**获取控件**|通过窗口句柄（`HWND`）、窗口类名（`ClassName`）、控件 ID|
|**操控方式**|发送 Windows 消息（`WM_CLICK`、`BM_CLICK`、`WM_SETTEXT`）|
|**诞生年代**|1990s（Windows 3.1/95 时代）|

**本质**：像"黑客"一样给目标窗口**发消息**或**读内存**，模拟人点击按钮的过程。

---

## 三、`uia` backend 到底是什么？

`uia` backend 走的是 **Microsoft UI Automation**（UIA）：

| 技术栈        | 说明                                                                          |
| :--------- | :-------------------------------------------------------------------------- |
| **核心 DLL** | `UIAutomationCore.dll`                                                      |
| **获取控件**   | 通过 COM 接口遍历 UI 树，用 `ControlType`、`Name`、`AutomationId` 定位                   |
| **操控方式**   | 调用控件暴露的 **Pattern 接口**（如 `InvokePattern.Click()`、`ValuePattern.SetValue()`） |
| **诞生年代**   | 2006+（随 .NET 3.0 / Vista 引入）                                                |

**本质**：像"外交官"一样，通过标准化的 COM 协议让目标程序**主动配合**完成操作。

---

## 四、核心区别对比


|维度|`win32` backend|`uia` backend|
|:--|:--|:--|
|**技术层次**|底层窗口消息（HWND）|高层 COM 自动化接口|
|**直接调内核？**|❌ 用户态 API|❌ 用户态 COM|
|**控件识别能力**|只能识别标准 Win32 控件（Button、Edit、Static）|能识别 WPF、UWP、Qt、Modern UI、WebView 等自绘控件|
|**跨进程稳定性**|差（依赖窗口句柄，程序重启后 HWND 变）|强（依赖逻辑属性如 `AutomationId`，与进程生命周期解耦）|
|**现代软件支持**|❌ 很多新软件识别为 `Pane`|✅ 能穿透自定义绘制|
|**获取控件信息**|有限（类名、标题、坐标）|丰富（Name、HelpText、ToggleState、Value 等）|
|**性能**|快（直接发消息）|稍慢（COM 遍历 + 跨进程 RPC）|
|**管理员权限要求**|高（跨进程发消息常被 UIPI 拦截）|低（标准无障碍接口，通常无需提权）|
|**Inspect 表现**|很多控件显示为 `Pane` 或识别不到|能显示 `UIA_ButtonControlTypeId` 等精确类型|

---

## 五、在 pywinauto 中的实际差异

### 连接同一个计算器：


```python
# win32 backend：只能看到传统控件
app = Application(backend="win32").connect(title="计算器")
dlg = app.window(title="计算器")
# 可能只能定位到主窗口，里面的按钮全是 "Button" 类，靠文字区分

# uia backend：能看到完整的现代控件树
app = Application(backend="uia").connect(title="计算器")
dlg = app.window(title="计算器")
# 可以直接用 control_type="Button", name="1" 精准定位
```

### 对自定义控件的差异：

表格

|场景|win32|uia|
|:--|:--|:--|
|CANoe 的 FuncTest 面板按钮|可能识别为 `Pane` 或 `AfxWnd`|显示 `UIA_ButtonControlTypeId`，可用|
|WPF 程序的自绘按钮|❌ 完全看不到|✅ 显示为 `Button`|
|浏览器内核界面（CEF/WebView）|❌ 只能看到一个大窗口|✅ 能看到内部 HTML 控件|

---

## 六、一句话总结

> **Win32 backend** 像"用望远镜+喊话"操控窗口（老派、直接、对现代软件失明）；  
> **UIA backend** 像"用雷达+协议通话"操控界面（现代、标准、能看透自绘控件）。

你截图里看到 `UIA_ButtonControlTypeId`，说明目标程序已经向系统注册了 UIA 接口，**无脑选 `uia` 即可**，`win32` 大概率识别不到或只能看到一个没有结构的壳子。