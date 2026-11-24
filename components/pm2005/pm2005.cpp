#include "esphome/core/log.h"
#include "pm2005.h"

#include <cmath>

namespace esphome::pm2005 {

ESPHOME_LOG_TAG(TAG, "pm2005");

// Converts a sensor situation to a human readable string
static const LogString *pm2005_get_situation_string(int status) {
  switch (status) {
    case 1:
      return LOG_STR("Close");
    case 2:
      return LOG_STR("Malfunction");
    case 3:
      return LOG_STR("Under detecting");
    case 0x80:
      return LOG_STR("Detecting completed");
    default:
      return LOG_STR("Invalid");
  }
}

// Converts a sensor measuring mode to a human readable string
static const LogString *pm2005_get_measuring_mode_string(int status) {
  switch (status) {
    case 2:
      return LOG_STR("Single");
    case 3:
      return LOG_STR("Continuous");
    case 5:
      return LOG_STR("Dynamic");
    default:
      return LOG_STR("Timing");
  }
}

static inline uint16_t get_sensor_value(const uint8_t *data, uint8_t i) { return data[i] * 0x100 + data[i + 1]; }

void PM2005Component::setup() {
  if (this->sensor_type_ == PM2005) {
    this->situation_value_index_ = 3;
    this->pm_1_0_value_index_ = 4;
    this->pm_2_5_value_index_ = 6;
    this->pm_10_0_value_index_ = 8;
    this->measuring_value_index_ = 10;
  } else {
    this->situation_value_index_ = 2;
    this->pm_1_0_value_index_ = 3;
    this->pm_2_5_value_index_ = 5;
    this->pm_10_0_value_index_ = 7;
    this->measuring_value_index_ = 9;
  }

  this->update();
  for (uint32_t delay_ms : {1000U, 2000U}) {
    this->set_timeout(delay_ms, [this]() {
      if (!this->has_communication_)
        this->update();
    });
  }
}

void PM2005Component::update() {
  const uint32_t now = millis();
  if (this->read(this->data_buffer_, sizeof(this->data_buffer_)) != i2c::ERROR_OK) {
    if (this->communication_failures_ < COMMUNICATION_FAILURE_LIMIT) {
      ++this->communication_failures_;
      ESP_LOGW(TAG, "Read result failed (%u/%u)", this->communication_failures_, COMMUNICATION_FAILURE_LIMIT);
    }
    this->status_set_warning();
    if (this->communication_failures_ >= COMMUNICATION_FAILURE_LIMIT)
      this->invalidate_reading_();
    this->publish_ages_(now);
    return;
  }

  this->has_communication_ = true;
  this->communication_failures_ = 0;
  this->last_communication_ms_ = now;

  const uint8_t situation = this->data_buffer_[this->situation_value_index_];
  const bool situation_changed = situation != this->sensor_situation_;
  this->sensor_situation_ = situation;
  if (situation_changed)
    ESP_LOGD(TAG, "Sensor situation: %s.", LOG_STR_ARG(pm2005_get_situation_string(situation)));
  if (this->sensor_situation_ == 2) {
    this->status_set_warning();
    this->invalidate_reading_();
    this->publish_ages_(now);
    return;
  }
  this->status_clear_warning();
  if (this->sensor_situation_ != 0x80) {
    // An idle or measuring module can still supply a usable previous result.
    this->publish_ages_(now);
    return;
  }

  const uint16_t pm1 = get_sensor_value(this->data_buffer_, this->pm_1_0_value_index_);
  const uint16_t pm25 = get_sensor_value(this->data_buffer_, this->pm_2_5_value_index_);
  const uint16_t pm10 = get_sensor_value(this->data_buffer_, this->pm_10_0_value_index_);
  const uint16_t sensor_measuring_mode = get_sensor_value(this->data_buffer_, this->measuring_value_index_);
  if (this->reading_available_ && !situation_changed && sensor_measuring_mode != 3 && pm1 == this->last_pm1_ &&
      pm25 == this->last_pm25_ && pm10 == this->last_pm10_) {
    this->publish_ages_(now);
    return;
  }

  this->last_pm1_ = pm1;
  this->last_pm25_ = pm25;
  this->last_pm10_ = pm10;
  this->has_valid_result_ = true;
  this->reading_available_ = true;
  this->last_valid_result_ms_ = now;
  ESP_LOGD(TAG, "PM1.0: %d, PM2.5: %d, PM10: %d, Measuring mode: %s.", pm1, pm25, pm10,
           LOG_STR_ARG(pm2005_get_measuring_mode_string(sensor_measuring_mode)));

  PUBLISH_STATE(this->pm_1_0_sensor_, pm1);
  PUBLISH_STATE(this->pm_2_5_sensor_, pm25);
  PUBLISH_STATE(this->pm_10_0_sensor_, pm10);

  this->publish_ages_(now);
}

void PM2005Component::publish_ages_(uint32_t now) {
  PUBLISH_STATE(this->last_communication_age_sensor_,
                this->has_communication_ ? (now - this->last_communication_ms_) / 1000.0f : NAN);
  PUBLISH_STATE(this->last_valid_age_sensor_,
                this->has_valid_result_ ? (now - this->last_valid_result_ms_) / 1000.0f : NAN);
}

void PM2005Component::invalidate_reading_() {
  if (!this->reading_available_)
    return;
  this->reading_available_ = false;
  PUBLISH_STATE(this->pm_1_0_sensor_, NAN);
  PUBLISH_STATE(this->pm_2_5_sensor_, NAN);
  PUBLISH_STATE(this->pm_10_0_sensor_, NAN);
}

void PM2005Component::dump_config() {
  ESP_LOGCONFIG(TAG,
                "PM2005:\n"
                "  Type: PM2%u05",
                this->sensor_type_ == PM2105);

  LOG_I2C_DEVICE(this);

  LOG_SENSOR("  ", "PM1.0", this->pm_1_0_sensor_);
  LOG_SENSOR("  ", "PM2.5", this->pm_2_5_sensor_);
  LOG_SENSOR("  ", "PM10 ", this->pm_10_0_sensor_);
  LOG_SENSOR("  ", "Last communication age", this->last_communication_age_sensor_);
  LOG_SENSOR("  ", "Last valid result age", this->last_valid_age_sensor_);
}

}  // namespace esphome::pm2005
