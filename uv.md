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
uv find        # 查看uv使用python
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