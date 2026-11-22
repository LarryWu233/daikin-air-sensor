"""Boot delay component: pause startup so slow power-on devices (e.g. SSD1306) are ready."""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID
from esphome.types import ConfigType

DOMAIN = "boot_delay"

boot_delay_ns = cg.esphome_ns.namespace("boot_delay")
BootDelay = boot_delay_ns.class_("BootDelay", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(BootDelay),
        cv.Optional("duration", default="5s"): cv.positive_time_period_milliseconds,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config: ConfigType) -> None:
    var = cg.new_Pvariable(config[CONF_ID])
    cg.add(var.set_duration(config["duration"].total_milliseconds))
    await cg.register_component(var, config)
