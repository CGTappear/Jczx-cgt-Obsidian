uv语法完全上帝视角

我会按“上帝视角”把 uv 当成 Python 项目/环境/包管理的一整套系统来讲：先给总图，再按常用场景给命令。

已思考 5s

下面给你一份 **uv 语法“上帝视角”总览**。可以把 uv 理解成 Python 世界里的：

> **pip + venv + pyenv + pip-tools + pipx + poetry 的统一替代方案**

官方定位是 **Python 包管理器与项目管理器**，可以管理 Python 版本、虚拟环境、依赖、锁文件、项目运行、工具运行和打包发布。

---

# uv 的核心命令总图

```
uv init        # 创建 Python 项目
uv add         # 添加项目依赖
uv remove      # 删除项目依赖
uv sync        # 根据 pyproject.toml / uv.lock 同步环境
uv lock        # 生成 / 更新 uv.lock
uv run         # 在项目环境中运行命令或脚本
uv python find    # 查看uv使用python
uv python      # 管理 Python 版本
uv venv        # 创建虚拟环境
uv pip         # pip 兼容模式
uv tool        # 管理全局命令行工具，类似 pipx
uvx            # 临时运行 Python 工具，等价于 uv tool run
uv build       # 构建 Python 包
uv publish     # 发布 Python 包
uv cache       # 管理缓存
```

# uv python默认安装位置

方法一


| 平台              | 默认路径                                                                  |
| --------------- | --------------------------------------------------------------------- |
| **Windows**     | `%APPDATA%\uv\python`（通常是 `C:\Users\<用户名>\AppData\Roaming\uv\python`） |
| **macOS/Linux** | `~/.local/share/uv/python`                                            |
|            |                                                                  |


注：**硬编码在代码中**的，但可以通过环境变量覆盖

```
// 伪代码示意
fn get_python_install_dir() -> PathBuf {
    // 1. 优先检查环境变量
    if let Ok(dir) = env::var("UV_PYTHON_INSTALL_DIR") {
        return PathBuf::from(dir);
    }
    
    // 2. 检查配置文件
    if let Some(config) = read_config() {
        if let Some(dir) = config.python_install_dir {
            return dir;
        }
    }
    
    // 3. 使用硬编码默认值
    default_python_install_dir()  // ← 这里硬编码
}

fn default_python_install_dir() -> PathBuf {
    #[cfg(windows)]
    {
        // Windows: %APPDATA%\uv\python
        dirs::data_dir().unwrap().join("uv").join("python")
    }
    
    #[cfg(unix)]
    {
        // Linux/macOS: ~/.local/share/uv/python
        dirs::data_local_dir().unwrap().join("uv").join("python")
    }
}
```

## 命令查看

```
uv python dir              # 查看 Python 安装目录
uv python dir --bin        # 查看 Python 可执行文件目录
```

## 修改安装位置

设置 `UV_PYTHON_INSTALL_DIR` 环境变量：**Windows (PowerShell)**

```
# 临时（当前会话）
$env:UV_PYTHON_INSTALL_DIR = "D:\uv\python"

# 永久（用户级别）
[Environment]::SetEnvironmentVariable("UV_PYTHON_INSTALL_DIR", "D:\uv\python", "User")
```

Windows CMD

```
set UV_PYTHON_INSTALL_DIR=D:\uv\python
```

方法二

命令行参数（单次生效）

```
uv python install 3.12 --install-dir D:\uv\python
```

其他相关目录


| 目录类型         | 查看命令                  | 修改环境变量                  |
| ------------ | --------------------- | ----------------------- |
| Python 安装目录  | `uv python dir`       | `UV_PYTHON_INSTALL_DIR` |
| Python 可执行文件 | `uv python dir --bin` | `UV_PYTHON_BIN_DIR`     |
| 缓存目录         | `uv cache dir`        | `UV_CACHE_DIR`          |
| 工具安装目录       | `uv tool dir`         | `UV_TOOL_DIR`           |
| 工具可执行文件      | `uv tool dir --bin`   | `UV_TOOL_BIN_DIR`       |


## **重要注意事项**

1. **修改后需重新创建虚拟环境**：已存在的 `.venv` 仍指向旧位置，需要删除后重新 `uv sync` 或 `uv venv`
2. **如果改了** `UV_PYTHON_INSTALL_DIR`**，后续操作也要设置该变量**，否则 `uv` 找不到已安装的 Python 版本
3. **uv 0.8.0+ 新增了** `python-install-dir` **配置项**，可以在 `pyproject.toml` 或 `uv.toml` 中配置，避免每次手动设置环境变量

```
# pyproject.toml
[tool.uv]
python-install-dir = "D:/uv/python"
```

# 如何进行uv python切换？

1. 项目级别切换（推荐）

通过 `.python-version` 文件控制，进入项目目录自动生效：

```
# 查看当前项目使用的 Python
cat .python-version

# 切换到指定版本（自动下载安装，如果不存在）
uv python pin 3.12

# 或手动修改文件
echo "3.11" > .python-version
```

然后同步环境：

```
uv sync        # 根据 .python-version 重新创建虚拟环境
```

2. 全局默认版本切换

```
# 查看所有已安装的 Python
uv python list

# 设置全局默认 Python（类似 pyenv global）
uv python install 3.12
uv python pin --global 3.12

# 或创建符号链接（Windows 需管理员权限）
uv python find 3.12          # 查看 3.12 的路径
```

3、临时切换

```
# 用指定 Python 版本运行脚本（不修改项目配置）
uv run --python 3.11 python main.py

# 用指定 Python 创建虚拟环境
uv venv --python 3.11

# 安装依赖时指定 Python
uv pip install --python 3.11 requests
```

4、虚拟环境级别切换

```
# 删除旧环境，用新版本重建
rm -rf .venv                    # Linux/macOS
# rmdir /s /q .venv            # Windows

uv venv --python 3.12          # 用 3.12 创建新环境
uv sync                        # 安装依赖
```

5、查看当前使用的python

```
uv python find                 # 当前项目使用的 Python 路径
uv python find --system        # 系统 Python 路径
uv run python --version        # 查看虚拟环境中的 Python 版本
```

&nbsp;

## uv 常用命令速查表

### 系统级命令

```bash
# 查看 uv 版本
uv --version

# 更新 uv 自身
uv self update

# 查看帮助
uv --help
uv tool --help
uv python --help
uv cache --help
```

```bash
# 查看 uv 缓存目录
uv cache dir

# 清空所有缓存
uv cache clean

# 清理无用缓存
uv cache prune
```

```bash
# 查看可安装的 Python 版本
uv python list

# 安装指定 Python 版本
uv python install 3.12
uv python install 3.11 3.12

# 查找当前可用 Python
uv python find

# 查看 uv 管理的 Python 安装目录
uv python dir
```

### 全局工具命令

```bash
# 安装全局工具
uv tool install ruff
uv tool install black
uv tool install pytest
```

```bash
# 临时运行工具，不需要提前安装
uvx ruff check .
uvx black .
uvx pytest
```

```bash
# 查看已安装工具
uv tool list

# 升级指定工具
uv tool upgrade ruff

# 升级全部工具
uv tool upgrade --all

# 卸载工具
uv tool uninstall ruff
```

## 项目级命令

### 初始化项目

```bash
# 初始化普通项目
uv init

# 初始化应用项目
uv init --app

# 初始化库项目
uv init --lib
```

### 虚拟环境管理

```bash
# 创建默认虚拟环境
uv venv

# 创建指定名称的虚拟环境
uv venv .venv

# 使用指定 Python 版本创建虚拟环境
uv venv --python 3.12
```

### Python 版本管理

```bash
# 为当前项目固定 Python 版本
uv python pin 3.12

# 切换项目 Python 版本
uv python pin 3.11

# 取消项目 Python 版本固定
uv python unpin
```

### 依赖管理

```bash
# 添加普通依赖
uv add requests
uv add fastapi uvicorn

# 添加开发依赖
uv add --dev pytest ruff black

# 添加指定版本依赖
uv add "django>=5.0"
uv add "requests==2.32.0"
```

```bash
# 添加依赖组
uv add --group docs mkdocs
uv add --group test pytest
uv add --group lint ruff black
```

```bash
# 删除依赖
uv remove requests

# 删除指定依赖组中的依赖
uv remove --group test pytest
```

### 同步环境

```bash
# 根据 pyproject.toml 和 uv.lock 同步环境
uv sync

# 同步开发依赖
uv sync --dev

# 同步指定依赖组
uv sync --group docs

# 严格使用现有锁文件，不重新解析
uv sync --frozen
```

### 运行命令

```bash
# 运行 Python 文件
uv run python main.py

# 运行测试
uv run pytest

# 运行代码检查
uv run ruff check .

# 运行格式化
uv run black .
```

### 锁文件管理

```bash
# 生成或更新 uv.lock
uv lock

# 升级所有依赖并重新锁定
uv lock --upgrade

# 只升级指定包
uv lock --upgrade-package requests
```

### 查看依赖

```bash
# 查看依赖树
uv tree

# 查看指定依赖组的依赖树
uv tree --group test
```

## uv pip 兼容命令

`uv pip` 可以看作是更快的 `pip` 替代品，适合迁移老项目或处理 `requirements.txt`。

```bash
# 从 requirements.txt 安装依赖
uv pip install -r requirements.txt

# 安装当前项目为可编辑模式
uv pip install -e .

# 安装单个包
uv pip install requests

# 卸载包
uv pip uninstall requests

# 查看已安装包
uv pip list

# 导出当前环境依赖
uv pip freeze

# 检查依赖冲突
uv pip check
```

## 系统级 vs 项目级对照表

| 场景          | 推荐命令                     |     |
| ----------- | ------------------------ | --- |
| 查看 uv 版本    | `uv --version`           |     |
| 更新 uv       | `uv self update`         |     |
| 安装 Python   | `uv python install 3.12` |     |
| 固定项目 Python | `uv python pin 3.12`     |     |
| 初始化项目       | `uv init`                |     |
| 创建虚拟环境      | `uv venv`                |     |
| 添加项目依赖      | `uv add requests`        |     |
| 添加开发依赖      | `uv add --dev pytest`    |     |
| 删除依赖        | `uv remove requests`     |     |
| 同步环境        | `uv sync`                |     |
| 运行项目命令      | `uv run python main.py`  |     |
| 锁定依赖        | `uv lock`                |     |
| 查看依赖树       | `uv tree`                |     |
| 安装全局工具      | `uv tool install ruff`   |     |
| 临时运行工具      | `uvx ruff check .`       |     |
| 清理缓存        | `uv cache prune`         |     |

## 推荐日常工作流

### 新项目

```bash
uv init
uv python pin 3.12
uv add requests
uv add --dev pytest ruff
uv run python main.py
```

### 老项目迁移

```bash
uv venv
uv pip install -r requirements.txt
uv pip check
```

### 团队协作

```bash
uv sync --frozen
uv run pytest
uv run ruff check .
```

### 更新依赖

```bash
uv lock --upgrade
uv sync
uv run pytest
```

## uv 系统级原理与 Python 控制模型

这一章重点理解 `uv` 到底在控制什么。不要只把 `uv` 理解成更快的 `pip`，它更像一个统一入口：同时管理 Python 解释器、虚拟环境、项目依赖、锁文件、全局命令行工具和包缓存。

### 1. uv 的分层模型

`uv` 的工作可以拆成几层：

| 层级 | 负责内容 | 常见文件 / 命令 |
|---|---|---|
| uv 可执行文件层 | 让系统能找到并运行 `uv`、`uvx` | `uv --version`、`uv self update`、`PATH` |
| Python 解释器层 | 查找、下载、安装、选择 Python | `uv python install`、`uv python find`、`.python-version` |
| 项目环境层 | 为项目创建独立虚拟环境 | `.venv`、`uv venv`、`uv sync`、`UV_PROJECT_ENVIRONMENT` |
| 依赖解析层 | 根据声明解析版本并生成锁文件 | `pyproject.toml`、`uv.lock`、`uv lock` |
| 安装执行层 | 把依赖安装进目标环境 | `uv sync`、`uv pip install` |
| 工具层 | 像 `pipx` 一样安装 / 临时运行命令行工具 | `uv tool install`、`uvx` |
| 缓存层 | 复用 wheel、源码包、构建结果、脚本环境 | `uv cache dir`、`uv cache clean` |

核心原则：

```text
uv 不会神秘地“切换系统 Python”。
uv 是在每次命令执行时，根据配置、参数和环境变量，选择一个 Python 解释器，然后创建或使用对应的环境。
```

所以排查 uv 问题时，要先问三件事：

```text
1. 当前运行的是哪个 uv？
2. uv 找到的是哪个 Python？
3. 当前命令要修改的是哪个环境？
```

### 2. uv 安装路径与调用链

在 Windows 上查看当前调用的是哪个 `uv`：

```powershell
Get-Command uv
Get-Command uvx
where.exe uv
where.exe uvx
```

在 macOS / Linux 上查看：

```bash
command -v uv
command -v uvx
which -a uv
which -a uvx
```

查看 `PATH` 的搜索顺序：

```powershell
# Windows PowerShell
$env:Path -split ';'
```

```bash
# macOS / Linux
echo "$PATH" | tr ':' '\n'
```

调用链可以理解为：

```text
终端输入 uv
  -> shell 按 PATH 顺序查找 uv.exe / uv
  -> 找到第一个匹配的 uv 可执行文件
  -> uv 再根据当前目录、配置文件、环境变量和命令参数决定后续行为
```

官方独立安装器默认会把 `uv`、`uvx` 安装到用户级可执行文件目录。常见路径：

| 系统 | 常见 uv 可执行文件目录 |
|---|---|
| Windows | `%USERPROFILE%\.local\bin` |
| macOS / Linux | `~/.local/bin` |

如果 PATH 中有多个 `uv`，以排在前面的为准。常见冲突来源：

```text
1. 官方安装器安装的 uv
2. pip / pipx 安装的 uv
3. Homebrew / Scoop / Winget 安装的 uv
4. 旧版 uv 遗留在其他目录
```

指定 uv 自身安装目录：

```powershell
# Windows：安装到指定目录
$env:UV_INSTALL_DIR = "$HOME\.local\bin"
powershell -ExecutionPolicy ByPass -c "irm https://astral.sh/uv/install.ps1 | iex"
```

```bash
# macOS / Linux：安装到指定目录
export UV_INSTALL_DIR="$HOME/.local/bin"
curl -LsSf https://astral.sh/uv/install.sh | sh
```

禁止安装器或 `uv self update` 自动修改 PATH：

```powershell
$env:UV_NO_MODIFY_PATH = "1"
uv self update
```

```bash
UV_NO_MODIFY_PATH=1 uv self update
```

### 3. uv 的系统级存储目录

uv 把不同类型的数据分开存放。理解这些路径后，就知道“删缓存”和“删 Python”不是一回事。

| 数据类型           | Windows 常见默认位置             | macOS / Linux 常见默认位置       | 查看 / 覆盖方式                                   |
| -------------- | -------------------------- | -------------------------- | ------------------------------------------- |
| uv / uvx 可执行文件 | `%USERPROFILE%\.local\bin` | `~/.local/bin`             | `UV_INSTALL_DIR`                            |
| 依赖缓存           | `%LOCALAPPDATA%\uv\cache`  | `~/.cache/uv`              | `uv cache dir` / `UV_CACHE_DIR`             |
| uv 持久数据根目录     | `%APPDATA%\uv\data`        | `~/.local/share/uv`        | 由系统数据目录决定                                   |
| uv 管理的 Python  | `%APPDATA%\uv\data\python` | `~/.local/share/uv/python` | `uv python dir` / `UV_PYTHON_INSTALL_DIR`   |
| Python 可执行入口   | `%USERPROFILE%\.local\bin` | `~/.local/bin`             | `uv python dir --bin` / `UV_PYTHON_BIN_DIR` |
| 全局工具环境         | `%APPDATA%\uv\data\tools`  | `~/.local/share/uv/tools`  | `uv tool dir` / `UV_TOOL_DIR`               |
| 全局工具入口         | `%USERPROFILE%\.local\bin` | `~/.local/bin`             | `uv tool dir --bin` / `UV_TOOL_BIN_DIR`     |
| 项目虚拟环境         | 项目根目录 `.venv`              | 项目根目录 `.venv`              | `UV_PROJECT_ENVIRONMENT`                    |

查看当前机器上的实际路径：

```bash
uv cache dir
uv python dir
uv python dir --bin
uv tool dir
uv tool dir --bin
```

清理缓存不会删除已安装的 Python 和工具：

```bash
# 只清理缓存
uv cache prune
uv cache clean
```

如果要彻底卸载 uv 相关数据，需要分别处理：

```text
1. uv / uvx 可执行文件
2. uv cache 缓存目录
3. uv managed Python 目录
4. uv tool 工具目录
5. 各项目自己的 .venv
```

### 4. managed Python 与 system Python

uv 把 Python 分成两类：

| 类型             | 含义                                                                                 |
| -------------- | ---------------------------------------------------------------------------------- |
| managed Python | 由 uv 下载、安装、升级和管理的 Python                                                           |
| system Python  | 不是 uv 安装的 Python，包括系统 Python、python.org、pyenv、Conda、Microsoft Store、其他工具安装的 Python |
![[uv命令-1782719132864.webp]]

| UV INIT --help                     | 原因                                                                                                                                                                                                             |
| ---------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| uv init -p --managed-python test02 | --这条命令会报错，因为你把 `-p` 当成了“项目/路径”的缩写，但在 `uv init` 里，会被uv理解为uv init --python <某个Python版本> --managed-python test02；但在 `-p` 后面没有给 Python 版本，所以它报错“a value is required for '--python <PYTHON>' but none was supplied“ |
| 正确写法01                             | uv init --managed-python test02                                                                                                                                                                                |
| 正确写法02                             | uv init --python 3.12 --managed-python test02  或 uv init -p 3.12 --managed-python test02   注：--managed-python  表示优先使用 uv 管理/安装的 Python，而不是系统里乱七八糟的 Python                                                      |





安装 uv 管理的 Python：

```bash
uv python install 3.12
uv python install 3.11 3.12
uv python install ">=3.10,<3.13"
uv python install pypy
```

查看已安装和可下载的 Python：

```bash
uv python list
uv python list 3.12
uv python list --only-installed
uv python list --all-versions
uv python list --all-platforms
```

查找 uv 实际会使用哪个 Python：

```bash
uv python find
uv python find 3.12
uv python find ">=3.11"
uv python find --system
```

升级 uv 管理的 Python：

```bash
# 升级某个 minor 版本到最新 patch
uv python upgrade 3.12

# 升级所有 uv 管理的 Python
uv python upgrade
```

注意：

```text
uv python upgrade 只支持 uv managed Python。
uv 支持透明升级 patch 版本，例如 3.12.7 -> 3.12.8。
uv 不会透明升级 minor 版本，例如 3.12 -> 3.13，因为这会影响依赖解析。
```

控制是否允许 uv 自动下载 Python：

```bash
# 当前命令禁止自动下载 Python
uv venv --python 3.12 --no-python-downloads

# 持久配置思路：只允许 uv python install 下载 Python
uv config set python-downloads manual
```

也可以用环境变量：

```powershell
$env:UV_PYTHON_DOWNLOADS = "manual"
```

```bash
export UV_PYTHON_DOWNLOADS=manual
```

控制优先使用 managed 还是 system Python：

```bash
# 只看 / 只用 uv managed Python
uv python list --managed-python

# 只看 / 只用 system Python
uv python list --no-managed-python
```

环境变量写法：

```powershell
# 只使用 uv 管理的 Python
$env:UV_PYTHON_PREFERENCE = "only-managed"

# 优先使用系统已有 Python
$env:UV_PYTHON_PREFERENCE = "system"

# 只使用系统 Python
$env:UV_PYTHON_PREFERENCE = "only-system"
```

### 5. uv 的 Python 选择顺序

实际使用哪个 Python，取决于命令类型。一般可以按下面理解：

```text
命令行 --python
  > 环境变量 UV_PYTHON
  > 当前项目或上级目录的 .python-version
  > pyproject.toml 的 requires-python
  > uv 的 Python 发现规则
```

`.python-version` 是“默认 Python 请求文件”，通常由下面命令生成：

```bash
# 在当前目录固定 Python 版本
uv python pin 3.12

# 全局固定默认 Python 版本
uv python pin --global 3.12

# 取消当前目录 pin
uv python unpin
```

`pyproject.toml` 里的 `requires-python` 是“项目支持的 Python 范围”：

```toml
[project]
name = "example"
version = "0.1.0"
requires-python = ">=3.12"
dependencies = []
```

两者区别：

| 配置 | 作用 |
|---|---|
| `.python-version` | 告诉 uv 这个目录默认请求哪个 Python |
| `requires-python` | 告诉 uv 项目支持哪些 Python 版本，并影响依赖解析 |
| `uv.lock` | 锁定依赖解析结果，不是 Python 安装目录 |
| `.venv` | 当前项目实际使用的虚拟环境 |

常见切换流程：

```bash
# 1. 安装目标 Python
uv python install 3.12

# 2. 在项目中固定版本
uv python pin 3.12

# 3. 同步环境
uv sync

# 4. 验证项目实际 Python
uv run python -V
uv run python -c "import sys; print(sys.executable)"
```

如果 `.venv` 已经用旧 Python 创建，`uv python pin` 本身只会改 `.python-version`，不等于强制重建旧虚拟环境。最稳的方式是备份或删除旧 `.venv` 后重新同步：

```powershell
# Windows PowerShell：确认不需要旧环境后再执行
Remove-Item -Recurse -Force .venv
uv sync
```

```bash
# macOS / Linux：确认不需要旧环境后再执行
rm -rf .venv
uv sync
```

### 6. 项目级 Python 与虚拟环境控制

项目级 uv 的核心文件：

```text
pyproject.toml   # 项目元数据、依赖、requires-python、tool.uv 配置
uv.lock          # 解析后的锁文件
.python-version  # 当前目录默认 Python 请求
.venv/           # 项目虚拟环境
```

初始化项目后，常见路径结构：

```text
my-project/
  pyproject.toml
  uv.lock
  .python-version
  .venv/
  src/ 或 main.py
```

项目环境默认创建在项目根目录的 `.venv`。如果要改位置：

```powershell
# 相对路径：相对于 workspace / 项目根目录
$env:UV_PROJECT_ENVIRONMENT = ".venv312"
uv sync
```

```bash
UV_PROJECT_ENVIRONMENT=.venv312 uv sync
```

绝对路径也可以，但要谨慎：

```powershell
$env:UV_PROJECT_ENVIRONMENT = "D:\venvs\my-project"
uv sync
```

注意：

```text
如果多个项目共用同一个绝对 UV_PROJECT_ENVIRONMENT，
后执行 uv sync 的项目可能覆盖前一个项目的环境。
因此绝对路径更适合 CI / Docker / 单项目专用环境。
```

### 7. uv run、激活环境与 VIRTUAL_ENV

使用 uv 时，通常不需要手动激活虚拟环境：

```bash
uv run python main.py
uv run pytest
uv run python -c "import sys; print(sys.executable)"
```

`uv run` 会把命令放进项目环境里执行。也就是说：

```text
uv run python
  -> 使用当前项目解析出的 .venv
  -> 临时把 .venv 的可执行目录放到命令搜索路径前面
  -> 执行 python
```

手动激活仍然可以：

```powershell
# Windows PowerShell
.venv\Scripts\activate
python -V
deactivate
```

```bash
# macOS / Linux
source .venv/bin/activate
python -V
deactivate
```

但要理解差异：

| 方式 | 特点 |
|---|---|
| `uv run ...` | 不依赖当前 shell 是否激活环境，推荐用于项目命令 |
| 激活 `.venv` 后运行 | 适合交互式调试，但容易受当前 shell 状态影响 |
| `uv pip ...` | 更接近 pip 的环境修改模型，默认要求虚拟环境 |

项目命令默认不会直接信任外部 `VIRTUAL_ENV`。如果当前 shell 激活了别的虚拟环境，而项目有自己的 `.venv`，uv 通常会优先使用项目环境，并给出提示。需要强制使用已激活环境时：

```bash
uv run --active python -V
```

不想看到 active venv 提示时：

```bash
uv run --no-active python -V
```

`uv pip` 的环境发现更像 pip，修改环境时会按顺序查找：

```text
1. VIRTUAL_ENV 指向的已激活虚拟环境
2. CONDA_PREFIX 指向的 Conda 环境
3. 当前目录或父目录中的 .venv
4. 如果找不到，提示创建 uv venv
```

修改系统 Python 需要显式确认：

```bash
# 安装到系统 Python，适合 CI / 容器，日常不推荐
uv pip install --system requests
```

### 8. 全局工具与 uvx 的原理

`uv tool install` 管的是“命令行工具”，不是当前项目依赖。

```bash
uv tool install ruff
uv tool install black
uv tool list
uv tool dir
uv tool dir --bin
```

它的原理：

```text
1. uv 为工具创建独立环境
2. 把工具包和依赖装进去
3. 在 tool bin 目录生成可执行入口
4. 你的 shell 通过 PATH 找到这个入口
```

`uvx` 是临时运行工具，等价于 `uv tool run`：

```bash
uvx ruff check .
uv tool run ruff check .
```

两者区别：

| 命令 | 用法 |
|---|---|
| `uv tool install ruff` | 长期安装，之后直接运行 `ruff` |
| `uvx ruff check .` | 临时运行，适合偶尔使用 |
| `uv run ruff check .` | 运行项目环境里的 `ruff`，适合项目 dev dependency |

推荐判断：

```text
项目内固定版本：uv add --dev ruff，然后 uv run ruff check .
个人全局工具：uv tool install ruff，然后 ruff check .
临时试用工具：uvx ruff check .
```

### 9. 常用环境变量速查

路径与目录：

| 环境变量 | 作用 |
|---|---|
| `UV_INSTALL_DIR` | uv / uvx 自身安装目录 |
| `UV_NO_MODIFY_PATH` | 禁止安装器或 self update 修改 PATH |
| `UV_CACHE_DIR` | 依赖缓存目录 |
| `UV_PYTHON_INSTALL_DIR` | uv managed Python 安装目录 |
| `UV_PYTHON_BIN_DIR` | managed Python 可执行入口目录 |
| `UV_TOOL_DIR` | 全局工具环境目录 |
| `UV_TOOL_BIN_DIR` | 全局工具可执行入口目录 |
| `UV_PROJECT_ENVIRONMENT` | 项目虚拟环境路径 |

Python 选择：

| 环境变量 | 作用 |
|---|---|
| `UV_PYTHON` | 等价于 `--python`，指定 Python 请求或解释器路径 |
| `UV_PYTHON_DOWNLOADS` | 控制是否允许自动下载 Python |
| `UV_PYTHON_PREFERENCE` | 控制优先使用 managed 还是 system Python |
| `UV_MANAGED_PYTHON` | 要求使用 uv managed Python |
| `UV_NO_MANAGED_PYTHON` | 禁止使用 uv managed Python |
| `UV_SYSTEM_PYTHON` | 等价于 `--system`，使用 PATH 中第一个系统 Python，谨慎使用 |
| `UV_PYTHON_SEARCH_PATH` | 覆盖用于发现 Python 的 PATH |
| `UV_PYTHON_NO_REGISTRY` | Windows 上禁用注册表 / Microsoft Store Python 发现与注册 |

项目与配置：

| 环境变量 | 作用 |
|---|---|
| `UV_PROJECT` | 等价于 `--project`，指定项目目录 |
| `UV_WORKING_DIR` | 等价于 `--directory`，指定工作目录 |
| `UV_CONFIG_FILE` | 指定某个 `uv.toml` 配置文件 |
| `UV_NO_CONFIG` | 禁止读取配置文件 |
| `UV_NO_SYSTEM_CONFIG` | 禁止读取系统级配置 |
| `UV_FROZEN` | 等价于 `--frozen`，不更新锁文件 |
| `UV_LOCKED` | 要求锁文件不变 |
| `UV_NO_SYNC` | 跳过环境同步 |
| `UV_ENV_FILE` | 指定 `uv run` 读取的 `.env` 文件 |
| `UV_NO_ENV_FILE` | 禁止读取 `.env` 文件 |

包源与网络：

| 环境变量 | 作用 |
|---|---|
| `UV_DEFAULT_INDEX` | 默认包索引地址，替代旧的 `UV_INDEX_URL` |
| `UV_INDEX` | 附加包索引地址列表 |
| `UV_INDEX_STRATEGY` | 多索引解析策略 |
| `UV_FIND_LINKS` | 额外查找包的位置 |
| `UV_OFFLINE` | 离线模式 |
| `UV_HTTP_TIMEOUT` | HTTP 读取超时 |
| `UV_HTTP_RETRIES` | HTTP 重试次数 |
| `UV_SYSTEM_CERTS` | 使用系统证书存储 |

PowerShell 临时设置示例：

```powershell
$env:UV_PYTHON = "3.12"
$env:UV_PROJECT_ENVIRONMENT = ".venv312"
$env:UV_DEFAULT_INDEX = "https://pypi.org/simple"
uv sync
```

PowerShell 永久设置用户环境变量：

```powershell
[Environment]::SetEnvironmentVariable("UV_PYTHON_PREFERENCE", "managed", "User")
[Environment]::SetEnvironmentVariable("UV_CACHE_DIR", "D:\uv-cache", "User")
```

macOS / Linux 临时设置示例：

```bash
UV_PYTHON=3.12 UV_PROJECT_ENVIRONMENT=.venv312 uv sync
```

macOS / Linux 写入 shell 配置：

```bash
export UV_PYTHON_PREFERENCE=managed
export UV_CACHE_DIR="$HOME/.cache/uv"
```

### 10. 配置优先级

uv 配置来源有优先级：

```text
命令行参数
  > 环境变量
  > 项目级配置 pyproject.toml / uv.toml
  > 用户级配置 uv.toml
  > 系统级配置 uv.toml
  > uv 默认值
```

项目级配置：

```toml
# pyproject.toml
[tool.uv]
python-preference = "managed"

[[tool.uv.index]]
url = "https://pypi.org/simple"
default = true
```

独立 `uv.toml`：

```toml
# uv.toml
python-preference = "managed"

[[index]]
url = "https://pypi.org/simple"
default = true
```

注意：

```text
同一目录下如果同时存在 uv.toml 和 pyproject.toml，
uv.toml 的优先级高于 pyproject.toml 中的 [tool.uv]。
```

用户级配置常见位置：

| 系统 | 用户级配置 |
|---|---|
| Windows | `%APPDATA%\uv\uv.toml` |
| macOS / Linux | `~/.config/uv/uv.toml` |

系统级配置常见位置：

| 系统 | 系统级配置 |
|---|---|
| Windows | `%PROGRAMDATA%\uv\uv.toml` |
| macOS / Linux | `/etc/uv/uv.toml` |

### 11. 切换与控制的典型场景

#### 场景 A：完全使用 uv 管理 Python

```bash
uv python install 3.12
uv init demo
cd demo
uv python pin 3.12
uv add requests
uv run python -V
uv run python -c "import sys; print(sys.executable)"
```

特点：

```text
Python 来自 uv managed Python。
项目环境来自当前项目 .venv。
依赖版本由 pyproject.toml + uv.lock 控制。
```

#### 场景 B：项目固定 Python，但不污染系统 Python

```bash
uv python pin 3.11
uv sync
uv run python -V
```

如果需要临时用另一个 Python 测试：

```bash
uv run --python 3.12 python -V
uv run --python 3.12 pytest
```

#### 场景 C：老项目只有 requirements.txt

```bash
uv venv --python 3.12
uv pip install -r requirements.txt
uv pip check
uv run python -V
```

特点：

```text
此时 uv 主要作为 pip + virtualenv 的高速替代。
还没有进入完整 uv project 模型。
```

#### 场景 D：CI / Docker 中固定环境

```bash
uv sync --frozen
uv run pytest
```

如果 CI 中明确要写入系统环境，才考虑：

```bash
uv pip install --system -r requirements.txt
```

日常开发不推荐 `--system`，因为它会修改系统 Python 环境。

#### 场景 E：排查“为什么 uv 用的不是我想要的 Python”

按顺序执行：

```bash
uv --version
uv python find
uv python find --system
uv python list --only-installed
uv run python -V
uv run python -c "import sys; print(sys.executable)"
```

Windows 继续查：

```powershell
Get-Command uv
where.exe uv
$env:UV_PYTHON
$env:UV_PYTHON_PREFERENCE
$env:UV_PROJECT_ENVIRONMENT
$env:VIRTUAL_ENV
```

macOS / Linux 继续查：

```bash
command -v uv
which -a uv
echo "$UV_PYTHON"
echo "$UV_PYTHON_PREFERENCE"
echo "$UV_PROJECT_ENVIRONMENT"
echo "$VIRTUAL_ENV"
```

判断逻辑：

```text
1. 如果 Get-Command / which 找到多个 uv，先解决 PATH 顺序。
2. 如果 UV_PYTHON 设置了，它会强烈影响 Python 选择。
3. 如果项目里有 .python-version，它会影响项目默认 Python。
4. 如果 .venv 已存在，确认它是不是旧 Python 创建的。
5. 如果 uv pip 行为异常，检查 VIRTUAL_ENV / CONDA_PREFIX。
```

### 12. 一句话总览

```text
uv 的“切换 Python”不是修改系统 Python，
而是在命令执行时通过 --python、UV_PYTHON、.python-version、requires-python 和发现规则，
选择解释器，再创建或复用对应的虚拟环境。
```

参考资料：

- uv 官方文档：https://docs.astral.sh/uv/
- 安装 uv：https://docs.astral.sh/uv/getting-started/installation/
- Python versions：https://docs.astral.sh/uv/concepts/python-versions/
- Storage：https://docs.astral.sh/uv/reference/storage/
- Environment variables：https://docs.astral.sh/uv/reference/environment/
- Configuration files：https://docs.astral.sh/uv/concepts/configuration-files/
