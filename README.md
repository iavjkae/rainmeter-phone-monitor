# Rainmeter Phone Monitor

面向手机副屏的 Rainmeter 皮肤套件，附带原生 GPU 插件与可选 FPS 采集程序。
提供蓝白、暖灰绿、OLED 黑红配色，可调背景透明度、纯黑底和低亮模式。

运行依赖 Windows x64、Rainmeter 4.5+；GPU 指标依赖 NVIDIA 驱动提供的 NVML。

## 开发环境

项目工具版本和任务入口统一由 [mise.toml](mise.toml) 管理，
Python 开发依赖由 `pyproject.toml` / `uv.lock` 锁定。
另需安装 Windows C++17 工具链（MSVC Developer PowerShell 或 MinGW-w64）。

```powershell
mise install
mise run setup
mise run check
```

`check` 包含 lint、生成结果一致性、隐私检查、原生构建和合成数据测试。
默认测试不查询真实硬件、不启动游戏、不获取屏幕。

| 任务 | 用途 |
| --- | --- |
| `mise run generate` | 从配色配置、模板和布局模块生成皮肤 |
| `mise run format` | 格式化 Python、C++、CMake 源码 |
| `mise run lint` | lint、配置检查、隐私检查和 Python 测试 |
| `mise run build` | 编译 Windows DLL、FPSBridge 和核心测试 |
| `mise run test` | 执行使用伪造数据的 C++ 核心测试 |
| `mise run smoke` | 仅校验 DLL ABI，不访问硬件 |
| `mise run package` | 生成不含第三方程序的安装目录 |
| `mise run package-fps` | 下载并校验官方 PresentMon 后打包 |

`package` 默认写入 `dist/`，要求目标目录不存在，避免把旧目录中的个人文件混入安装包。
需要保留旧包时，通过 `tools/package.py --output` 指定一个新的目录。
真实传感器检查只能显式执行 `tools/smoke_telemetry.py --hardware`，结果仍不输出硬件值。

## 代码结构

```text
include/phone_monitor/       仅公开 Rainmeter C ABI
src/rainmeter/              Host API、配置适配、输出格式与插件边界
src/telemetry/              类型化数据、Provider 接口、采集服务与 FPS 文件源
src/platform/windows/       WMI、NVML、字符转换及 Windows 进程/资源管理
src/fps/                    配置、CSV、筛选、帧历史、状态协议与采集编排
tools/skin_builder/         绘图原语、布局区域和 measure 生成
templates/                  Rainmeter 配置模板
config/themes.toml          三种配色配置
skins/PhoneMonitor/         生成的皮肤及默认资源配置
tests/                      合成数据测试与可选图形调试程序
```

采集服务不接触 Rainmeter 或 Windows API；硬件实现通过 Provider 接口注入。
GPU 名称只在初始化所选设备时查询，CPU 名称通过 WMI 查询一次并缓存。
NVML 的句柄、结构和函数指针只存在于 Windows 实现内。单个指标读取失败返回空值，
可用性状态与读数分开；Rainmeter 适配器保留原有 13 字段输出以兼容皮肤。
详见 [架构说明](docs/architecture.md)。

## 安装

1. 执行 `mise run package` 或 `mise run package-fps`。
2. 退出 Rainmeter，将 `dist/Skins/PhoneMonitor` 复制到实际皮肤目录，
   将 `dist/Plugins/PhoneTelemetry.dll` 复制到实际插件目录。
3. 启动 Rainmeter，刷新后加载 PhoneMonitor 下的配色，将其拖到手机扩展屏。

也可调用 `tools/install.ps1`，显式传入自己的 `-SkinPath` 和 `-PluginPath`。
脚本只在本地备份现有皮肤，保留显示设置和个人 FPS/遥测配置，不修改其他皮肤。
卸载时退出 Rainmeter，移除该皮肤与本项目 DLL 即可。

默认画布为 1600 × 720，`S` 控制等比缩放。顶部 BLUE / CLOUD / OLED 切换配色。
“背景”数字是不透明度：点击按 25% 切换，滚轮按 10% 调节；
“纯黑底”决定透明区域显示纯黑还是露出桌面；“低亮”降低主题颜色亮度。
设置保存在安装目录的 `DisplaySettings.inc`。
透明不能消除烧屏风险；建议降低手机亮度，不用时熄屏。

## 采集配置

`@Resources/TelemetrySettings.inc` 提供 `GPUIndex` 与 `FPSFreshnessMs`。
`GPUIndex` 选择 NVML 设备序号；缺少 NVML 时保留 CPU、内存、网络功能。

`@Resources/FPSBridge.ini` 提供采集程序路径、会话名、更新周期、统计窗口和进程策略。
个人覆盖写入同目录 `FPSBridge.local.ini`；该文件不受版本控制，也不会被打包。

```ini
[Selection]
Include=sample-game*.exe
Exclude=sample-game-helper.exe
ForegroundOnly=1
```

以上进程名是虚构示例。规则用分号分隔，支持 `*` 与 `?`，ASCII 大小写不敏感；排除优先。
默认 include / exclude 均为空，源码中没有应用名单。默认仅选择前台进程；
明确设置 `ForegroundOnly=0` 后，才允许退回到活跃的其他交换链。
配置不合法或出现未知键时，程序报通用错误，不静默忽略，也不回显配置值。

点击皮肤的 FPS 启动入口，确认 ETW 所需的 Windows UAC。
FPS 使用近期平均，1% Low 根据历史窗口内最慢 1% 帧时间计算；没有有效数据时显示 `--`。
采集使用独立命名会话，仅管理自身子进程；皮肤卸载后按配置的宽限时间停止。
不保存原始 PresentMon CSV、进程清单或诊断读数，临时状态只供本地皮肤通信。

## 许可证

项目源码采用 [MIT License](LICENSE)。PresentMon 作为可选独立依赖固定版本和 SHA-256，
打包时保留其许可证。Rainmeter、驱动和字体不随本项目分发。
参见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。
