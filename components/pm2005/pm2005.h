#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome::pm2005 {

enum SensorType {
  PM2005,
  PM2105,
};

class PM2005Component final : public PollingComponent, public i2c::I2CDevice {
  SUB_SENSOR(pm_1_0)
  SUB_SENSOR(pm_2_5)
  SUB_SENSOR(pm_10_0)
  SUB_SENSOR(last_communication_age)
  SUB_SENSOR(last_valid_age)

 public:
  void set_sensor_type(SensorType sensor_type) { this->sensor_type_ = sensor_type; }

  void setup() override;
  void dump_config() override;
  void update() override;

 protected:
  void publish_ages_(uint32_t now);
  void invalidate_reading_();

  static constexpr uint8_t COMMUNICATION_FAILURE_LIMIT = 3;
  uint8_t sensor_situation_{0};
  uint8_t data_buffer_[12];
  SensorType sensor_type_{PM2005};
  uint8_t communication_failures_{0};
  bool has_communication_{false};
  bool has_valid_result_{false};
  bool reading_available_{false};
  uint32_t last_communication_ms_{0};
  uint32_t last_valid_result_ms_{0};
  uint16_t last_pm1_{0};
  uint16_t last_pm25_{0};
  uint16_t last_pm10_{0};

  uint8_t situation_value_index_{3};
  uint8_t pm_1_0_value_index_{4};
  uint8_t pm_2_5_value_index_{6};
  uint8_t pm_10_0_value_index_{8};
  uint8_t measuring_value_index_{10};
};

}  // namespace esphome::pm2005
