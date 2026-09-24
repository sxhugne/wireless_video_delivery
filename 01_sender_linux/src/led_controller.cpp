#include "gwv3_sender/led_controller.hpp"
#include <chrono>
#include <cstring>

namespace gwv3 {

LedController::LedController(const std::string& chip_name, unsigned int line)
    : chip_name_(chip_name),
      line_(line),
      chip_(nullptr),
      gpio_line_(nullptr),
      initialized_(false) {

    // 打开 GPIO 芯片
    chip_ = gpiod_chip_open_by_name(chip_name_.c_str());
    if (!chip_) {
        last_error_ = "Failed to open GPIO chip '" + chip_name_ + "': " +
                      std::string(std::strerror(errno));
        return;
    }

    // 获取 GPIO 引脚
    gpio_line_ = gpiod_chip_get_line(chip_, line_);
    if (!gpio_line_) {
        last_error_ = "Failed to get GPIO line " + std::to_string(line_) +
                      " from chip '" + chip_name_ + "': " +
                      std::string(std::strerror(errno));
        gpiod_chip_close(chip_);
        chip_ = nullptr;
        return;
    }

    // 请求输出模式（初始状态：熄灭）
    int ret = gpiod_line_request_output(gpio_line_, "led-controller", LED_OFF_VALUE);
    if (ret < 0) {
        last_error_ = "Failed to request GPIO line as output: " +
                      std::string(std::strerror(errno)) +
                      " (需要 root 权限)";
        gpiod_chip_close(chip_);
        chip_ = nullptr;
        gpio_line_ = nullptr;
        return;
    }

    initialized_ = true;
    state_.store(State::OFF);
}

LedController::~LedController() {
    // 停止闪烁线程
    stopBlink();

    // 熄灭 LED
    if (initialized_) {
        setGpioValue(LED_OFF_VALUE);
    }

    // 释放 GPIO 资源
    if (gpio_line_) {
        gpiod_line_release(gpio_line_);
    }
    if (chip_) {
        gpiod_chip_close(chip_);
    }
}

bool LedController::turnOn() {
    if (!initialized_) {
        last_error_ = "LED controller not initialized";
        return false;
    }

    // 停止可能正在运行的闪烁
    stopBlink();

    if (setGpioValue(LED_ON_VALUE)) {
        state_.store(State::ON);
        return true;
    }
    return false;
}

bool LedController::turnOff() {
    if (!initialized_) {
        last_error_ = "LED controller not initialized";
        return false;
    }

    // 停止可能正在运行的闪烁
    stopBlink();

    if (setGpioValue(LED_OFF_VALUE)) {
        state_.store(State::OFF);
        return true;
    }
    return false;
}

bool LedController::blink(double period_sec) {
    if (!initialized_) {
        last_error_ = "LED controller not initialized";
        return false;
    }

    if (period_sec <= 0.0) {
        last_error_ = "Blink period must be positive";
        return false;
    }

    // 停止之前可能正在运行的闪烁
    stopBlink();

    // 启动闪烁线程
    blink_running_.store(true);
    state_.store(State::BLINKING);
    blink_thread_ = std::thread(&LedController::blinkLoop, this, period_sec);

    return true;
}

void LedController::stopBlink() {
    if (blink_running_.load()) {
        blink_running_.store(false);
        if (blink_thread_.joinable()) {
            blink_thread_.join();
        }
    }
}

void LedController::blinkLoop(double period) {
    bool led_state = false;  // false=灭, true=亮

    while (blink_running_.load()) {
        // 切换 LED 状态
        led_state = !led_state;
        setGpioValue(led_state ? LED_ON_VALUE : LED_OFF_VALUE);

        // 等待半周期
        auto sleep_duration = std::chrono::duration<double>(period);
        auto start = std::chrono::steady_clock::now();
        auto end = start + sleep_duration;

        // 使用短睡眠循环，以便快速响应停止信号
        while (std::chrono::steady_clock::now() < end && blink_running_.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    // 退出时熄灭 LED
    setGpioValue(LED_OFF_VALUE);
    state_.store(State::OFF);
}

bool LedController::setGpioValue(int value) {
    if (!gpio_line_) {
        last_error_ = "GPIO line not initialized";
        return false;
    }

    int ret = gpiod_line_set_value(gpio_line_, value);
    if (ret < 0) {
        last_error_ = "Failed to set GPIO value: " +
                      std::string(std::strerror(errno));
        return false;
    }

    return true;
}

}  // namespace gwv3
