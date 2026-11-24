#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace cm1106_pro {

// ABC logic requested at boot; CM1106_ABC_NONE means nothing is sent and the
// sensor keeps its stored setting.
enum CM1106ABCLogic : uint8_t {
  CM1106_ABC_NONE = 0,
  CM1106_ABC_ENABLED,
  CM1106_ABC_DISABLED,
};

class CM1106ProComponent : public PollingComponent, public uart::UARTDevice {
  SUB_SENSOR(co2)
  SUB_SENSOR(co2_raw)
  SUB_SENSOR(co2_status)
  SUB_SENSOR(co2_errors)
  SUB_SENSOR(co2_last_valid_age)

 public:
  void setup() override;
  void update() override;
  void dump_config() override;

  void calibrate_zero(uint16_t ppm);
  void abc_enable() { this->abc_set_(true); }
  void abc_disable() { this->abc_set_(false); }
  void read_version();

  void set_abc(bool enabled, uint8_t cycle_days, uint16_t baseline_ppm) {
    this->abc_boot_logic_ = enabled ? CM1106_ABC_ENABLED : CM1106_ABC_DISABLED;
    this->abc_cycle_ = cycle_days;
    this->abc_baseline_ = baseline_ppm;
  }

  // Per-device: some CM1106 units report STATUS Bit0 inverted (0=preheating/1=running),
  // others follow the V0.6 spec (1=preheating/0=running). Default = V0.6 spec (not inverted).
  void set_bit0_inverted(bool inverted) { this->bit0_inverted_ = inverted; }

 protected:
  bool cm1106_write_command_(const uint8_t *command, size_t command_len, uint8_t *response, size_t response_len);
  bool read_co2_frame_(uint8_t (&response)[8]);
  bool send_command_expect_ack_(const uint8_t *command, size_t command_len, const uint8_t *ack);
  void abc_set_(bool enabled);
  void log_status_(uint8_t status, uint16_t ppm);
  void read_firmware_version_();
  void read_serial_number_();
  bool is_preheating_(uint8_t status) const;

  CM1106ABCLogic abc_boot_logic_{CM1106_ABC_NONE};
  uint8_t abc_cycle_{15};
  uint16_t abc_baseline_{400};

  bool initialized_{false};
  bool has_valid_{false};
  uint32_t last_valid_millis_{0};
  uint32_t error_count_{0};
  static constexpr uint32_t STALE_THRESHOLD_MS = 30000;
  static constexpr uint32_t PREHEAT_TIMEOUT_MS = 300000;  // 5 min
  // CM1106-C measurement range is 400-5000 ppm; the sensor reports this value
  // together with STATUS Bit1 when the concentration leaves the range.
  static constexpr uint16_t RANGE_MAX_PPM = 5000;

  // Diagnostic state tracking (no logic changes, only logging dedup + timestamps).
  uint8_t last_status_{0xFF};         // previous STATUS byte; 0xFF = first sample
  uint32_t preheat_start_millis_{0};  // when the current preheat window began (0 = not preheating)
  bool preheat_5min_warned_{false};   // 5-min preheat timeout warning already emitted
  bool stale_warned_{false};          // stale-data warning already emitted
  bool bit0_inverted_{false};         // per-device Bit0 polarity (default = V0.6: 1=preheating)
};

/// Action to trigger a manual CO2 calibration to an arbitrary target ppm (400-1500).
template<typename... Ts>
class CalibrateZeroAction : public Action<Ts...>, public Parented<CM1106ProComponent> {
 public:
  TEMPLATABLE_VALUE(uint16_t, ppm)

  void play(Ts... x) override { this->parent_->calibrate_zero(this->ppm_.value(x...)); }
};

}  // namespace cm1106_pro
}  // namespace esphome
