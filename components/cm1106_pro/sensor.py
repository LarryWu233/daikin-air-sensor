"""CM1106 Pro sensor component (extended CM1106 with status diagnostics).

Extends the official CM1106 driver with:
- co2               : valid CO2 value only (published as NaN when invalid/stale)
- co2_raw           : raw ppm regardless of status (for diagnostics)
- co2_status        : raw STATUS byte bitmask (0-255)
- co2_errors        : cumulative error counter
- co2_last_valid_age: seconds since the last valid measurement

Preheat and sensor errors suppress CO2 readings. Range limits are published as
reported by the module. Calibration, light-aging and drift flags are logged.
"""

from esphome import automation
from esphome.automation import maybe_simple_id
import esphome.codegen as cg
from esphome.components import sensor, uart
from esphome.components.const import CONF_AUTOMATIC_BASELINE_CALIBRATION
import esphome.config_validation as cv
from esphome.const import (
    CONF_BASELINE,
    CONF_CO2,
    CONF_CYCLE,
    CONF_ID,
    DEVICE_CLASS_CARBON_DIOXIDE,
    ICON_MOLECULE_CO2,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,
    UNIT_PARTS_PER_MILLION,
    UNIT_SECOND,
)
from esphome.types import ConfigType
from esphome.core import ID
from esphome.cpp_generator import MockObj, TemplateArgsType

DEPENDENCIES = ["uart"]

CONF_CO2_RAW = "co2_raw"
CONF_CO2_STATUS = "co2_status"
CONF_CO2_ERRORS = "co2_errors"
CONF_CO2_LAST_VALID_AGE = "co2_last_valid_age"
CONF_PPM = "ppm"
CONF_BIT0_INVERTED = "bit0_inverted"


cm1106_pro_ns = cg.esphome_ns.namespace("cm1106_pro")
CM1106ProComponent = cm1106_pro_ns.class_(
    "CM1106ProComponent", cg.PollingComponent, uart.UARTDevice
)
CalibrateZeroAction = cm1106_pro_ns.class_("CalibrateZeroAction", automation.Action)


# CM1106-C V0.6: cycle 1-30 days (default 15), baseline 400-1500 ppm (default 400).
_ABC_SETTINGS_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_CYCLE, default=15): cv.int_range(min=1, max=30),
        cv.Optional(CONF_BASELINE, default=400): cv.int_range(min=400, max=1500),
    }
)


CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(CM1106ProComponent),
            cv.Optional(CONF_CO2): sensor.sensor_schema(
                unit_of_measurement=UNIT_PARTS_PER_MILLION,
                icon=ICON_MOLECULE_CO2,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_CARBON_DIOXIDE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_CO2_RAW): sensor.sensor_schema(
                unit_of_measurement=UNIT_PARTS_PER_MILLION,
                icon=ICON_MOLECULE_CO2,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_CARBON_DIOXIDE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_CO2_STATUS): sensor.sensor_schema(
                accuracy_decimals=0,
            ),
            cv.Optional(CONF_CO2_ERRORS): sensor.sensor_schema(
                accuracy_decimals=0,
                state_class=STATE_CLASS_TOTAL_INCREASING,
            ),
            cv.Optional(CONF_CO2_LAST_VALID_AGE): sensor.sensor_schema(
                unit_of_measurement=UNIT_SECOND,
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_AUTOMATIC_BASELINE_CALIBRATION): cv.Any(
                cv.boolean, _ABC_SETTINGS_SCHEMA
            ),
            cv.Optional(CONF_BIT0_INVERTED, default=False): cv.boolean,
        },
    )
    .extend(cv.polling_component_schema("10s"))
    .extend(uart.UART_DEVICE_SCHEMA)
)


FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "cm1106_pro",
    baud_rate=9600,
    data_bits=8,
    parity="NONE",
    stop_bits=1,
)


async def to_code(config: ConfigType) -> None:
    var = await uart.new_uart_component(config)
    sensors = sensor.sub_sensors(config)
    await sensors(CONF_CO2, var.set_co2_sensor)
    await sensors(CONF_CO2_RAW, var.set_co2_raw_sensor)
    await sensors(CONF_CO2_STATUS, var.set_co2_status_sensor)
    await sensors(CONF_CO2_ERRORS, var.set_co2_errors_sensor)
    await sensors(CONF_CO2_LAST_VALID_AGE, var.set_co2_last_valid_age_sensor)

    if (abc := config.get(CONF_AUTOMATIC_BASELINE_CALIBRATION)) is not None:
        enabled = abc if isinstance(abc, bool) else True
        settings = _ABC_SETTINGS_SCHEMA({}) if isinstance(abc, bool) else abc
        cg.add(var.set_abc(enabled, settings[CONF_CYCLE], settings[CONF_BASELINE]))

    cg.add(var.set_bit0_inverted(config[CONF_BIT0_INVERTED]))


CALIBRATION_ACTION_SCHEMA = maybe_simple_id(
    {
        cv.GenerateID(): cv.use_id(CM1106ProComponent),
    },
)


@automation.register_action(
    "cm1106_pro.calibrate_zero",
    CalibrateZeroAction,
    maybe_simple_id(
        {
            cv.GenerateID(): cv.use_id(CM1106ProComponent),
            cv.Optional(CONF_PPM, default=400): cv.templatable(cv.int_range(min=400, max=1500)),
        }
    ),
)
async def cm1106_pro_calibrate_zero_to_code(
    config: ConfigType,
    action_id: ID,
    template_arg: cg.TemplateArguments,
    args: TemplateArgsType,
) -> MockObj:
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    template_ = await cg.templatable(config[CONF_PPM], args, cg.uint16)
    cg.add(var.set_ppm(template_))
    return var


for _name, _call in (
    ("cm1106_pro.abc_enable", "abc_enable()"),
    ("cm1106_pro.abc_disable", "abc_disable()"),
    ("cm1106_pro.read_version", "read_version()"),
):
    automation.register_apply_action(
        _name, CALIBRATION_ACTION_SCHEMA, automation.ApplyCall(_call)
    )
