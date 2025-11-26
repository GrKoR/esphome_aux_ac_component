import logging
from esphome.core import CORE, Define
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.components import uart, climate

from esphome.const import (
    CONF_DATA,
    CONF_ID,
    CONF_UART_ID,
)

AUX_AC_FIRMWARE_VERSION = '0.2.17'

_LOGGER = logging.getLogger(__name__)

CODEOWNERS = ["@GrKoR"]
DEPENDENCIES = ["uart"]

aux_ac_ns = cg.esphome_ns.namespace("aux_ac")

AuxUart = aux_ac_ns.class_("AuxUart", uart.UARTDevice)

def output_info(config):
    _LOGGER.info("AUX_AC firmware version: %s", AUX_AC_FIRMWARE_VERSION)
    return config

CONFIG_SCHEMA = cv.All(
    uart.UART_DEVICE_SCHEMA
    .extend(
        {
            cv.GenerateID(): cv.declare_id(AuxUart),
        }
    ),
    output_info,
)

async def to_code(config):
    CORE.add_define(
        Define("AUX_AC_FIRMWARE_VERSION", '"'+AUX_AC_FIRMWARE_VERSION+'"')
    )
    var = cg.new_Pvariable(config[CONF_ID])
    # await cg.register_component(var, config)

    parent = await cg.get_variable(config[CONF_UART_ID])
    cg.add(var.initAC(parent))

    if CONF_INDOOR_TEMPERATURE in config:
        conf = config[CONF_INDOOR_TEMPERATURE]
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_indoor_temperature_sensor(sens))

    if CONF_OUTDOOR_TEMPERATURE in config:
        conf = config[CONF_OUTDOOR_TEMPERATURE]
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_outdoor_temperature_sensor(sens))

    if CONF_OUTBOUND_TEMPERATURE in config:
        conf = config[CONF_OUTBOUND_TEMPERATURE]
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_outbound_temperature_sensor(sens))

    if CONF_INBOUND_TEMPERATURE in config:
        conf = config[CONF_INBOUND_TEMPERATURE]
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_inbound_temperature_sensor(sens))

    if CONF_COMPRESSOR_TEMPERATURE in config:
        conf = config[CONF_COMPRESSOR_TEMPERATURE]
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_compressor_temperature_sensor(sens))

    if CONF_VLOUVER_STATE in config:
        conf = config[CONF_VLOUVER_STATE]
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_vlouver_state_sensor(sens))

    if CONF_DISPLAY_STATE in config:
        conf = config[CONF_DISPLAY_STATE]
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(var.set_display_sensor(sens))

    if CONF_DEFROST_STATE in config:
        conf = config[CONF_DEFROST_STATE]
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(var.set_defrost_state(sens))

    if CONF_INVERTER_POWER in config:
        conf = config[CONF_INVERTER_POWER]
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_inverter_power_sensor(sens))

    if CONF_PRESET_REPORTER in config:
        conf = config[CONF_PRESET_REPORTER]
        sens = await text_sensor.new_text_sensor(conf)
        cg.add(var.set_preset_reporter_sensor(sens))

    if CONF_INVERTER_POWER_LIMIT_VALUE in config:
        conf = config[CONF_INVERTER_POWER_LIMIT_VALUE]
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_inverter_power_limit_value_sensor(sens))

    if CONF_INVERTER_POWER_LIMIT_STATE in config:
        conf = config[CONF_INVERTER_POWER_LIMIT_STATE]
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(var.set_inverter_power_limit_state_sensor(sens))

    cg.add(var.set_period(config[CONF_PERIOD].total_milliseconds))
    cg.add(var.set_show_action(config[CONF_SHOW_ACTION]))
    cg.add(var.set_display_inverted(config[CONF_DISPLAY_INVERTED]))
    cg.add(var.set_packet_timeout(config[CONF_TIMEOUT]))
    cg.add(var.set_optimistic(config[CONF_OPTIMISTIC]))
    if CONF_SUPPORTED_MODES in config:
        cg.add(var.set_supported_modes(config[CONF_SUPPORTED_MODES]))
    if CONF_SUPPORTED_SWING_MODES in config:
        cg.add(var.set_supported_swing_modes(config[CONF_SUPPORTED_SWING_MODES]))
    if CONF_SUPPORTED_PRESETS in config:
        cg.add(var.set_supported_presets(config[CONF_SUPPORTED_PRESETS]))
    if CONF_CUSTOM_PRESETS in config:
        cg.add(var.set_custom_presets(config[CONF_CUSTOM_PRESETS]))
    if CONF_CUSTOM_FAN_MODES in config:
        cg.add(var.set_custom_fan_modes(config[CONF_CUSTOM_FAN_MODES]))
