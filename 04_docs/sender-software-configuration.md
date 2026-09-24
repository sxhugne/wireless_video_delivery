# 发送端软件配置文档

**文档版本**: v1.0  
**更新时间**: 2026-09-23  
**适用范围**: Gemini Wireless Video V3 发送端

---

## 1. 文档概述

### 1.1 目的

本文档详细说明 Gemini Wireless Video V3 发送端软件的配置方法、参数含义和部署流程，为开发和运维人员提供完整的配置指南。

### 1.2 适用范围

- **硬件平台**: ARM64 Linux 开发板（LubanCat、OrangePi 5 Pro、RK3588 等）
- **相机型号**: Orbbec Gemini 系列（Gemini 305、SV1301S 等）
- **软件版本**: Gemini Wireless Video V3.x

### 1.3 前置条件

在开始配置前，需要确保：

1. 开发板已安装 ARM64 Linux 操作系统
2. 已获取相机的序列号或 UID
3. 已确定接收端 IP 地址
4. 具备 SSH 或串口访问权限

---

## 2. 系统要求

### 2.1 硬件要求

| 项目 | 要求 | 说明 |
|------|------|------|
| 处理器 | ARM64 / aarch64 | 必须支持硬件 H.264 编码 |
| 内存 | ≥2GB | 建议 4GB 以上 |
| 存储 | ≥8GB | 用于系统和日志 |
| USB 接口 | USB 2.0/3.0 | 用于连接 Orbbec 相机 |
| 网络接口 | 以太网或 Wi-Fi | 建议使用 5GHz Wi-Fi 或千兆以太网 |

**已验证的开发板**：
- LubanCat-3IO (RK3576)
- OrangePi 5 Pro (RK3588)
- 其他 RK3588/RK3576 开发板

### 2.2 软件要求

| 组件 | 版本要求 | 说明 |
|------|----------|------|
| Linux 内核 | ≥5.10 | 需要支持 USB 和 V4L2 |
| Orbbec SDK | 2.8.6+ (Gemini 305) | 不同相机型号要求不同 |
| GStreamer | 1.20+ | 需要与插件 ABI 一致 |
| Rockchip MPP | 匹配内核版本 | 硬件编码器驱动 |
| OpenCV | 4.x | 用于预览和图像处理 |
| cmake | ≥3.16 | 编译工具 |
| g++ | 支持 C++17 | 编译器 |

### 2.3 依赖库

运行时依赖：
```bash
libopencv-core
libopencv-imgproc
libopencv-imgcodecs
libopencv-highgui
libjsoncpp
libgstreamer1.0-0
libgstreamer-plugins-base1.0-0
zlib1g
libjpeg8
```

编译时额外依赖：
```bash
cmake
g++
pkg-config
libopencv-dev
libjsoncpp-dev
libgstreamer1.0-dev
libgstreamer-plugins-base1.0-dev
zlib1g-dev
liblz4-dev
libjpeg-dev
```

---

## 3. 配置文件结构

### 3.1 配置文件位置

- **开发/测试**: `06_configs/<sender-config>.json`
- **生产运行**: `/etc/gwv3/sender.json`（由安装脚本创建）

### 3.2 配置文件格式

配置文件采用 JSON 格式，主要包含以下部分：

```json
{
  "sender_id": "设备唯一标识",
  "sender_version": "软件版本",
  "receiver": { /* 接收端配置 */ },
  "clock_sync": { /* 时钟同步配置 */ },
  "transport": { /* 网络传输配置 */ },
  "heartbeat_interval_ms": 1000,
  "preview": { /* 本地预览配置 */ },
  "web_rgb_preview": { /* Web 预览配置 */ },
  "logging": { /* 日志配置 */ },
  "hotplug": { /* 热插拔配置 */ },
  "recording_buffer": { /* 缓冲配置 */ },
  "cameras": [ /* 相机阵列配置 */ ]
}
```

---

## 4. 核心配置项详解

### 4.1 设备身份配置

#### 4.1.1 sender_id（必填）

发送端的唯一标识，用于在接收端区分不同设备。

**配置规则**：
- **固定值**（推荐）：使用设备序列号、MAC 地址或资产编号
- **自动生成**：设置为 `"auto"`，系统会基于硬件信息生成

```json
"sender_id": "lubancat-e8cc0cb3"
```

⚠️ **注意事项**：
1. 多设备环境中 `sender_id` 必须唯一
2. 不要使用临时信息（如 IP 地址）
3. 更换 Wi-Fi 网卡不应改变 `sender_id`
4. 克隆镜像时必须修改 `sender_id`

#### 4.1.2 sender_version

软件版本号，用于版本管理和兼容性检查。

```json
"sender_version": "3.0.0"
```

### 4.2 接收端配置

#### 4.2.1 receiver.ip（必填）

接收端的 IP 地址，作为自动发现失败时的兜底地址。

```json
"receiver": {
  "ip": "192.168.1.196",
  "media_port": 50010,
  "status_port": 50011
}
```

| 字段 | 类型 | 说明 | 默认值 |
|------|------|------|--------|
| ip | string | 接收端 IP 地址或主机名 | 必填 |
| media_port | int | 媒体数据端口 (TCP) | 50010 |
| status_port | int | 状态数据端口 (UDP) | 50011 |

**自动发现机制**：
- 发送端会通过 UDP 50009 端口自动发现接收端
- `receiver.ip` 仅在自动发现失败时使用
- 自动发现成功后会优先使用发现的地址

### 4.3 时钟同步配置

CLOCK_SYNC 用于在发送端和接收端之间建立软件统一时间轴。

```json
"clock_sync": {
  "enabled": true,
  "receiver_ip": "192.168.1.196",
  "port": 50012,
  "interval_ms": 2000,
  "timeout_ms": 100,
  "max_delay_us": 100000,
  "sample_window": 10
}
```

| 字段 | 说明 | 推荐值 |
|------|------|--------|
| enabled | 是否启用时钟同步 | true |
| receiver_ip | CLOCK_SYNC 服务器地址 | 与 receiver.ip 一致 |
| port | 时钟同步端口 | 50012 |
| interval_ms | 探测间隔（毫秒） | 2000 |
| timeout_ms | 单次探测超时 | 100 |
| max_delay_us | 最大可接受延迟（微秒） | 100000 |
| sample_window | 滑动窗口大小 | 10 |

⚠️ **重要**：CLOCK_SYNC 不能替代 chrony 系统时间同步，也不能实现硬件级曝光同步。

### 4.4 网络传输配置

```json
"transport": {
  "enabled": true,
  "status_protocol": "udp",
  "media_protocol": "tcp",
  "connect_timeout_ms": 1500,
  "send_timeout_ms": 80,
  "send_buffer_bytes": 33554432,
  "reconnect_interval_ms": 1000
}
```

| 字段 | 说明 | 生产值 |
|------|------|--------|
| enabled | 是否启用传输 | true |
| status_protocol | 状态协议 | "udp" |
| media_protocol | 媒体协议 | "tcp"（推荐） |
| connect_timeout_ms | 连接超时 | 1500 |
| send_timeout_ms | 发送超时 | 80 |
| send_buffer_bytes | 发送缓冲区大小 | 33554432 (32MB) |
| reconnect_interval_ms | 重连间隔 | 1000 |

### 4.5 相机配置

这是配置的核心部分，支持单相机或多相机配置。

#### 4.5.1 基本相机配置

```json
"cameras": [
  {
    "camera_id": "cam01",
    "capture_backend": "v4l2",
    "serial_number": "AY2M54301VE",
    "video_device": "/dev/video0",
    "device_model": "Gemini_305",
    "validate_rgb_mjpeg": false
  }
]
```

| 字段 | 必填 | 说明 |
|------|------|------|
| camera_id | ✅ | 相机唯一标识（在该 sender 内） |
| capture_backend | ✅ | 采集后端：`"orbbec_sdk"` 或 `"v4l2"` |
| serial_number | 条件 | 相机序列号（多相机必填） |
| uid | 条件 | USB 设备 UID（可替代 serial_number） |
| video_device | 条件 | V4L2 设备节点（v4l2 后端需要） |
| device_model | ✅ | 相机型号 |
| validate_rgb_mjpeg | ⚠️ | 是否验证 MJPEG 格式 |

**相机标识规则**：
- **单相机设备**：只需配置 `device_model`，可接受同型号替换
- **多相机设备**：必须配置 `serial_number` 或 `uid`，确保 camera_id 稳定
- **camera_id 命名**：建议使用 `cam01`、`cam02` 等规则命名

#### 4.5.2 RGB 配置

```json
"rgb_profile": {
  "width": 1920,
  "height": 1080,
  "fps": 30,
  "format": "mjpg"
}
```

| 字段 | 可选值 | 说明 |
|------|--------|------|
| width | 640, 1280, 1920 | 分辨率宽度 |
| height | 480, 720, 1080 | 分辨率高度 |
| fps | 15, 30 | 帧率 |
| format | "mjpg", "rgb", "yuyv" | 色彩格式 |

**推荐配置**：
- **高质量录制**：1920x1080 @ 30fps, MJPG
- **低带宽场景**：1280x720 @ 30fps, MJPG

#### 4.5.3 Depth 配置

```json
"depth_profile": {
  "enabled": true,
  "width": 640,
  "height": 400,
  "fps": 30,
  "format": "y16"
}
```

| 字段 | 可选值 | 说明 |
|------|--------|------|
| enabled | true/false | 是否启用深度采集 |
| width | 320, 640 | 深度图宽度 |
| height | 200, 400 | 深度图高度 |
| fps | 15, 30 | 深度帧率 |
| format | "y16", "y12" | 深度格式（16位/12位） |

#### 4.5.4 RGB 编码配置

```json
"rgb_encoding": {
  "codec": "h264",
  "mode": "hardware",
  "gstreamer_encoder": "mpph264enc",
  "bitrate_bps": 6000000
}
```

| 字段 | 说明 | 推荐值 |
|------|------|--------|
| codec | 编码器类型 | "h264" |
| mode | 编码模式 | "hardware" |
| gstreamer_encoder | GStreamer 编码器插件 | "mpph264enc" (Rockchip) |
| bitrate_bps | 码率（比特/秒） | 6000000 (1080p30) |

**码率建议**：
- 1920x1080 @ 30fps: 6000000 bps (6 Mbps)
- 1280x720 @ 30fps: 3000000 bps (3 Mbps)
- 640x480 @ 30fps: 1000000 bps (1 Mbps)

#### 4.5.5 色彩控制配置

```json
"color_controls": {
  "auto_exposure": false,
  "exposure": 312,
  "auto_exposure_priority": 0,
  "power_line_frequency": 1
}
```

| 字段 | 说明 | 值范围 |
|------|------|--------|
| auto_exposure | 自动曝光 | true/false |
| exposure | 曝光时间 | 相机相关（Gemini 305: 1-10000） |
| auto_exposure_priority | 自动曝光优先级 | 0/1 |
| power_line_frequency | 防闪烁 | 0(禁用), 1(50Hz), 2(60Hz) |

⚠️ **曝光注意事项**：
- `auto_exposure=true` 时，`exposure` 值不作为实时生效值
- 使用软件自适应曝光时，必须关闭原生 AE
- 不同型号相机的曝光范围和单位不同，不能直接复制数值

### 4.6 预览配置

#### 4.6.1 本地预览

```json
"preview": {
  "enabled": false,
  "fps": 30
}
```

**使用场景**：
- 调试时在开发板本地显示器上查看画面
- 需要 X11 显示环境
- 生产环境通常禁用

#### 4.6.2 Web 预览

```json
"web_rgb_preview": {
  "enabled": false,
  "max_width": 640,
  "max_height": 360,
  "fps": 30,
  "bitrate_bps": 500000
}
```

**使用场景**：
- 在接收端 Web 界面查看实时画面
- 低码率低分辨率，不影响录制质量
- 建议在调试或监控时启用

### 4.7 日志配置

```json
"logging": {
  "directory": "08_reports/sender_logs",
  "max_bytes": 10485760
}
```

| 字段 | 说明 | 推荐值 |
|------|------|--------|
| directory | 日志目录（相对或绝对路径） | 08_reports/sender_logs |
| max_bytes | 单个日志文件最大字节数 | 10485760 (10MB) |

### 4.8 热插拔配置

```json
"hotplug": {
  "enabled": false
}
```

**说明**：
- 启用后，系统会自动检测相机插拔并重新连接
- 生产环境通常禁用，使用固定相机配置
- 调试时可启用以支持相机重插

---

## 5. 编译和安装

### 5.1 编译步骤

⚠️ **重要**：发送端必须在目标 ARM64 设备本机编译，不能跨架构复制二进制。

```bash
# 1. 设置 Orbbec SDK 路径
export ORBBEC_SDK_ROOT=/path/to/OrbbecSDK

# 2. 配置 CMake
cmake -S . -B 12_build \
  -DGWV3_BUILD_RECEIVER=OFF \
  -DGWV3_BUILD_SENDER=ON \
  -DBUILD_TESTING=ON \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo

# 3. 编译
cmake --build 12_build -j2

# 4. 运行测试
ctest --test-dir 12_build --output-on-failure
```

### 5.2 编译产物位置

```
12_build/
└── bin/
    ├── gemini_sender                # 主程序
    ├── orbbec_depth_probe          # 深度探测工具
    ├── orbbec_fps_probe            # FPS 探测工具
    ├── depth_compression_bench     # 压缩性能测试
    └── OrbbecSDKConfig_v1.0.xml    # SDK 配置文件
```

### 5.3 配置验证

```bash
# 验证配置文件格式
./12_build/bin/gemini_sender \
  --config 06_configs/<sender-config>.json \
  --validate-config

# 运行预检脚本
./05_tools/sender_preflight.sh 06_configs/<sender-config>.json
```

---

## 6. 服务配置和开机自启

### 6.1 安装 systemd 服务

```bash
# 使用安装脚本（推荐）
sudo ./05_tools/install_device.sh sender \
  --config 06_configs/<sender-config>.json \
  --run-user cat \
  --receiver-fallback 192.168.1.196 \
  --chrony-server 192.168.1.196
```

**参数说明**：
- `--config`: 配置文件路径
- `--run-user`: 运行服务的 Linux 用户
- `--receiver-fallback`: 接收端 IP 地址（覆盖配置文件中的值）
- `--chrony-server`: 时间同步服务器地址

### 6.2 服务管理命令

```bash
# 查看服务状态
systemctl status gwv3-gemini-sender.service

# 启动服务
sudo systemctl start gwv3-gemini-sender.service

# 停止服务
sudo systemctl stop gwv3-gemini-sender.service

# 重启服务
sudo systemctl restart gwv3-gemini-sender.service

# 查看服务日志
journalctl -u gwv3-gemini-sender.service -f
```

### 6.3 检查开机自启

```bash
# 查看是否启用开机自启
systemctl is-enabled gwv3-gemini-sender.service

# 启用开机自启
sudo systemctl enable gwv3-gemini-sender.service

# 禁用开机自启
sudo systemctl disable gwv3-gemini-sender.service
```

---

## 7. 运行和调试

### 7.1 前台运行（调试）

```bash
# 前台运行，查看实时输出
./05_tools/run_sender_foreground.sh 06_configs/<sender-config>.json

# 带本地预览窗口运行
./05_tools/start_sender_preview.sh 06_configs/<sender-config>.json
```

### 7.2 后台运行（生产）

```bash
# 启动后台服务
./05_tools/start_sender.sh 06_configs/<sender-config>.json

# 查看运行状态
./05_tools/status_sender.sh 06_configs/<sender-config>.json

# 停止服务
./05_tools/stop_sender.sh
```

### 7.3 查看日志

```bash
# 查看实时日志
tail -f 08_reports/sender_logs/sender.log

# 查看 systemd 日志
journalctl -u gwv3-gemini-sender.service -f

# 查看最近 100 行日志
journalctl -u gwv3-gemini-sender.service -n 100
```

### 7.4 调试命令

```bash
# 检查相机是否识别
lsusb -d 2bc5:

# 查看 Video 设备
ls -la /dev/video*

# 检查 GStreamer 编码器
gst-inspect-1.0 mpph264enc

# 查看进程状态
ps aux | grep gemini_sender

# 查看网络连接
sudo ss -tulnp | grep gemini
```

---

## 8. 网络和性能优化

发送端的网络性能直接影响视频流的稳定性和延迟。本节介绍 Wi-Fi、TCP 和系统级的优化配置。

### 8.1 Wi-Fi 优化

#### 8.1.1 Wi-Fi 频段要求

发送端默认要求使用 **5GHz Wi-Fi**，以保证足够的带宽和较低的延迟。

**频段检查**：
```bash
# 查看当前 Wi-Fi 链路信息
iw dev wlan0 link

# 输出示例：
# Connected to xx:xx:xx:xx:xx:xx (on wlan0)
#     SSID: MyNetwork
#     freq: 5180         # 5GHz (5000+ MHz)
```

**Wi-Fi Guard 自动策略**：

发送端 Watchdog 会在启动前运行 `sender_wifi_guard.sh` 检查：
1. 当前链路是否在 5GHz 频段（≥5000 MHz）
2. 如果不满足，自动从已保存的 Wi-Fi 配置中选择优先级最高的 5GHz 连接
3. 禁用 Wi-Fi 省电模式

**环境变量配置**：

```bash
# 指定 Wi-Fi 接口（默认 wlan0）
export GEMINI_SENDER_WIFI_IFACE=wlan0

# 指定固定连接名（用于特定测试）
export GEMINI_SENDER_WIFI_CONNECTION="MyNetwork5G"

# 要求的最低频率（默认 5000 MHz）
export GEMINI_SENDER_WIFI_MIN_FREQ_MHZ=5000

# 要求的 SSID
export GEMINI_SENDER_WIFI_REQUIRED_SSID="MyNetwork5G"

# 是否禁用省电模式（默认 1=禁用）
export GEMINI_SENDER_WIFI_DISABLE_POWERSAVE=1
```

#### 8.1.2 Wi-Fi 驱动优化（rtw_8821cu / rtl8821cu）

**问题**：USB Wi-Fi 驱动 `rtw_8821cu` 和 `rtl8821cu` 默认发送队列过大，导致数百毫秒的排队延迟。

**解决方案**：安装 Wi-Fi 队列限制服务

```bash
sudo ./05_tools/install_sender_wifi_tuning.sh
```

**优化效果**：
- ✅ 限制 Wi-Fi 发送队列为 **128 个包**（默认可能数千）
- ✅ 减少驱动排队延迟（从 200-500ms 降至 10-20ms）
- ✅ 强制禁用 Wi-Fi 省电模式
- ✅ 开机自动生效
- ✅ 网卡重新枚举或重连后自动恢复配置

**验证配置**：

```bash
# 1. 查看队列类型和限制
tc -j qdisc show dev wlan0 | python3 -c '
import json, sys
for q in json.load(sys.stdin):
    if q.get("root"):
        print(f"Kind: {q.get(\"kind\")}, Limit: {q.get(\"options\", {}).get(\"limit\")}")
'
# 期望输出：Kind: pfifo, Limit: 128

# 2. 查看省电模式
iw dev wlan0 get power_save
# 期望输出：Power save: off

# 3. 查看服务状态
systemctl status gwv3-sender-wifi-tuning.service

# 4. 查看应用日志
journalctl -u gwv3-sender-wifi-tuning@wlan0.service -n 20
```

**工作原理**：

安装脚本会部署：
1. `/usr/local/sbin/gwv3-apply-sender-wifi-tuning` - 应用脚本
2. `/etc/systemd/system/gwv3-sender-wifi-tuning.service` - 开机服务
3. `/etc/systemd/system/gwv3-sender-wifi-tuning@.service` - 按接口实例化服务
4. `/etc/NetworkManager/dispatcher.d/90-gwv3-sender-wifi-tuning` - 网络事件钩子

当网卡状态变化（插入、重连、DHCP 续约）时，NetworkManager dispatcher 会触发队列重新配置。

**支持的驱动**：
- `rtw_8821cu`（Realtek 官方驱动）
- `rtl8821cu`（厂商修改版）

其他 Wi-Fi 驱动不受影响（保持系统默认配置）。

#### 8.1.3 Wi-Fi 故障排查

```bash
# 检查当前 Wi-Fi 状态
./05_tools/sender_preflight.sh 06_configs/<sender-config>.json | grep -i wifi

# 手动运行 Wi-Fi Guard
source ./05_tools/sender_wifi_guard.sh
gemini_sender_wifi_apply_repo_defaults
gemini_sender_wifi_policy_summary
gemini_sender_wifi_check_policy

# 查看当前频率
gemini_sender_wifi_current_freq

# 手动切换到 5GHz
nmcli connection up "MyNetwork5G" ifname wlan0
```

### 8.2 TCP 传输优化

#### 8.2.1 TCP 发送缓冲区

发送端默认配置 **32MB TCP 发送缓冲区**，以应对网络抖动和突发流量。

**配置项**（在 JSON 配置文件中）：

```json
"transport": {
  "send_buffer_bytes": 33554432,  // 32MB = 32 * 1024 * 1024
  "tcp_notsent_lowat_bytes": 131072  // 128KB 低水位
}
```

**参数说明**：

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `send_buffer_bytes` | 33554432 (32MB) | TCP socket 发送缓冲区大小 |
| `tcp_notsent_lowat_bytes` | 0 (禁用) | TCP_NOTSENT_LOWAT 低水位阈值 |

**send_buffer_bytes**：
- 通过 `SO_SNDBUF` 设置 TCP socket 发送缓冲区
- 实际分配的大小由内核决定（通常是请求值的 2 倍）
- 容量 = 实际分配 / 2（内核簿记开销）

**tcp_notsent_lowat_bytes**（高级特性）：
- Linux 3.12+ 支持 `TCP_NOTSENT_LOWAT`
- 当 socket 中**未发送**字节数低于此阈值时，才允许继续写入
- 作用：保持数据在应用层有界队列中，而非内核的无界队列
- 推荐值：128KB - 256KB（约 20-40 帧数据）

**代码实现**（[transport.cpp:187-207](D:\projcet\wireless_video_delivery\01_sender_linux\src\transport.cpp#L187-L207)）：

```cpp
if(config_.transport.send_buffer_bytes > 0) {
    int requested = config_.transport.send_buffer_bytes;
    setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &requested, sizeof(requested));
}
int actual = 0;
socklen_t actual_len = sizeof(actual);
if(getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &actual, &actual_len) == 0) {
    media_send_buffer_bytes_ = actual;
}
int one = 1;
setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
#ifdef TCP_NOTSENT_LOWAT
if(config_.transport.tcp_notsent_lowat_bytes > 0) {
    const uint32_t lowat = static_cast<uint32_t>(config_.transport.tcp_notsent_lowat_bytes);
    setsockopt(fd, IPPROTO_TCP, TCP_NOTSENT_LOWAT, &lowat, sizeof(lowat));
}
#endif
```

#### 8.2.2 TCP 背压和重传监控

发送端会实时监控 TCP 连接状态：

```cpp
// 获取诊断信息
std::string Transport::media_diagnostics() const {
    int queued = -1;
    ioctl(media_tcp_fd_, TIOCOUTQ, &queued);  // 获取发送队列字节数
    
    tcp_info info{};
    socklen_t length = sizeof(info);
    getsockopt(media_tcp_fd_, IPPROTO_TCP, TCP_INFO, &info, &length);
    
    return "tcp_send_queue_bytes=" + std::to_string(queued)
           + " tcp_rtt_us=" + std::to_string(info.tcpi_rtt)        // RTT（微秒）
           + " tcp_rto_us=" + std::to_string(info.tcpi_rto)        // RTO（微秒）
           + " tcp_unacked=" + std::to_string(info.tcpi_unacked)   // 未确认包数
           + " tcp_total_retrans=" + std::to_string(info.tcpi_total_retrans);  // 总重传数
}
```

**监控指标**：
- `tcp_send_queue_bytes` - 发送队列积压（越大越可能丢帧）
- `tcp_rtt_us` - 往返时延（正常 5G Wi-Fi: 1000-5000 微秒）
- `tcp_total_retrans` - 累计重传次数（频繁重传说明网络质量差）

#### 8.2.3 TCP 重连策略

**连接超时**：
```json
"transport": {
  "connect_timeout_ms": 1500,      // 连接超时 1.5 秒
  "reconnect_interval_ms": 1000    // 重连间隔 1 秒
}
```

**自动重连触发条件**（[transport.cpp:469-543](D:\projcet\wireless_video_delivery\01_sender_linux\src\transport.cpp#L469-L543)）：
1. TCP 对端关闭连接（`POLLHUP` / `POLLRDHUP`）
2. 发送失败（`send()` 返回错误）
3. 连续 30 次背压丢帧（缓冲区满）
4. 接收端地址变更（自动发现更新）

### 8.3 系统级网络优化

#### 8.3.1 发送端系统参数

发送端通常不需要修改系统网络参数，Wi-Fi 和 TCP 优化已足够。

如果需要进一步调优（如有线千兆网络），可以调整：

```bash
# 增大 TCP 发送缓冲区上限
sudo sysctl -w net.core.wmem_max=33554432
sudo sysctl -w net.ipv4.tcp_wmem="4096 1048576 33554432"

# 增大 UDP 发送缓冲区
sudo sysctl -w net.core.wmem_default=8388608

# 持久化配置
sudo tee /etc/sysctl.d/99-gwv3-sender.conf <<EOF
net.core.wmem_max = 33554432
net.ipv4.tcp_wmem = 4096 1048576 33554432
net.core.wmem_default = 8388608
EOF
sudo sysctl --system
```

#### 8.3.2 接收端网络优化（参考）

接收端需要处理多个发送端的并发流量，需要安装网络优化：

```bash
sudo ./05_tools/install_receiver_network_tuning.sh
```

**优化内容**（[99-gwv3-receiver-network.conf](D:\projcet\wireless_video_delivery\06_configs\99-gwv3-receiver-network.conf)）：

```ini
# 接收队列深度（处理突发流量）
net.core.netdev_max_backlog = 16384

# 每次轮询处理的包数
net.core.netdev_budget = 600

# 每次轮询的时间预算（微秒）
net.core.netdev_budget_usecs = 8000

# RPS（Receive Packet Steering）流表大小
net.core.rps_sock_flow_entries = 32768

# TCP/UDP 接收/发送缓冲区
net.core.rmem_max = 33554432
net.core.wmem_max = 33554432
net.ipv4.tcp_rmem = 4096 1048576 33554432
net.ipv4.tcp_wmem = 4096 1048576 33554432
```

**RPS 配置**（多核负载均衡）：

脚本会自动检测默认路由接口，并为每个 RX 队列配置 RPS：

```bash
# 查看 RPS 配置
cat /sys/class/net/eth0/queues/rx-0/rps_cpus
cat /sys/class/net/eth0/queues/rx-0/rps_flow_cnt

# 手动应用（已由服务自动执行）
sudo /usr/local/sbin/gwv3-apply-receiver-network-tuning
```

### 8.4 CPU 调度优化

#### 8.4.1 CPU 频率调节器

**查看当前调节器**：
```bash
cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
```

**设置性能模式**（最大频率运行）：
```bash
# 临时设置
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# 持久化配置（Ubuntu/Debian）
sudo apt install cpufrequtils
echo 'GOVERNOR="performance"' | sudo tee /etc/default/cpufrequtils
sudo systemctl restart cpufrequtils
```

**可选调节器**：
- `performance` - 始终最大频率（高性能，高功耗）
- `ondemand` - 根据负载动态调整（默认，推荐）
- `powersave` - 始终最低频率（省电，性能差）
- `schedutil` - 基于调度器反馈调整（现代内核推荐）

#### 8.4.2 进程优先级

发送端进程默认以普通用户权限运行，通常不需要提升优先级。

如果需要降低延迟，可以调整 nice 值：

```bash
# 查看进程优先级
ps -eo pid,ni,cmd | grep gemini_sender

# 提升优先级（降低 nice 值，需要 root）
sudo renice -10 -p $(pgrep gemini_sender)
```

### 8.5 性能监控

#### 8.5.1 网络流量监控

```bash
# 实时网络流量（需要安装 iftop）
sudo iftop -i wlan0

# 查看 TCP 连接状态
ss -tin | grep :50010

# 查看 TCP 统计（重传率）
ss -s
```

#### 8.5.2 资源占用监控

```bash
# CPU 和内存占用
top -p $(pgrep gemini_sender)

# 详细线程信息
htop -p $(pgrep gemini_sender)

# 查看 I/O 和网络
pidstat -t -p $(pgrep gemini_sender) 1
```

#### 8.5.3 TCP 诊断日志

发送端会在日志中输出 TCP 诊断信息：

```
tcp_send_queue_bytes=524288 tcp_rtt_us=2500 tcp_rto_us=210000 tcp_unacked=15 tcp_total_retrans=3
```

**指标解读**：
- `tcp_send_queue_bytes > 10MB` - 网络拥塞，可能开始丢帧
- `tcp_rtt_us > 50000` (50ms) - 延迟过高，检查 Wi-Fi 信号或网络路径
- `tcp_total_retrans` 持续增长 - 网络质量差，考虑切换 Wi-Fi 频段或降低码率

### 8.6 性能调优建议

#### 8.6.1 网络延迟优化

**目标**：端到端延迟 < 100ms

| 优化项 | 效果 | 优先级 |
|--------|------|--------|
| 使用 5GHz Wi-Fi | 减少 50-100ms | ⭐⭐⭐ |
| 安装 Wi-Fi 队列限制 | 减少 100-200ms | ⭐⭐⭐ |
| 降低编码码率 | 减少网络排队 | ⭐⭐ |
| 启用 TCP_NOTSENT_LOWAT | 减少 20-50ms | ⭐ |

#### 8.6.2 带宽优化

**单路 1080p30 带宽需求**：
- RGB H.264: 6 Mbps
- Depth 压缩: 0.5-1 Mbps
- 控制和状态: < 0.1 Mbps
- **总计**: ~7 Mbps

**5GHz Wi-Fi 实际吞吐量**：
- 理论: 300-600 Mbps
- 实际: 100-200 Mbps（取决于信号质量）
- **可支持**: 10-20 路并发

**带宽不足排查**：
```bash
# 1. 检查 Wi-Fi 速率
iw dev wlan0 link | grep -i rate

# 2. 测试实际带宽
iperf3 -c <receiver-ip> -t 10

# 3. 降低编码码率（紧急措施）
# 编辑配置文件 bitrate_bps: 6000000 -> 4000000
```

#### 8.6.3 稳定性优化

**防止长时间运行卡死**：
1. ✅ 启用 Watchdog 监控（已默认开启）
2. ✅ 配置 TCP 自动重连（已默认开启）
3. ✅ 启用相机热插拔恢复（可选）
4. ✅ 安装 Wi-Fi 自动调优服务

**日志轮转**（防止磁盘满）：
```bash
# 查看日志大小限制
grep max_bytes 06_configs/<sender-config>.json

# 手动清理旧日志
find 08_reports/sender_logs/ -name "*.log.*" -mtime +7 -delete
```

---

## 9. 故障排查

### 9.1 相机无法识别

**症状**：`lsusb` 看不到 Orbbec 设备

**排查步骤**：
```bash
# 1. 检查 USB 设备
lsusb

# 2. 检查 USB 枚举日志
dmesg | grep -i usb | tail -20

# 3. 检查权限
ls -la /dev/bus/usb/*/* | grep 2bc5

# 4. 尝试重新插拔相机

# 5. 检查 Orbbec SDK 版本
# Gemini 305 (2bc5:0840) 在 RK3576 上需要 SDK 2.8.6+
```

### 9.2 GStreamer 编码器异常

**症状**：`gst-inspect-1.0 mpph264enc` 返回 "No such element"

**排查步骤**：
```bash
# 1. 检查 GStreamer 版本
pkg-config --modversion gstreamer-1.0

# 2. 检查插件路径
gst-inspect-1.0 --print-plugin-path

# 3. 检查插件是否被列入黑名单
GST_DEBUG=2 gst-inspect-1.0 mpph264enc 2>&1 | grep blacklist

# 4. 检查 ABI 版本匹配
# GStreamer runtime 和 MPP 插件必须 ABI 一致
```

**解决方案**：
- 确保 GStreamer runtime 和 Rockchip MPP 插件 ABI 版本一致
- 如果不一致，将匹配版本的插件放入专用目录
- 通过 `GST_PLUGIN_PATH_1_0` 环境变量指定插件路径

### 9.3 网络连接问题

**症状**：无法连接到接收端

**排查步骤**：
```bash
# 1. 检查接收端是否可达
ping 192.168.1.196

# 2. 检查端口是否开放
telnet 192.168.1.196 50010
nc -zv 192.168.1.196 50010

# 3. 检查防火墙
sudo iptables -L -n

# 4. 查看发送端连接状态
sudo ss -tn | grep 50010

# 5. 查看自动发现日志
journalctl -u gwv3-gemini-sender.service | grep discovery
```

### 9.4 编码器性能问题

**症状**：帧率低于配置值、CPU 占用过高

**排查步骤**：
```bash
# 1. 检查 CPU 负载
top -p $(pgrep gemini_sender)

# 2. 检查是否使用硬件编码器
# 查看日志中的编码器类型

# 3. 检查相机输出帧率
./12_build/bin/orbbec_fps_probe

# 4. 降低分辨率或帧率测试
```

### 9.5 常见错误代码

| 错误信息 | 可能原因 | 解决方法 |
|---------|---------|---------|
| `Failed to enumerate cameras` | SDK 版本不匹配 | 更新到正确的 SDK 版本 |
| `GStreamer encoder initialization failed` | 编码器插件不可用 | 检查 GStreamer 插件 |
| `Connection timeout` | 网络不通或接收端未运行 | 检查网络和接收端状态 |
| `USB device not found` | 相机未连接或权限问题 | 检查 USB 连接和权限 |

---

## 10. 配置示例

### 10.1 单相机配置示例（Gemini 305）

```json
{
  "sender_id": "lubancat-e8cc0cb3",
  "sender_version": "3.0.0",
  "receiver": {
    "ip": "192.168.1.196",
    "media_port": 50010,
    "status_port": 50011
  },
  "clock_sync": {
    "enabled": true,
    "receiver_ip": "192.168.1.196",
    "port": 50012,
    "interval_ms": 2000,
    "timeout_ms": 100,
    "max_delay_us": 100000,
    "sample_window": 10
  },
  "transport": {
    "enabled": true,
    "status_protocol": "udp",
    "media_protocol": "tcp",
    "connect_timeout_ms": 1500,
    "send_timeout_ms": 80,
    "send_buffer_bytes": 33554432,
    "reconnect_interval_ms": 1000
  },
  "heartbeat_interval_ms": 1000,
  "preview": {
    "enabled": false,
    "fps": 30
  },
  "web_rgb_preview": {
    "enabled": true,
    "max_width": 640,
    "max_height": 360,
    "fps": 30,
    "bitrate_bps": 500000
  },
  "logging": {
    "directory": "08_reports/sender_logs",
    "max_bytes": 10485760
  },
  "hotplug": {
    "enabled": false
  },
  "recording_buffer": {
    "enabled": true,
    "rgb_frames_per_slot": 900,
    "depth_frames_per_slot": 1,
    "depth_compression_frames_per_slot": 1
  },
  "cameras": [
    {
      "camera_id": "cam01",
      "capture_backend": "orbbec_sdk",
      "device_model": "Gemini_305",
      "serial_number": "AY2M54301VE",
      "rgb_profile": {
        "width": 1920,
        "height": 1080,
        "fps": 30,
        "format": "mjpg"
      },
      "depth_profile": {
        "enabled": true,
        "width": 640,
        "height": 400,
        "fps": 30,
        "format": "y16"
      },
      "rgb_encoding": {
        "codec": "h264",
        "mode": "hardware",
        "gstreamer_encoder": "mpph264enc",
        "bitrate_bps": 6000000
      },
      "depth_transport": {
        "compression": "zlib",
        "level": 6
      },
      "color_controls": {
        "auto_exposure": false,
        "exposure": 312,
        "auto_exposure_priority": 0,
        "power_line_frequency": 1
      }
    }
  ]
}
```

### 10.2 多相机配置示例（4 路 V4L2）

参见：[sender_orangepi5pro-133_four_rgb_v4l2.json](../06_configs/sender_orangepi5pro-133_four_rgb_v4l2.json)

**关键差异**：
1. 每个相机必须有唯一的 `serial_number`
2. 每个相机映射到不同的 `/dev/videoX` 设备
3. 所有相机共享相同的网络和编码配置

---

## 11. 开机启动流程

### 11.1 启动流程概览

发送端开机启动采用 **systemd 服务 + Watchdog 监控**的架构，确保系统可靠启动和自动恢复。

完整启动流程：

```
开机上电
    ↓
Linux 内核启动
    ↓
systemd 初始化
    ↓
systemd 启动 gwv3-gemini-sender.service
    ↓
执行 /usr/local/sbin/gwv3-sender-service-launcher
    ↓
读取环境配置 /etc/gwv3/sender.env
    ↓
启动 sender_watchdog.sh (父进程)
    ↓
┌─────────────────────────────────────┐
│ sender_watchdog.sh 主循环           │
├─────────────────────────────────────┤
│ 1. 检查 Wi-Fi 频段                 │
│ 2. 运行硬件预检                    │
│ 3. 启动 gemini_sender 进程         │
│ 4. 监控子进程健康状态              │
│ 5. 异常时自动重启                  │
└─────────────────────────────────────┘
    ↓
gemini_sender 进程启动
    ↓
┌─────────────────────────────────────┐
│ gemini_sender 初始化流程            │
├─────────────────────────────────────┤
│ 1. 解析配置文件                    │
│ 2. 初始化日志系统                  │
│ 3. 注册信号处理 (SIGINT/SIGTERM)  │
│ 4. 初始化 Orbbec SDK               │
│ 5. 枚举和打开相机                  │
│ 6. 初始化 GStreamer 编码器         │
│ 7. 连接接收端                      │
│ 8. 启动 CLOCK_SYNC 客户端          │
│ 9. 启动采集、编码、发送线程        │
│ 10. 进入主循环                     │
└─────────────────────────────────────┘
    ↓
正常运行 (采集、编码、发送)
```

### 11.2 启动时序

从开机到开始采集大约需要 **15-25 秒**：

| 时间 | 阶段 | 说明 |
|------|------|------|
| 0s | 系统启动 | Linux 内核启动 |
| 10s | 网络就绪 | 等待 network-online.target |
| 15s | 服务启动 | systemd 启动 gwv3-gemini-sender.service |
| 16s | Wi-Fi 检查 | sender_wifi_guard.sh 检查频段 |
| 17s | 硬件预检 | 检查相机、编码器、网络 |
| 18s | 进程启动 | gemini_sender 进程启动 |
| 20s | SDK 初始化 | 枚举和打开相机 |
| 22s | 开始采集 | 启动 RGB/Depth 数据流 |

### 11.3 systemd 服务配置

服务文件位置：`/etc/systemd/system/gwv3-gemini-sender.service`

```ini
[Unit]
Description=Gemini Wireless Video Sender
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
User=cat
EnvironmentFile=/etc/gwv3/sender.env
ExecStart=/usr/local/sbin/gwv3-sender-service-launcher
Restart=on-failure
RestartSec=10s
KillMode=mixed
TimeoutStopSec=30s

[Install]
WantedBy=multi-user.target
```

**关键配置说明**：
- `After=network-online.target` - 等待网络就绪
- `Restart=on-failure` - 异常退出时自动重启
- `RestartSec=10s` - 重启间隔 10 秒
- `TimeoutStopSec=30s` - 停止超时 30 秒
- `KillMode=mixed` - 先发送 SIGTERM，超时后 SIGKILL

### 11.4 环境变量

从 `/etc/gwv3/sender.env` 加载：

```bash
GWV3_ROOT=/home/cat/wireless_video_delivery
GWV3_CONFIG=/etc/gwv3/sender.json
GWV3_RUN_USER=cat
GWV3_HOME=/home/cat
GWV3_MODE=no-local-preview
ORBBEC_SDK_ROOT=/path/to/OrbbecSDK
GST_PLUGIN_PATH_1_0=/path/to/gstreamer-plugins
```

### 11.5 Watchdog 机制

**sender_watchdog.sh** 功能：

1. **启动前检查**
   - Wi-Fi 频段验证（确保 5GHz）
   - 相机 USB 枚举检查
   - 编码器可用性检查
   - 网络路由检查

2. **进程监控**
   - 启动 gemini_sender 子进程
   - 记录进程 PID 到文件
   - 等待子进程退出
   - 记录退出码

3. **自动恢复**
   - 检测到崩溃（非 0 退出码）
   - 等待 5 秒冷却
   - 重新运行预检
   - 重启 gemini_sender

### 11.6 信号处理和优雅关机

gemini_sender 注册了信号处理器：

```cpp
std::signal(SIGINT, handle_signal);   // Ctrl+C
std::signal(SIGTERM, handle_signal);  // systemd 停止
```

**优雅关机流程**：
1. 收到 SIGTERM 信号（systemd 停止或短按电源键）
2. 设置全局退出标志 `g_running = false`
3. 主循环检测到标志，停止采集
4. 关闭相机连接
5. 释放资源（RAII 自动析构）
6. 进程退出（退出码 0）
7. Watchdog 检测到正常退出，不再重启

### 11.7 启动状态检查

**查看服务状态**：
```bash
# 服务是否运行
systemctl status gwv3-gemini-sender.service

# 服务是否启用开机自启
systemctl is-enabled gwv3-gemini-sender.service

# 服务启动时间
systemctl show gwv3-gemini-sender.service | grep ActiveEnterTimestamp
```

**查看进程状态**：
```bash
# 查看进程树
pstree -p $(pgrep sender_watchdog)

# 查看进程启动时间
ps -eo pid,lstart,cmd | grep gemini_sender

# 查看进程资源占用
ps aux | grep gemini_sender
```

**查看启动日志**：
```bash
# 实时日志
journalctl -u gwv3-gemini-sender.service -f

# 本次启动的日志
journalctl -u gwv3-gemini-sender.service -b

# 最近 100 行
journalctl -u gwv3-gemini-sender.service -n 100
```

### 11.8 启动失败排查

**检查步骤**：

```bash
# 1. 查看服务状态和最近错误
systemctl status gwv3-gemini-sender.service
journalctl -u gwv3-gemini-sender.service --since "5 min ago" | grep -i error

# 2. 检查环境配置
cat /etc/gwv3/sender.env
cat /etc/gwv3/sender.json

# 3. 手动运行预检
./05_tools/sender_preflight.sh /etc/gwv3/sender.json

# 4. 手动前台运行测试
./05_tools/run_sender_foreground.sh /etc/gwv3/sender.json

# 5. 检查硬件
lsusb -d 2bc5:                    # 相机
gst-inspect-1.0 mpph264enc        # 编码器
ping 192.168.1.196                # 接收端
```

**常见启动失败原因**：

| 问题 | 症状 | 解决方法 |
|------|------|---------|
| 相机未连接 | `Failed to enumerate cameras` | 检查 USB 连接，运行 `lsusb -d 2bc5:` |
| SDK 版本不匹配 | `SDK initialization failed` | 更新到正确的 Orbbec SDK 版本 |
| 编码器不可用 | `GStreamer encoder failed` | 检查 `gst-inspect-1.0 mpph264enc` |
| 网络不通 | `Connection timeout` | 检查接收端 IP 和网络路由 |
| Wi-Fi 频段错误 | Watchdog 卡住 | 运行 Wi-Fi guard 或手动切换到 5GHz |
| 配置文件错误 | `Config validation failed` | 运行 `--validate-config` 检查 |

### 11.9 完整启动检查脚本

```bash
#!/bin/bash
echo "=== 发送端启动状态检查 ==="
echo ""

echo "1. 服务状态："
systemctl is-active gwv3-gemini-sender.service
systemctl is-enabled gwv3-gemini-sender.service
echo ""

echo "2. 进程状态："
if pgrep gemini_sender >/dev/null; then
    echo "✓ 进程正在运行"
    ps aux | grep gemini_sender | grep -v grep
else
    echo "✗ 进程未运行"
fi
echo ""

echo "3. 相机连接："
if lsusb -d 2bc5: >/dev/null 2>&1; then
    echo "✓ 相机已识别"
    lsusb -d 2bc5:
else
    echo "✗ 相机未识别"
fi
echo ""

echo "4. 网络连接："
if ss -tn | grep -q 50010; then
    echo "✓ 已连接到接收端"
    ss -tn | grep 50010
else
    echo "✗ 未连接到接收端"
fi
echo ""

echo "5. 系统运行时间："
uptime
echo ""

echo "6. 服务启动时间："
systemctl show gwv3-gemini-sender.service | grep ActiveEnterTimestamp
echo ""

echo "7. 最近错误（最近 5 分钟）："
journalctl -u gwv3-gemini-sender.service --since "5 min ago" | grep -i error | tail -5
```

### 11.10 模拟重启测试

测试开机自启是否正常：

```bash
# 方法 1: 软重启（推荐）
sudo reboot

# 重启后检查
ssh cat@board-ip
systemctl status gwv3-gemini-sender.service
ps aux | grep gemini_sender

# 方法 2: 服务重启测试
sudo systemctl restart gwv3-gemini-sender.service
journalctl -u gwv3-gemini-sender.service -f

# 方法 3: 断电测试（最终验证）
sudo poweroff
# 物理断电后重新上电
# SSH 登录检查
```

---

## 12. 附录

### 11.1 端口分配表

| 端口 | 协议 | 用途 | 方向 |
|------|------|------|------|
| 50009 | UDP | 接收端自动发现 | Sender → Receiver |
| 50010 | TCP | RGB/Depth 媒体传输 | Sender → Receiver |
| 50011 | UDP | 状态和控制信息 | Sender ↔ Receiver |
| 50012 | UDP | 时钟同步 (CLOCK_SYNC) | Sender ↔ Receiver |

### 11.2 常用命令速查

```bash
# 编译
cmake -S . -B 12_build -DGWV3_BUILD_SENDER=ON
cmake --build 12_build -j2

# 配置验证
./12_build/bin/gemini_sender --config <config> --validate-config

# 前台运行
./05_tools/run_sender_foreground.sh <config>

# 服务管理
systemctl status gwv3-gemini-sender.service
journalctl -u gwv3-gemini-sender.service -f

# 硬件检查
lsusb -d 2bc5:
gst-inspect-1.0 mpph264enc
ls -la /dev/video*

# 性能监控
top -p $(pgrep gemini_sender)
ss -tn | grep 50010
```

### 11.3 相关文档链接

- [架构文档](architecture.md)
- [部署手册](deployment.md)
- [配置文档](configuration.md)
- [数据流水线](data-pipeline.md)
- [CLAUDE.md](../CLAUDE.md)

### 11.4 配置文件检查清单

部署前检查：
- [ ] `sender_id` 唯一且稳定
- [ ] 相机 `serial_number` 或 `uid` 正确
- [ ] `receiver.ip` 可路由
- [ ] 分辨率和帧率符合硬件能力
- [ ] 编码器类型和码率合理
- [ ] 日志目录存在且可写

---

**文档修订历史**：
- v1.0 (2026-09-23): 初始版本，基于代码和现有文档生成

**维护者**: 请根据实际部署经验补充和完善本文档
