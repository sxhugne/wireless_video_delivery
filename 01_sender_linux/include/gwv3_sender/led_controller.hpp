#pragma once

#include <gpiod.h>
#include <atomic>
#include <memory>
#include <string>
#include <thread>

namespace gwv3 {

/// LED 状态指示器
/// 控制 LubanCat 3IO 底板上的 LED (GPIO0_D0, line 24)
///
/// 硬件规格:
/// - GPIO 芯片: gpiochip0
/// - GPIO 引脚: GPIO0_D0 (line 24)
/// - 极性: ACTIVE-LOW (0=亮, 1=灭)
/// - 需要 root 权限
class LedController {
public:
    enum class State {
        OFF,       // 熄灭
        ON,        // 常亮
        BLINKING   // 闪烁中
    };

    /// 构造函数
    /// @param chip_name GPIO 芯片名称 (默认 "gpiochip0")
    /// @param line GPIO 引脚编号 (默认 24 = GPIO0_D0)
    explicit LedController(const std::string& chip_name = "gpiochip0",
                          unsigned int line = 24);

    ~LedController();

    // 禁止拷贝和移动
    LedController(const LedController&) = delete;
    LedController& operator=(const LedController&) = delete;

    /// 点亮 LED
    /// @return true=成功, false=失败
    bool turnOn();

    /// 熄灭 LED
    /// @return true=成功, false=失败
    bool turnOff();

    /// 开始闪烁
    /// @param period_sec 半周期时间(秒)，例如 0.5 表示 1Hz 闪烁
    /// @return true=成功, false=失败
    bool blink(double period_sec = 0.5);

    /// 停止闪烁并熄灭
    void stopBlink();

    /// 查询当前状态
    State getState() const { return state_.load(); }

    /// 是否正在闪烁
    bool isBlinking() const { return state_.load() == State::BLINKING; }

    /// 是否已初始化成功
    bool isInitialized() const { return initialized_; }

    /// 获取最后的错误信息
    std::string getLastError() const { return last_error_; }

private:
    void blinkLoop(double period);
    bool setGpioValue(int value);

    // GPIO 硬件参数
    std::string chip_name_;
    unsigned int line_;
    static constexpr int LED_ON_VALUE = 0;   // ACTIVE-LOW: 0=亮
    static constexpr int LED_OFF_VALUE = 1;  // ACTIVE-LOW: 1=灭

    // libgpiod 资源
    struct gpiod_chip* chip_;
    struct gpiod_line* gpio_line_;

    // 状态管理
    std::atomic<State> state_{State::OFF};
    std::atomic<bool> blink_running_{false};
    std::thread blink_thread_;
    bool initialized_;
    std::string last_error_;
};

}  // namespace gwv3
