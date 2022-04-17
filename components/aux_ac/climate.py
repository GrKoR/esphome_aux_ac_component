import logging
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.components import climate, uart, sensor, binary_sensor
from esphome import automation
from esphome.automation import maybe_simple_id
from esphome.const import (
    CONF_ID,
    CONF_UART_ID,
    CONF_PERIOD,
    CONF_CUSTOM_FAN_MODES,
    CONF_CUSTOM_PRESETS,
    CONF_INTERNAL,
    UNIT_CELSIUS,
    ICON_THERMOMETER,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
)
from esphome.components.climate import (
    ClimateMode,
    ClimatePreset,
    ClimateSwingMode,
)

_LOGGER = logging.getLogger(__name__)

CODEOWNERS = ["@GrKoR"]
DEPENDENCIES = ["climate", "uart"]
AUTO_LOAD = ["sensor", "binary_sensor"]

CONF_SUPPORTED_MODES = 'supported_modes'
CONF_SUPPORTED_SWING_MODES = 'supported_swing_modes'
CONF_SUPPORTED_PRESETS = 'supported_presets'
CONF_SHOW_ACTION = 'show_action'
CONF_INDOOR_TEMPERATURE = 'indoor_temperature'
CONF_DISPLAY_STATE = 'display_state'

ICON_DISPLAY = "mdi:numeric"

aux_ac_ns = cg.esphome_ns.namespace("aux_ac")
AirCon = aux_ac_ns.class_("AirCon", climate.Climate, cg.Component)
Capabilities = aux_ac_ns.namespace("Constants")

AirConDisplayOffAction = aux_ac_ns.class_("AirConDisplayOffAction", automation.Action)
AirConDisplayOnAction = aux_ac_ns.class_("AirConDisplayOnAction", automation.Action)

ALLOWED_CLIMATE_MODES = {
    "HEAT_COOL": ClimateMode.CLIMATE_MODE_HEAT_COOL,
    "COOL": ClimateMode.CLIMATE_MODE_COOL,
    "HEAT": ClimateMode.CLIMATE_MODE_HEAT,
    "DRY": ClimateMode.CLIMATE_MODE_DRY,
    "FAN_ONLY": ClimateMode.CLIMATE_MODE_FAN_ONLY,
}
validate_modes = cv.enum(ALLOWED_CLIMATE_MODES, upper=True)

ALLOWED_CLIMATE_PRESETS = {
    "SLEEP": ClimatePreset.CLIMATE_PRESET_SLEEP,
}
validate_presets = cv.enum(ALLOWED_CLIMATE_PRESETS, upper=True)

ALLOWED_CLIMATE_SWING_MODES = {
    "BOTH": ClimateSwingMode.CLIMATE_SWING_BOTH,
    "VERTICAL": ClimateSwingMode.CLIMATE_SWING_VERTICAL,
    "HORIZONTAL": ClimateSwingMode.CLIMATE_SWING_HORIZONTAL,
}
validate_swing_modes = cv.enum(ALLOWED_CLIMATE_SWING_MODES, upper=True)

CUSTOM_FAN_MODES = {
    "MUTE": Capabilities.MUTE,
    "TURBO": Capabilities.TURBO,
}
validate_custom_fan_modes = cv.enum(CUSTOM_FAN_MODES, upper=True)

CUSTOM_PRESETS = {
    "CLEAN": Capabilities.CLEAN,
    "FEEL": Capabilities.FEEL,
    "HEALTH": Capabilities.HEALTH,
    "ANTIFUNGUS": Capabilities.ANTIFUNGUS,
}
validate_custom_presets = cv.enum(CUSTOM_PRESETS, upper=True)

def output_info(config):
    """_LOGGER.info(config)"""
    return config

CONFIG_SCHEMA = cv.All(
    climate.CLIMATE_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(AirCon),
            cv.Optional(CONF_PERIOD, default="7s"): cv.time_period,
            cv.Optional(CONF_SHOW_ACTION, default="true"): cv.boolean,
            cv.Optional(CONF_INDOOR_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                icon=ICON_THERMOMETER,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ).extend(
                {
                    cv.Optional(CONF_INTERNAL, default="true"): cv.boolean,
                }
            ),
            cv.Optional(CONF_DISPLAY_STATE): binary_sensor.binary_sensor_schema(
                icon=ICON_DISPLAY
            ).extend(
                {
                    cv.Optional(CONF_INTERNAL, default="true"): cv.boolean
                }
            ),
            cv.Optional(CONF_SUPPORTED_MODES): cv.ensure_list(validate_modes),
            cv.Optional(CONF_SUPPORTED_SWING_MODES): cv.ensure_list(validate_swing_modes),
            cv.Optional(CONF_SUPPORTED_PRESETS): cv.ensure_list(validate_presets),
            cv.Optional(CONF_CUSTOM_PRESETS): cv.ensure_list(validate_custom_presets),
            cv.Optional(CONF_CUSTOM_FAN_MODES): cv.ensure_list(validate_custom_fan_modes),
        }
    )
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA),
    output_info
)

async def to_code(config):
    """_LOGGER.info("--------------")"""
    """_LOGGER.info(config)"""
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)
    
    parent = await cg.get_variable(config[CONF_UART_ID])
    cg.add(var.initAC(parent))

    if CONF_INDOOR_TEMPERATURE in config:
        conf = config[CONF_INDOOR_TEMPERATURE]
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_indoor_temperature_sensor(sens))

    if CONF_DISPLAY_STATE in config:
        conf = config[CONF_DISPLAY_STATE]
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(var.set_display_sensor(sens))

    cg.add(var.set_period(config[CONF_PERIOD].total_milliseconds))
    cg.add(var.set_show_action(config[CONF_SHOW_ACTION]))
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



DISPLAY_ACTION_SCHEMA = maybe_simple_id(
    {
        cv.Required(CONF_ID): cv.use_id(AirCon),
    }
)

@automation.register_action("aux_ac.display_off", AirConDisplayOffAction, DISPLAY_ACTION_SCHEMA)
async def switch_toggle_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)

@automation.register_action("aux_ac.display_on", AirConDisplayOnAction, DISPLAY_ACTION_SCHEMA)
async def switch_toggle_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)