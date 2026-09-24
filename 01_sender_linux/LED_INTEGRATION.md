# LED 控制器集成指南

## 概述

LED 控制器用于通过 GPIO 控制 LubanCat 3IO 开发板上的 LED 状态指示灯。

## 硬件信息

根据《LED控制-交付说明-20260924.md》：

- **GPIO 芯片**: `gpiochip0`
- **GPIO 引脚**: GPIO0_D0 (libgpiod line 24)
- **极性**: ACTIVE-LOW (0=亮, 1=灭)
- **权限要求**: 需要 root 权限

## 使用方法

### 1. 包含头文件

```cpp
#include "gwv3_sender/led_controller.hpp"
```

### 2. 创建 LED 控制器实例

```cpp
// 使用默认参数 (gpiochip0, line 24)
auto led = std::make_unique<gwv3::LedController>();

// 检查初始化是否成功
if (!led->isInitialized()) {
    std::cerr << "LED 初始化失败: " << led->getLastError() << std::endl;
    // 注意：LED 失败不应阻塞主程序运行
}
```

### 3. 基础控制

```cpp
// 点亮 LED
if (led->turnOn()) {
    std::cout << "LED 已点亮" << std::endl;
}

// 熄灭 LED
if (led->turnOff()) {
    std::cout << "LED 已熄灭" << std::endl;
}
```

### 4. 闪烁控制

```cpp
// 开始闪烁 (默认 1Hz，半周期 0.5 秒)
led->blink();

// 快速闪烁 (半周期 0.2 秒 = 2.5Hz)
led->blink(0.2);

// 慢速闪烁 (半周期 1.0 秒 = 0.5Hz)
led->blink(1.0);

// 停止闪烁
led->stopBlink();
```

### 5. 状态查询

```cpp
// 获取当前状态
auto state = led->getState();
switch (state) {
    case gwv3::LedController::State::OFF:
        std::cout << "LED 已熄灭" << std::endl;
        break;
    case gwv3::LedController::State::ON:
        std::cout << "LED 常亮" << std::endl;
        break;
    case gwv3::LedController::State::BLINKING:
        std::cout << "LED 闪烁中" << std::endl;
        break;
}

// 检查是否正在闪烁
if (led->isBlinking()) {
    std::cout << "LED 正在闪烁" << std::endl;
}
```

## 集成示例：在 Application 中使用

### 示例 1: 基础状态指示

```cpp
class SenderApplication {
private:
    std::unique_ptr<gwv3::LedController> led_;
    
public:
    void init() {
        // 初始化 LED 控制器
        led_ = std::make_unique<gwv3::LedController>();
        
        if (led_->isInitialized()) {
            // 启动时点亮 LED，表示系统正在启动
            led_->turnOn();
        } else {
            // LED 失败只记录日志，不影响主功能
            logger_.warn("LED controller init failed: " + led_->getLastError());
        }
    }
    
    void run() {
        if (led_->isInitialized()) {
            // 正常运行时慢速闪烁
            led_->blink(1.0);
        }
        
        // 主业务逻辑...
    }
    
    void shutdown() {
        if (led_->isInitialized()) {
            // 关闭时熄灭 LED
            led_->turnOff();
        }
    }
};
```

### 示例 2: 错误状态指示

```cpp
void onCameraError() {
    if (led_ && led_->isInitialized()) {
        // 相机错误时快速闪烁 (0.2 秒半周期)
        led_->blink(0.2);
        logger_.error("Camera error - LED fast blinking");
    }
}

void onNetworkError() {
    if (led_ && led_->isInitialized()) {
        // 网络错误时中速闪烁 (0.5 秒半周期)
        led_->blink(0.5);
        logger_.error("Network error - LED medium blinking");
    }
}

void onRecovery() {
    if (led_ && led_->isInitialized()) {
        // 恢复正常后恢复慢速闪烁
        led_->blink(1.0);
        logger_.info("System recovered - LED normal blinking");
    }
}
```

### 示例 3: 完整的生命周期管理

```cpp
int main() {
    // 创建 LED 控制器
    auto led = std::make_unique<gwv3::LedController>();
    
    if (!led->isInitialized()) {
        std::cerr << "Warning: LED not available: " 
                  << led->getLastError() << std::endl;
        // 继续运行，LED 不是关键功能
    }
    
    try {
        // 1. 启动阶段 - 常亮
        if (led->isInitialized()) {
            led->turnOn();
        }
        
        // 初始化系统...
        
        // 2. 运行阶段 - 慢闪
        if (led->isInitialized()) {
            led->blink(1.0);
        }
        
        // 主循环运行...
        while (running) {
            // 检测到错误
            if (has_error && led->isInitialized()) {
                led->blink(0.2);  // 快闪表示错误
            }
            
            // 业务逻辑...
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        
        // 3. 异常退出 - 熄灭
        if (led && led->isInitialized()) {
            led->turnOff();
        }
        return 1;
    }
    
    // 4. 正常退出 - 熄灭
    if (led && led->isInitialized()) {
        led->turnOff();
    }
    
    return 0;
}
```

## 注意事项

### 1. 权限要求

LED 控制需要 root 权限。运行程序时使用：

```bash
sudo ./gemini_sender --config config.json
```

### 2. 错误处理

LED 控制失败不应阻塞主程序运行。始终检查初始化状态：

```cpp
if (led->isInitialized()) {
    // 安全使用 LED
} else {
    // 记录日志但继续运行
    logger.warn("LED unavailable: " + led->getLastError());
}
```

### 3. 资源清理

`LedController` 析构函数会自动：
- 停止闪烁线程
- 熄灭 LED
- 释放 GPIO 资源

不需要手动清理。

### 4. 硬件冲突风险

⚠️ 根据交付文档，GPIO0_D0 被以下外设声明：
- SPI0 (当前 disabled)
- PDM0 (当前 disabled)
- SAI0 (当前 disabled)

**如果将来在设备树中启用这些外设，LED 将失效！**

### 5. 线程安全

- `turnOn()`, `turnOff()`, `blink()` 是线程安全的
- 可以从不同线程调用控制方法

## 编译配置

CMakeLists.txt 已自动配置，包含：

```cmake
# 查找 libgpiod
pkg_check_modules(GPIOD REQUIRED libgpiod>=1.6)

# 添加源文件
add_executable(gemini_sender
    src/led_controller.cpp
    # ...其他文件
)

# 链接库
target_link_libraries(gemini_sender PRIVATE
    ${GPIOD_LIBRARIES}
    # ...其他库
)
```

## 编译和运行

### 在板子上编译

```bash
# 1. 安装 libgpiod 开发库
sudo apt install libgpiod-dev

# 2. 配置 CMake
cmake -S . -B 12_build_sender \
  -DGWV3_BUILD_RECEIVER=OFF \
  -DGWV3_BUILD_SENDER=ON \
  -DBUILD_TESTING=ON

# 3. 编译
cmake --build 12_build_sender -j2

# 4. 运行 (需要 root)
sudo ./12_build_sender/bin/gemini_sender --config 06_configs/xxx.json
```

## LED 状态语义建议

| 状态 | LED 模式 | 含义 |
|------|----------|------|
| 启动中 | 常亮 | 系统正在初始化 |
| 正常运行 | 慢闪 (1.0s) | 采集和发送正常 |
| 轻微警告 | 中速闪 (0.5s) | 网络抖动、临时错误 |
| 严重错误 | 快闪 (0.2s) | 相机断开、编码失败 |
| 关闭 | 熄灭 | 系统已停止 |

## 与系统命令的关系

文档提到板子上已部署 `/usr/local/bin/led` 命令。我们的 C++ 实现：

- ✅ **直接使用 libgpiod**，不依赖外部命令
- ✅ **性能更好**，无需 fork 进程
- ✅ **更可靠**，不依赖 shell 脚本
- ✅ **类型安全**，编译期检查

两者底层都操作同一个 GPIO 引脚，**不要同时使用**。

## 故障排查

### 错误：Failed to open GPIO chip

```
Failed to open GPIO chip 'gpiochip0': No such file or directory
```

**解决方法**：
1. 检查设备是否存在：`ls /dev/gpiochip0`
2. 检查内核模块：`lsmod | grep gpio`

### 错误：Failed to request GPIO line as output

```
Failed to request GPIO line as output: Operation not permitted
```

**解决方法**：
1. 使用 `sudo` 运行程序
2. 检查引脚是否被占用：`gpioinfo gpiochip0 | grep -A5 "line  24"`

### 错误：Device or resource busy

```
Failed to request GPIO line as output: Device or resource busy
```

**解决方法**：
1. 检查是否有其他程序在使用该引脚
2. 停止可能冲突的服务：`sudo systemctl stop led.service`
3. 检查设备树中 SPI0/PDM0/SAI0 是否被启用

## 参考资料

- 交付文档：`LED控制-交付说明-20260924.md`
- libgpiod 文档：https://git.kernel.org/pub/scm/libs/libgpiod/libgpiod.git/about/
- GPIO 引脚定义：GPIO0_D0 = gpiochip0 line 24
