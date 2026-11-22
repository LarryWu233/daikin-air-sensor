#include "cm1106_pro.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

#include <cinttypes>
#include <cmath>
#include <cstring>

namespace esphome {
namespace cm1106_pro {

ESPHOME_LOG_TAG(TAG, "cm1106_pro");

static constexpr uint8_t C_M1106_CMD_GET_CO2[4] = {0x11, 0x01, 0x01, 0xED};
static constexpr uint8_t C_M1106_CMD_SET_CO2_CALIB[6] = {0x11, 0x03, 0x03, 0x00, 0x00, 0x00};
static constexpr uint8_t C_M1106_CMD_SET_CO2_CALIB_RESPONSE[4] = {0x16, 0x01, 0x03, 0xE6};
// CM1106-C V0.6 ABC config: DF1=0x64 (reserved), DF2=switch (0x00=enable / 0x02=disable),
// DF3=cycle days, DF4/DF5=baseline (high/low), DF6=0x64 (reserved).
static constexpr uint8_t C_M1106_CMD_SET_ABC_STATUS[10] = {0x11, 0x07, 0x10, 0x64, 0x00, 0x0F, 0x01, 0x90, 0x64, 0x00};
static constexpr uint8_t C_M1106_CMD_SET_ABC_STATUS_RESPONSE[4] = {0x16, 0x01, 0x10, 0xD9};

static constexpr uint8_t CM1106_ABC_FLAG_ENABLE = 0x00;
static constexpr uint8_t CM1106_ABC_FLAG_DISABLE = 0x02;

// Checksum is an 8-bit additive checksum: CS = (256 - sum(bytes)) % 256,
// equivalently CS = (-sum(bytes)) & 0xFF. NOT a CRC-8 polynomial.
static uint8_t cm1106_pro_checksum(const uint8_t *data, size_t len) {
  return static_cast<uint8_t>(-sum_bytes(data, len - 1));
}

void CM1106ProComponent::setup() {
  uint8_t response[8] = {0};
  if (!this->cm1106_write_command_(C_M1106_CMD_GET_CO2, sizeof(C_M1106_CMD_GET_CO2), response, sizeof(response))) {
    this->mark_failed(LOG_STR(ESP_LOG_MSG_COMM_FAIL));
    return;
  }

  // Read the internal firmware version (0x1E) once at boot to confirm whether
  // this is a different protocol variant (the STATUS Bit0 semantics are inverted
  // on this hardware vs the CM1106-C V0.6 spec).
  this->read_firmware_version_();

  if (this->abc_boot_logic_ != CM1106_ABC_NONE) {
    this->abc_set_(this->abc_boot_logic_ == CM1106_ABC_ENABLED);
  }
}

void CM1106ProComponent::read_firmware_version_() {
  static constexpr uint8_t CMD_READ_FW[4] = {0x11, 0x01, 0x1E, 0x00};  // CS computed on write
  // Send the command without a fixed-length response read, then parse the
  // response by its LEN field: some units return 14 bytes (16 0B 1E + 10 data + CS,
  // no reserved byte), others 15 bytes (16 0C 1E + 11 data + CS).
  if (!this->cm1106_write_command_(CMD_READ_FW, sizeof(CMD_READ_FW), nullptr, 0)) {
    ESP_LOGW(TAG, "Read firmware version failed (send)");
    return;
  }
  uint8_t header[3] = {0};
  if (!this->read_array(header, 3)) {
    ESP_LOGW(TAG, "Read firmware version failed (no header)");
    return;
  }
  if (header[0] != 0x16 || header[2] != 0x1E) {
    ESP_LOGW(TAG, "FW version unexpected header: %02X %02X %02X", header[0], header[1], header[2]);
    return;
  }
  uint8_t len = header[1];
  if (len < 1 || len > 14) {
    ESP_LOGW(TAG, "FW version bad length: %u", len);
    return;
  }
  uint8_t data_len = len - 1;
  uint8_t buf[16] = {0};
  buf[0] = header[0];
  buf[1] = header[1];
  buf[2] = header[2];
  if (!this->read_array(&buf[3], data_len + 1)) {
    ESP_LOGW(TAG, "Read firmware version failed (no data)");
    return;
  }
  uint8_t total = 3 + data_len + 1;
  uint8_t checksum = cm1106_pro_checksum(buf, total);
  if (buf[total - 1] != checksum) {
    ESP_LOGW(TAG, "FW version checksum mismatch: got 0x%02X expected 0x%02X", buf[total - 1], checksum);
    return;
  }
  char fw[11] = {0};
  uint8_t copy = data_len < 10 ? data_len : 10;
  memcpy(fw, &buf[3], copy);
  ESP_LOGI(TAG, "CM1106 firmware version: \"%s\"", fw);
}

void CM1106ProComponent::read_serial_number_() {
  static constexpr uint8_t CMD_READ_SN[4] = {0x11, 0x01, 0x1F, 0x00};  // CS computed on write
  uint8_t response[14] = {0};  // 16 0B 1F + 10 data + CS
  if (!this->cm1106_write_command_(CMD_READ_SN, sizeof(CMD_READ_SN), response, sizeof(response))) {
    ESP_LOGW(TAG, "Read serial number failed (no response)");
    return;
  }
  if (response[0] != 0x16 || response[1] != 0x0B || response[2] != 0x1F) {
    ESP_LOGW(TAG, "SN unexpected response: %02X %02X %02X", response[0], response[1], response[2]);
    return;
  }
  uint8_t checksum = cm1106_pro_checksum(response, sizeof(response));
  if (response[13] != checksum) {
    ESP_LOGW(TAG, "SN checksum mismatch: got 0x%02X expected 0x%02X", response[13], checksum);
    return;
  }
  ESP_LOGI(TAG, "CM1106 serial number: %02X%02X %02X%02X %02X%02X %02X%02X %02X%02X", response[3], response[4],
           response[5], response[6], response[7], response[8], response[9], response[10], response[11], response[12]);
}

void CM1106ProComponent::read_version() {
  ESP_LOGI(TAG, "=== Reading CM1106 identity (0x1E + 0x1F) ===");
  this->read_firmware_version_();
  this->read_serial_number_();
}

bool CM1106ProComponent::is_preheating_(uint8_t status) const {
  // Per-device Bit0 polarity:
  //   inverted=false (V0.6 spec): 1 = preheating, 0 = running
  //   inverted=true (some units): 0 = preheating, 1 = running
  return this->bit0_inverted_ ? (status & 0x01) == 0 : (status & 0x01) != 0;
}

void CM1106ProComponent::log_status_(uint8_t status, uint16_t ppm) {
  // Log only on state CHANGE (avoid repeating the same line every 10 s). The raw
  // RX frame (with checksum) is already dumped every sample in update().
  if (status == this->last_status_)
    return;

  bool was_preheating = this->is_preheating_(this->last_status_);
  bool is_preheating = this->is_preheating_(status);

  if (this->last_status_ == 0xFF) {
    ESP_LOGI(TAG, "STATUS initial=%02X preheating=%s", status, is_preheating ? "yes" : "no");
  } else {
    ESP_LOGI(TAG, "STATUS transition %02X -> %02X (preheating %s -> %s)", this->last_status_, status,
             was_preheating ? "yes" : "no", is_preheating ? "yes" : "no");
  }

  // Bit1 is documented as "sensor error", but this hardware also sets it (with a
  // full-scale reading) once the concentration exceeds the measuring range - that
  // is out-of-range, not a fault. Only treat Bit1 below full scale as an error.
  if (status & 0x02) {
    if (ppm >= RANGE_MAX_PPM)
      ESP_LOGI(TAG, "Status: measurement above range (%uppm full scale)", ppm);
    else
      ESP_LOGW(TAG, "Status: sensor error");
  }
  if (status & 0x04)
    ESP_LOGI(TAG, "Status: over measurement range");
  if (status & 0x08)
    ESP_LOGI(TAG, "Status: below measurement range");
  if (status & 0x10)
    ESP_LOGW(TAG, "Status: not calibrated");
  if (status & 0x20)
    ESP_LOGW(TAG, "Status: light source aging");
  if (status & 0x40)
    ESP_LOGW(TAG, "Status: drift");
  if (status & 0x80)
    ESP_LOGW(TAG, "Status: reserved bit set");

  this->last_status_ = status;
}

void CM1106ProComponent::update() {
  const uint32_t now = millis();

  if (this->co2_errors_sensor_ != nullptr)
    this->co2_errors_sensor_->publish_state(this->error_count_);

  if (this->has_valid_) {
    uint32_t age_s = (now - this->last_valid_millis_) / 1000;
    if (this->co2_last_valid_age_sensor_ != nullptr)
      this->co2_last_valid_age_sensor_->publish_state(age_s);
    if ((now - this->last_valid_millis_) > STALE_THRESHOLD_MS) {
      if (!this->stale_warned_) {
        ESP_LOGW(TAG, "No valid CO2 data for >%us; invalidating reading",
                 static_cast<unsigned>(STALE_THRESHOLD_MS / 1000));
        this->stale_warned_ = true;
      }
      if (this->co2_sensor_ != nullptr)
        this->co2_sensor_->publish_state(NAN);
    }
  }

  uint8_t response[8] = {0};
  if (!this->cm1106_write_command_(C_M1106_CMD_GET_CO2, sizeof(C_M1106_CMD_GET_CO2), response, sizeof(response))) {
    ESP_LOGW(TAG, "Reading data failed!");
    this->error_count_++;
    this->status_set_warning();
    return;
  }

  if (response[0] != 0x16 || response[1] != 0x05 || response[2] != 0x01) {
    ESP_LOGW(TAG, "Unexpected response: %02X %02X %02X %02X", response[0], response[1], response[2], response[3]);
    this->error_count_++;
    this->status_set_warning();
    return;
  }

  uint8_t checksum = cm1106_pro_checksum(response, sizeof(response));

  // Raw frame dump BEFORE any state decision, so preheating/invalid frames are
  // also visible. Lets us tell "sensor really reports status=01" from "corrupt
  // UART frame" (checksum FAIL).
  ESP_LOGD(TAG, "RX[%ums] %02X %02X %02X %02X %02X %02X %02X %02X chk=%02X %s",
           (unsigned) now, response[0], response[1], response[2], response[3], response[4], response[5],
           response[6], response[7], checksum, response[7] == checksum ? "OK" : "FAIL");

  if (response[7] != checksum) {
    ESP_LOGW(TAG, "Checksum mismatch: got 0x%02X expected 0x%02X", response[7], checksum);
    this->error_count_++;
    this->status_set_warning();
    return;
  }

  const uint16_t ppm = encode_uint16(response[3], response[4]);
  const uint8_t status = response[5];

  if (this->co2_raw_sensor_ != nullptr)
    this->co2_raw_sensor_->publish_state(ppm);
  if (this->co2_status_sensor_ != nullptr)
    this->co2_status_sensor_->publish_state(status);

  this->log_status_(status, ppm);

  if (this->is_preheating_(status)) {
    // Preheating: track duration, warn once if it exceeds the timeout. Do NOT
    // auto-calibrate or reset the sensor.
    if (this->preheat_start_millis_ == 0)
      this->preheat_start_millis_ = now;
    if (!this->preheat_5min_warned_ && (now - this->preheat_start_millis_) > PREHEAT_TIMEOUT_MS) {
      ESP_LOGW(TAG, "Preheating not complete after %us - check sensor power/supply",
               static_cast<unsigned>(PREHEAT_TIMEOUT_MS / 1000));
      this->preheat_5min_warned_ = true;
    }
    this->status_set_warning();
    return;
  }

  // This CM1106 variant signals an out-of-range measurement with STATUS Bit1 plus
  // a full-scale reading (verified frame: 16 05 01 13 88 03 -> 5000 ppm, status
  // 0x03). Only Bit1 below full scale is a genuine sensor error; range-flag
  // samples stay valid and keep being published as a bounded value, otherwise HA
  // would drop to unavailable and the OLED would show "--".
  const bool sensor_error = (status & 0x02) != 0;
  const bool above_range = (status & 0x04) != 0 || (sensor_error && ppm >= RANGE_MAX_PPM);

  if (sensor_error && !above_range) {
    this->error_count_++;
    this->status_set_warning();
    return;
  }

  // Preheating completed (status Bit0 now 0): log the duration, reset trackers.
  if (this->preheat_start_millis_ != 0) {
    ESP_LOGI(TAG, "Preheating complete after %ums", (unsigned) (now - this->preheat_start_millis_));
    this->preheat_start_millis_ = 0;
    this->preheat_5min_warned_ = false;
  }

  this->has_valid_ = true;
  this->last_valid_millis_ = now;
  this->stale_warned_ = false;
  if (this->co2_sensor_ != nullptr)
    this->co2_sensor_->publish_state(ppm);
  if (this->co2_last_valid_age_sensor_ != nullptr)
    this->co2_last_valid_age_sensor_->publish_state(0);
  this->status_clear_warning();
  ESP_LOGD(TAG, "CO2=%uppm status=%02X", ppm, status);
}

void CM1106ProComponent::calibrate_zero(uint16_t ppm) {
  uint8_t pre[8] = {0};
  uint16_t pre_ppm = 0;
  bool have_pre = false;
  if (this->cm1106_write_command_(C_M1106_CMD_GET_CO2, sizeof(C_M1106_CMD_GET_CO2), pre, sizeof(pre)) && pre[0] == 0x16 &&
      pre[1] == 0x05 && pre[2] == 0x01 && pre[7] == cm1106_pro_checksum(pre, sizeof(pre))) {
    pre_ppm = encode_uint16(pre[3], pre[4]);
    have_pre = true;
  }

  if (have_pre) {
    ESP_LOGI(TAG, "Manual calibration: target=%uppm, pre-reading=%uppm", ppm, pre_ppm);
  } else {
    ESP_LOGI(TAG, "Manual calibration: target=%uppm, pre-reading unavailable", ppm);
  }

  uint8_t cmd[6];
  memcpy(cmd, C_M1106_CMD_SET_CO2_CALIB, sizeof(cmd));
  cmd[3] = ppm >> 8;
  cmd[4] = ppm & 0xFF;
  if (this->send_command_expect_ack_(cmd, sizeof(cmd), C_M1106_CMD_SET_CO2_CALIB_RESPONSE)) {
    ESP_LOGI(TAG, "Calibration to %uppm acknowledged (%02X %02X %02X %02X)", ppm,
             C_M1106_CMD_SET_CO2_CALIB_RESPONSE[0], C_M1106_CMD_SET_CO2_CALIB_RESPONSE[1],
             C_M1106_CMD_SET_CO2_CALIB_RESPONSE[2], C_M1106_CMD_SET_CO2_CALIB_RESPONSE[3]);
  } else {
    ESP_LOGW(TAG, "Calibration to %uppm failed", ppm);
  }
}

void CM1106ProComponent::abc_set_(bool enabled) {
  uint8_t cmd[10];
  memcpy(cmd, C_M1106_CMD_SET_ABC_STATUS, sizeof(cmd));
  cmd[4] = enabled ? CM1106_ABC_FLAG_ENABLE : CM1106_ABC_FLAG_DISABLE;
  cmd[5] = this->abc_cycle_;
  cmd[6] = this->abc_baseline_ >> 8;
  cmd[7] = this->abc_baseline_ & 0xFF;

  ESP_LOGI(TAG, "TX ABC 0x10 @%ums: %s (cycle=%u d, baseline=%u ppm)", (unsigned) millis(),
           enabled ? "enable" : "disable", this->abc_cycle_, this->abc_baseline_);

  if (this->send_command_expect_ack_(cmd, sizeof(cmd), C_M1106_CMD_SET_ABC_STATUS_RESPONSE)) {
    ESP_LOGI(TAG, "ABC 0x10 ACK OK @%ums", (unsigned) millis());
  } else {
    ESP_LOGW(TAG, "ABC 0x10 ACK failed @%ums", (unsigned) millis());
  }
}

bool CM1106ProComponent::send_command_expect_ack_(const uint8_t *command, size_t command_len, const uint8_t *ack) {
  uint8_t response[4] = {0};
  if (!this->cm1106_write_command_(command, command_len, response, sizeof(response))) {
    ESP_LOGW(TAG, "Command %02X failed", command[2]);
    this->status_set_warning(LOG_STR(ESP_LOG_MSG_COMM_FAIL));
    return false;
  }
  if (!this->status_comm_result(memcmp(response, ack, sizeof(response)) == 0)) {
    ESP_LOGW(TAG, "Unexpected response: %02X %02X %02X %02X", response[0], response[1], response[2], response[3]);
    return false;
  }
  return true;
}

bool CM1106ProComponent::cm1106_write_command_(const uint8_t *command, size_t command_len, uint8_t *response,
                                               size_t response_len) {
  this->discard_available();
  this->write_array(command, command_len - 1);
  this->write_byte(cm1106_pro_checksum(command, command_len));
  this->flush();

  if (response == nullptr)
    return true;

  return this->read_array(response, response_len);
}

void CM1106ProComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "CM1106 Pro:");
  LOG_SENSOR("  ", "CO2", this->co2_sensor_);
  LOG_SENSOR("  ", "CO2 Raw", this->co2_raw_sensor_);
  LOG_SENSOR("  ", "CO2 Status", this->co2_status_sensor_);
  LOG_SENSOR("  ", "CO2 Errors", this->co2_errors_sensor_);
  LOG_SENSOR("  ", "CO2 Last Valid Age", this->co2_last_valid_age_sensor_);
  if (this->abc_boot_logic_ != CM1106_ABC_NONE) {
    ESP_LOGCONFIG(TAG,
                  "  Automatic baseline calibration on boot: %s\n"
                  "  ABC calibration cycle: %u days\n"
                  "  ABC baseline: %uppm",
                  ONOFF(this->abc_boot_logic_ == CM1106_ABC_ENABLED), this->abc_cycle_, this->abc_baseline_);
  }
}

}  // namespace cm1106_pro
}  // namespace esphome


