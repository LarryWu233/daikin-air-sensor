#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace boot_delay {

/// Blocks startup so the 4-pin SSD1306 (whose internal RESET pin is floating /
/// RC-coupled and whose power-on reset is unreliable) has time to become ready
/// before the I2C bus scan and display init run. The delay is split into short
/// slices and the task watchdog is fed each slice, so a multi-second wait does
/// not trigger a WDT reset / boot loop.
class BootDelay : public Component {
 public:
  void set_duration(uint32_t duration_ms) { this->duration_ms_ = duration_ms; }

  float get_setup_priority() const override { return setup_priority::BUS + 1.0f; }

  void setup() override {
    uint32_t elapsed = 0;
    while (elapsed < this->duration_ms_) {
      delay(100);
      elapsed += 100;
      arch_feed_wdt();
    }
  }

 protected:
  uint32_t duration_ms_{5000};
};

}  // namespace boot_delay
}  // namespace esphome
