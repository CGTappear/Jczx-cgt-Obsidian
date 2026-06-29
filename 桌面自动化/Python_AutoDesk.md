# 🖥️ Windows桌面自动化全攻略 | Python党必看干货合集

---

## 🎯 方案一：GUI 控件精准操控（最推荐）

适合操作有标准控件的 Windows 原生程序，比如按钮、输入框、菜单栏。

### 1️⃣ `pywinauto` —— Win32 老牌王者

- **核心技术**：Win32 API / MS UI Automation
    
- **适合**：操作传统 Windows 软件（如记事本、对话框、工具栏）
    
- **优点**：生态成熟，能深度操控控件属性
    
```python
from pywinauto import Application

app = Application(backend="uia").start("notepad.exe")
dlg = app.window(title="无标题 - 记事本")
dlg.type_keys("Hello World{ENTER}")
dlg.menu_select("文件->保存")
```

### 2️⃣ `uiautomation` —— 现代软件神器

- **核心技术**：微软官方 UIA 框架
    
- **适合**：WPF、UWP、QT 写的现代界面
    
- **优点**：跨进程能力强，能穿透很多"画"出来的控件
    
```python
import uiautomation as auto

calc = auto.WindowControl(searchDepth=1, Name="计算器")
calc.ButtonControl(Name="1").Click()
calc.ButtonControl(Name="等于").Click()
```

> 💡 **博主 Tips**：先用 Windows SDK 里的 `Inspect.exe` 看看软件控件树，再决定用 `win32` 还是 `uia` 后端！

---

## 🖱️ 方案二：键鼠模拟 + 图像识别（兜底方案）

当软件**完全封闭**，拿不到任何控件信息时，就用这套"笨办法"。

### 3️⃣ `pyautogui` —— 跨平台万能胶

- **核心技术**：屏幕坐标 + 键盘模拟
    
- **适合**：任何有图形界面的程序，甚至游戏
    
- **优点**：最简单，几行代码就能跑
    
- **缺点**：分辨率一变就翻车，窗口动了就点歪
    
```python
import pyautogui

pyautogui.click(100, 200)           # 绝对坐标点击
pyautogui.typewrite("Hello", interval=0.1)
pyautogui.hotkey('ctrl', 's')       # 快捷键

# 甚至能找图点击！🤯
loc = pyautogui.locateOnScreen('save_btn.png')
if loc:
    pyautogui.click(pyautogui.center(loc))
```

### 4️⃣ `SikuliX` / `lackey` —— 纯视觉驱动

- **核心技术**：OpenCV 图像识别
    
- **适合**：Flash、自定义绘制、完全无控件的界面
    
- **缺点**：速度慢，受界面主题/颜色影响大
    

> ⚠️ **避雷**：除非万不得已，**不要依赖坐标点击**！尽量走控件操控路线！

---

## 🔌 方案三：COM / OLE 自动化（专业软件首选）

很多专业软件（Office、CANoe、Outlook）都内置了 COM 接口，这是**最稳定**的操控方式。

### 5️⃣ `win32com.client` —— 操控一切 COM 软件

- **适合**：CANoe、Excel、Word、Outlook、AutoCAD
    
- **优点**：直接调用软件内部 API，比点鼠标稳一百倍
    
```python
import win32com.client

# 操控 CANoe
app = win32com.client.Dispatch("CANoe.Application")
app.Open(r"D:\Test.cfg")
app.Measurement.Start()

# 操控 Excel
excel = win32com.client.Dispatch("Excel.Application")
excel.Visible = True
excel.Workbooks.Add()
```

> 💡 **博主 Tips**：用 COM 前记得先 `canoe64.exe /regserver` 注册一下，不然 Python 找不到类！

---

## ⚙️ 方案四：系统级自动化（运维/监控向）

不操作界面，而是管理电脑本身。

表格

|需求|神器|代码片段|
|:--|:--|:--|
|查杀进程/看CPU|`psutil`|`psutil.process_iter()`|
|执行 CMD 命令|`subprocess`|`subprocess.run(["ipconfig"])`|
|监控文件变化|`watchdog`|自动触发脚本|
|操作注册表|`winreg`|标准库自带|
|剪贴板/通知|`pyperclip` / `win10toast`|复制粘贴一键搞定|

```python
import psutil

# 找到 CANoe 进程
for proc in psutil.process_iter(['pid', 'name']):
    if proc.info['name'] == 'CANoe64.exe':
        print(f"🔍 找到了！PID: {proc.info['pid']}")
```

---

## 🌐 方案五：浏览器自动化（Web 端）

虽然不算"桌面"，但经常和桌面自动化配合使用。

表格

|库|特点|推荐指数|
|:--|:--|:--|
|**Playwright**|微软出品，自动等待，速度快，能录屏生成代码|⭐⭐⭐⭐⭐|
|**Selenium**|老牌经典，资料最多|⭐⭐⭐⭐|
|**DrissionPage**|国产整合方案，同时支持浏览器和 requests|⭐⭐⭐⭐|


```python
# Playwright 示例
from playwright.sync_api import sync_playwright

with sync_playwright() as p:
    browser = p.chromium.launch()
    page = browser.new_page()
    page.goto("https://example.com")
    page.fill("input[name='q']", "Python自动化")
    page.click("button[type='submit']")
```

---

## ⏰ 方案六：定时任务调度

自动化脚本跑起来了，怎么定时触发？

表格

| 库              | 场景                      |
| :------------- | :---------------------- |
| `schedule`     | 简单定时，比如"每 10 分钟执行一次"    |
| `APScheduler`  | 企业级，支持 Cron 表达式、多线程、持久化 |
| Windows 任务计划程序 | 注册系统级任务，电脑重启也能跑         |

```python
import schedule, time

def job():
    print("⏰ 到点干活了！")

schedule.every(10).minutes.do(job)

while True:
    schedule.run_pending()
    time.sleep(1)
```

---

## 📊 一图看懂怎么选

|你的场景|首选方案|备选方案|
|:--|:--|:--|
|软件有 COM 接口（CANoe/Office）|`win32com.client`|—|
|能看到按钮/输入框（标准控件）|`pywinauto` / `uiautomation`|—|
|现代 WPF/QT 界面|`uiautomation`|`pywinauto` (uia后端)|
|完全封闭，无控件|`pyautogui` + 找图|`SikuliX`|
|系统运维监控|`psutil` + `watchdog`|—|
|浏览器操作|`Playwright`|`DrissionPage`|
|定时调度|`APScheduler`|`schedule`|

---

## 🚨 避雷指南（血泪经验）

1. **能用 API 绝不点鼠标** 🖱️❌  
    COM 接口 > 控件操控 > 坐标点击，稳定性天差地别！
    
2. **管理员权限很重要** 🔑  
    操控其他进程或发系统按键时，右键"以管理员运行"你的 Python！
    
3. **别写死坐标** 📐  
    分辨率一变，脚本全废。尽量用控件名称定位。
    
4. **调试神器 Inspect.exe** 🔍  
    在 Windows SDK 里，免费！能看到软件所有控件的属性和树结构。
    

---

## 📝 写在最后

- 操控 **CANoe** → 走 `win32com` + CAPL 函数
    
- 操控 **老旧工控软件** → `pywinauto` 或 `uiautomation`
    
- **完全没接口的封闭软件** → 只能 `pyautogui` 硬点 + 图像识别兜底