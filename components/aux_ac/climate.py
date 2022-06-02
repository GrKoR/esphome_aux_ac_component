import logging
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.components import climate, uart, sensor, binary_sensor, text_sensor
from esphome import automation
from esphome.automation import maybe_simple_id
from esphome.const import (
    CONF_ID,
    CONF_UART_ID,
    CONF_PERIOD,
    CONF_CUSTOM_FAN_MODES,
    CONF_CUSTOM_PRESETS,
    CONF_INTERNAL,
    CONF_DATA,
    CONF_SUPPORTED_MODES,
    CONF_SUPPORTED_SWING_MODES,
    CONF_SUPPORTED_PRESETS,
    #CONF_PRESSURE,
    UNIT_CELSIUS,
    UNIT_PERCENT,
    #UNIT_PASCAL,
    ICON_POWER,
    ICON_THERMOMETER,
    #ICON_GAS_CYLINDER,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_POWER_FACTOR,
    #DEVICE_CLASS_PRESSURE,
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
AUTO_LOAD = ["sensor", "binary_sensor", "text_sensor"]

CONF_SHOW_ACTION = 'show_action'
CONF_INDOOR_TEMPERATURE = 'indoor_temperature'
CONF_OUTDOOR_TEMPERATURE = 'outdoor_temperature'
ICON_OUTDOOR_TEMPERATURE = 'mdi:home-thermometer-outline'
CONF_INBOUND_TEMPERATURE = 'inbound_temperature'
ICON_INBOUND_TEMPERATURE = 'mdi:thermometer-plus'
CONF_OUTBOUND_TEMPERATURE = 'outbound_temperature'
ICON_OUTBOUND_TEMPERATURE = 'mdi:thermometer-minus'
CONF_COMPRESSOR_TEMPERATURE = 'compressor_temperature'
ICON_COMPRESSOR_TEMPERATURE = 'mdi:thermometer-lines'
CONF_DISPLAY_STATE = 'display_state'
CONF_INVERTOR_POWER = 'invertor_power'
CONF_DEFROST_STATE = 'defrost_state'
ICON_DEFROST = "mdi:snowflake-melt"
CONF_DISPLAY_INVERTED = 'display_inverted'
ICON_DISPLAY = "mdi:clock-digital"
CONF_PRESET_REPORTER = "preset_reporter"
ICON_PRESET_REPORTER = "mdi:format-list-group"


aux_ac_ns = cg.esphome_ns.namespace("aux_ac")
AirCon = aux_ac_ns.class_("AirCon", climate.Climate, cg.Component)
Capabilities = aux_ac_ns.namespace("Constants")

AirConDisplayOffAction = aux_ac_ns.class_("AirConDisplayOffAction", automation.Action)
AirConDisplayOnAction = aux_ac_ns.class_("AirConDisplayOnAction", automation.Action)
AirConVLouverSwingAction = aux_ac_ns.class_("AirConVLouverSwingAction", automation.Action)
AirConVLouverStopAction = aux_ac_ns.class_("AirConVLouverStopAction", automation.Action)
AirConVLouverTopAction = aux_ac_ns.class_("AirConVLouverTopAction", automation.Action)
AirConVLouverMiddleAboveAction = aux_ac_ns.class_("AirConVLouverMiddleAboveAction", automation.Action)
AirConVLouverMiddleAction = aux_ac_ns.class_("AirConVLouverMiddleAction", automation.Action)
AirConVLouverMiddleBelowAction = aux_ac_ns.class_("AirConVLouverMiddleBelowAction", automation.Action)
AirConVLouverBottomAction = aux_ac_ns.class_("AirConVLouverBottomAction", automation.Action)
AirConSendTestPacketAction = aux_ac_ns.class_("AirConSendTestPacketAction", automation.Action)

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
    "HEALTH": Capabilities.HEALTH,
    "ANTIFUNGUS": Capabilities.ANTIFUNGUS,
}
validate_custom_presets = cv.enum(CUSTOM_PRESETS, upper=True)


def validate_raw_data(value):
    if isinstance(value, list):
        return cv.Schema([cv.hex_uint8_t])(value)
    raise cv.Invalid(
        "data must be a list of bytes"
    )


def output_info(config):
    """_LOGGER.info(config)"""
    return config

CONFIG_SCHEMA = cv.All(
    climate.CLIMATE_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(AirCon),
            cv.Optional(CONF_PERIOD, default="7s"): cv.time_period,
            cv.Optional(CONF_SHOW_ACTION, default="true"): cv.boolean,
            cv.Optional(CONF_DISPLAY_INVERTED, default="false"): cv.boolean,
            cv.Optional(CONF_INVERTOR_POWER): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                icon=ICON_POWER,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_POWER_FACTOR,
                state_class=STATE_CLASS_MEASUREMENT,
            ).extend(
                {
                    cv.Optional(CONF_INTERNAL, default="true"): cv.boolean,
                }
            ),
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
            cv.Optional(CONF_OUTDOOR_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                icon=ICON_OUTDOOR_TEMPERATURE,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ).extend(
                {
                    cv.Optional(CONF_INTERNAL, default="true"): cv.boolean,
                }
            ),
            cv.Optional(CONF_INBOUND_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                icon=ICON_INBOUND_TEMPERATURE,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ).extend(
                {
                    cv.Optional(CONF_INTERNAL, default="true"): cv.boolean,
                }
            ),
            cv.Optional(CONF_OUTBOUND_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                icon=ICON_OUTBOUND_TEMPERATURE,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ).extend(
                {
                    cv.Optional(CONF_INTERNAL, default="true"): cv.boolean,
                }
            ),
            cv.Optional(CONF_COMPRESSOR_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                icon=ICON_COMPRESSOR_TEMPERATURE,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ).extend(
                {
                    cv.Optional(CONF_INTERNAL, default="true"): cv.boolean,
                }
            ),
            cv.Optional(CONF_DISPLAY_STATE): binary_sensor.binary_sensor_schema(
                icon=ICON_DISPLAY,
            ).extend(
                {
                    cv.Optional(CONF_INTERNAL, default="true"): cv.boolean,
                }
            ),
            cv.Optional(CONF_DEFROST_STATE): binary_sensor.binary_sensor_schema(
                icon=ICON_DEFROST,
            ).extend(
                {
                    cv.Optional(CONF_INTERNAL, default="true"): cv.boolean,
                }
            ),
            cv.Optional(CONF_PRESET_REPORTER): text_sensor.text_sensor_schema(
                icon=ICON_PRESET_REPORTER,
            ).extend(
                {
                    cv.Optional(CONF_INTERNAL, default="true"): cv.boolean,
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

    if CONF_DISPLAY_STATE in config:
        conf = config[CONF_DISPLAY_STATE]
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(var.set_display_sensor(sens))

    if CONF_DEFROST_STATE in config:
        conf = config[CONF_DEFROST_STATE]
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(var.set_defrost_state(sens))

    if CONF_INVERTOR_POWER in config:
        conf = config[CONF_INVERTOR_POWER]
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_invertor_power_sensor(sens))

    if CONF_PRESET_REPORTER in config:
        conf = config[CONF_PRESET_REPORTER]
        sens = await text_sensor.new_text_sensor(conf)
        cg.add(var.set_preset_reporter_sensor(sens))

    cg.add(var.set_period(config[CONF_PERIOD].total_milliseconds))
    cg.add(var.set_show_action(config[CONF_SHOW_ACTION]))
    cg.add(var.set_display_inverted(config[CONF_DISPLAY_INVERTED]))
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
async def display_off_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)

@automation.register_action("aux_ac.display_on", AirConDisplayOnAction, DISPLAY_ACTION_SCHEMA)
async def display_on_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)


VLOUVER_ACTION_SCHEMA = maybe_simple_id(
    {
        cv.Required(CONF_ID): cv.use_id(AirCon),
    }
)

@automation.register_action("aux_ac.vlouver_stop", AirConVLouverStopAction, VLOUVER_ACTION_SCHEMA)
async def vlouver_stop_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)

@automation.register_action("aux_ac.vlouver_swing", AirConVLouverSwingAction, VLOUVER_ACTION_SCHEMA)
async def vlouver_swing_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)


@automation.register_action("aux_ac.vlouver_top", AirConVLouverTopAction, VLOUVER_ACTION_SCHEMA)
async def vlouver_top_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)

@automation.register_action("aux_ac.vlouver_middle_above", AirConVLouverMiddleAboveAction, VLOUVER_ACTION_SCHEMA)
async def vlouver_middle_above_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)

@automation.register_action("aux_ac.vlouver_middle", AirConVLouverMiddleAction, VLOUVER_ACTION_SCHEMA)
async def vlouver_middle_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)

@automation.register_action("aux_ac.vlouver_middle_below", AirConVLouverMiddleBelowAction, VLOUVER_ACTION_SCHEMA)
async def vlouver_middle_below_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)

@automation.register_action("aux_ac.vlouver_bottom", AirConVLouverBottomAction, VLOUVER_ACTION_SCHEMA)
async def vlouver_bottom_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)


# *********************************************************************************************************
# ВАЖНО! Только для инженеров!
# Вызывайте метод aux_ac.send_packet только если понимаете, что делаете! Он не проверяет данные, а передаёт
# кондиционеру всё как есть. Какой эффект получится от передачи кондиционеру рандомных байт, никто не знает.
# Вы действуете на свой страх и риск.
# *********************************************************************************************************
SEND_TEST_PACKET_ACTION_SCHEMA = maybe_simple_id(
    {
        cv.Required(CONF_ID): cv.use_id(AirCon),
        cv.Required(CONF_DATA): cv.templatable(validate_raw_data),
    }
)

@automation.register_action(
    "aux_ac.send_packet",
    AirConSendTestPacketAction,
    SEND_TEST_PACKET_ACTION_SCHEMA
)
async def send_packet_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)

    data = config[CONF_DATA]
    if isinstance(data, bytes):
        data = list(data)

    if cg.is_template(data):
        templ = await cg.templatable(data, args, cg.std_vector.template(cg.uint8))
        cg.add(var.set_data_template(templ))
    else:
        cg.add(var.set_data_static(data))

    return var    