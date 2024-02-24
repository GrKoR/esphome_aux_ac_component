import logging
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.components import climate, uart, sensor, binary_sensor, text_sensor
from esphome import automation
from esphome.automation import maybe_simple_id
from esphome.const import (
    CONF_CUSTOM_FAN_MODES,
    CONF_CUSTOM_PRESETS,
    CONF_DATA,
    CONF_ID,
    CONF_INTERNAL,
    CONF_OPTIMISTIC,
    CONF_PERIOD,
    CONF_POSITION,
    CONF_SUPPORTED_MODES,
    CONF_SUPPORTED_SWING_MODES,
    CONF_SUPPORTED_PRESETS,
    CONF_TIMEOUT,
    CONF_UART_ID,
    UNIT_CELSIUS,
    UNIT_PERCENT,
    ICON_POWER,
    ICON_THERMOMETER,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_POWER_FACTOR,
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

CONF_SHOW_ACTION_DEPRICATED = "show_action"
CONF_INDOOR_TEMPERATURE_DEPRICATED = "indoor_temperature"
CONF_INBOUND_TEMPERATURE_DEPRICATED = "inbound_temperature"
CONF_OUTDOOR_TEMPERATURE_DEPRICATED = "outdoor_temperature"
CONF_OUTBOUND_TEMPERATURE_DEPRICATED = "outbound_temperature"
CONF_COMPRESSOR_TEMPERATURE_DEPRICATED = "compressor_temperature"

CONF_INDOOR_AMBIENT_TEMPERATURE = "indoor_ambient_temperature"

CONF_INDOOR_COIL_TEMPERATURE = "indoor_coil_temperature"
ICON_INDOOR_COIL_TEMPERATURE = "mdi:thermometer-plus"

CONF_OUTDOOR_AMBIENT_TEMPERATURE = "outdoor_ambient_temperature"
ICON_OUTDOOR_AMBIENT_TEMPERATURE = "mdi:home-thermometer-outline"

CONF_OUTDOOR_CONDENSER_TEMPERATURE = "outdoor_condenser_temperature"
ICON_OUTDOOR_CONDENSER_TEMPERATURE = "mdi:thermometer-minus"

CONF_DEFROST_TEMPERATURE = "defrost_temperature"
ICON_DEFROST_TEMPERATURE = "mdi:thermometer-lines"

CONF_COMPRESSOR_DISCHARGE_TEMPERATURE = "compressor_discharge_temperature"
ICON_COMPRESSOR_DISCHARGE_TEMPERATURE = "mdi:thermometer-lines"

CONF_COMPRESSOR_SUCTION_TEMPERATURE = "compressor_suction_temperature"
ICON_COMPRESSOR_SUCTION_TEMPERATURE = "mdi:thermometer-lines"

CONF_DISPLAY_STATE = "display_state"
ICON_DISPLAY_STATE = "mdi:clock-digital"

CONF_INVERTER_POWER = "inverter_power"
CONF_INVERTER_POWER_DEPRICATED = "invertor_power"

CONF_DEFROST_STATE = "defrost_state"
ICON_DEFROST_STATE = "mdi:snowflake-melt"

CONF_DISPLAY_INVERTED = "display_inverted"

CONF_PRESET_REPORTER = "preset_reporter"
ICON_PRESET_REPORTER = "mdi:format-list-group"

CONF_VLOUVER_STATE = "vlouver_state"
ICON_VLOUVER_STATE = "mdi:compare-vertical"

CONF_LIMIT = "limit"
CONF_INVERTER_POWER_LIMIT_VALUE = "inverter_power_limit_value"
ICON_INVERTER_POWER_LIMIT_VALUE = "mdi:meter-electric-outline"
CONF_INVERTER_POWER_LIMIT_STATE = "inverter_power_limit_state"
ICON_INVERTER_POWER_LIMIT_STATE = "mdi:meter-electric-outline"


aux_ac_ns = cg.esphome_ns.namespace("aux_airconditioner")
AirCon = aux_ac_ns.class_("AirCon", climate.Climate, cg.Component)
Capabilities = aux_ac_ns.namespace("Capabilities")

# Display actions
AirConDisplayOffAction = aux_ac_ns.class_(
    "AirConDisplayOffAction", automation.Action)
AirConDisplayOnAction = aux_ac_ns.class_(
    "AirConDisplayOnAction", automation.Action)

# vertical louvers actions
AirConVLouverSwingAction = aux_ac_ns.class_(
    "AirConVLouverSwingAction", automation.Action)
AirConVLouverStopAction = aux_ac_ns.class_(
    "AirConVLouverStopAction", automation.Action)
AirConVLouverTopAction = aux_ac_ns.class_(
    "AirConVLouverTopAction", automation.Action)
AirConVLouverMiddleAboveAction = aux_ac_ns.class_(
    "AirConVLouverMiddleAboveAction", automation.Action)
AirConVLouverMiddleAction = aux_ac_ns.class_(
    "AirConVLouverMiddleAction", automation.Action)
AirConVLouverMiddleBelowAction = aux_ac_ns.class_(
    "AirConVLouverMiddleBelowAction", automation.Action)
AirConVLouverBottomAction = aux_ac_ns.class_(
    "AirConVLouverBottomAction", automation.Action)
AirConVLouverSetAction = aux_ac_ns.class_(
    "AirConVLouverSetAction", automation.Action)

# power limitation actions
AirConPowerLimitationOffAction = aux_ac_ns.class_(
    "AirConPowerLimitationOffAction", automation.Action)
AirConPowerLimitationOnAction = aux_ac_ns.class_(
    "AirConPowerLimitationOnAction", automation.Action)


AC_PACKET_TIMEOUT_MIN = 300
AC_PACKET_TIMEOUT_MAX = 800


def validate_packet_timeout(value):
    minV = AC_PACKET_TIMEOUT_MIN
    maxV = AC_PACKET_TIMEOUT_MAX
    if value in range(minV, maxV+1):
        return cv.Schema(cv.uint32_t)(value)
    raise cv.Invalid(f"Timeout should be in range: {minV}..{maxV}.")


AC_POWER_LIMIT_MIN = 30
AC_POWER_LIMIT_MAX = 100


def validate_power_limit_range(value):
    minV = AC_POWER_LIMIT_MIN
    maxV = AC_POWER_LIMIT_MAX
    if value in range(minV, maxV+1):
        return cv.Schema(cv.uint32_t)(value)
    raise cv.Invalid(f"Power limit should be in range: {minV}..{maxV}")


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
    "MUTE": Capabilities.CUSTOM_FAN_MODE_MUTE,
    "TURBO": Capabilities.CUSTOM_FAN_MODE_TURBO,
}
validate_custom_fan_modes = cv.enum(CUSTOM_FAN_MODES, upper=True)

CUSTOM_PRESETS = {
    "CLEAN": Capabilities.CUSTOM_PRESET_CLEAN,
    "HEALTH": Capabilities.CUSTOM_PRESET_HEALTH,
    "ANTIFUNGUS": Capabilities.CUSTOM_PRESET_ANTIFUNGUS,
}
validate_custom_presets = cv.enum(CUSTOM_PRESETS, upper=True)


def output_info(config):
    # _LOGGER.info(config.items())
    return config


CONFIG_SCHEMA = cv.All(
    climate.CLIMATE_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(AirCon),
            cv.Optional(CONF_PERIOD, default="7s"): cv.time_period,
            # cv.Optional(CONF_SHOW_ACTION, default="true"): cv.boolean,
            cv.Optional(CONF_SHOW_ACTION_DEPRICATED): cv.invalid(
                f"Parameter '{CONF_SHOW_ACTION_DEPRICATED}' was deleted in v.1.0.0. Update your config please."
            ),
            cv.Optional(CONF_DISPLAY_INVERTED, default="false"): cv.boolean,
            cv.Optional(CONF_TIMEOUT, default=AC_PACKET_TIMEOUT_MIN): validate_packet_timeout,
            cv.Optional(CONF_OPTIMISTIC, default="true"): cv.boolean,
            cv.Optional(CONF_INVERTER_POWER_DEPRICATED): cv.invalid(
                f"The name of sensor was changed in v.0.2.9 from '{CONF_INVERTER_POWER_DEPRICATED}' to '{CONF_INVERTER_POWER}'. Update your config please."
            ),
            cv.Optional(CONF_INVERTER_POWER): sensor.sensor_schema(
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

            cv.Optional(CONF_INDOOR_TEMPERATURE_DEPRICATED): cv.invalid(
                f"Parameter '{CONF_INDOOR_TEMPERATURE_DEPRICATED}' was deleted in v.1.0.0, use '{CONF_INDOOR_AMBIENT_TEMPERATURE}' instead."
            ),
            cv.Optional(CONF_INDOOR_AMBIENT_TEMPERATURE): sensor.sensor_schema(
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

            cv.Optional(CONF_INBOUND_TEMPERATURE_DEPRICATED): cv.invalid(
                f"Parameter '{CONF_INBOUND_TEMPERATURE_DEPRICATED}' was deleted in v.1.0.0, use '{CONF_INDOOR_COIL_TEMPERATURE}' instead."
            ),
            cv.Optional(CONF_INDOOR_COIL_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                icon=ICON_INDOOR_COIL_TEMPERATURE,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ).extend(
                {
                    cv.Optional(CONF_INTERNAL, default="true"): cv.boolean,
                }
            ),

            cv.Optional(CONF_OUTDOOR_TEMPERATURE_DEPRICATED): cv.invalid(
                f"Parameter '{CONF_OUTDOOR_TEMPERATURE_DEPRICATED}' was deleted in v.1.0.0, use '{CONF_OUTDOOR_AMBIENT_TEMPERATURE}' instead."
            ),
            cv.Optional(CONF_OUTDOOR_AMBIENT_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                icon=ICON_OUTDOOR_AMBIENT_TEMPERATURE,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ).extend(
                {
                    cv.Optional(CONF_INTERNAL, default="true"): cv.boolean,
                }
            ),

            cv.Optional(CONF_OUTDOOR_CONDENSER_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                icon=ICON_OUTDOOR_CONDENSER_TEMPERATURE,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ).extend(
                {
                    cv.Optional(CONF_INTERNAL, default="true"): cv.boolean,
                }
            ),

            cv.Optional(CONF_COMPRESSOR_TEMPERATURE_DEPRICATED): cv.invalid(
                f"Parameter '{CONF_COMPRESSOR_TEMPERATURE_DEPRICATED}' was deleted in v.1.0.0, use '{CONF_COMPRESSOR_DISCHARGE_TEMPERATURE}' instead."
            ),
            cv.Optional(CONF_COMPRESSOR_DISCHARGE_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                icon=ICON_COMPRESSOR_DISCHARGE_TEMPERATURE,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ).extend(
                {
                    cv.Optional(CONF_INTERNAL, default="true"): cv.boolean,
                }
            ),

            cv.Optional(CONF_OUTBOUND_TEMPERATURE_DEPRICATED): cv.invalid(
                f"Parameter '{CONF_OUTBOUND_TEMPERATURE_DEPRICATED}' was deleted in v.1.0.0, use '{CONF_COMPRESSOR_SUCTION_TEMPERATURE}' instead."
            ),
            cv.Optional(CONF_COMPRESSOR_SUCTION_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                icon=ICON_COMPRESSOR_SUCTION_TEMPERATURE,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ).extend(
                {
                    cv.Optional(CONF_INTERNAL, default="true"): cv.boolean,
                }
            ),

            cv.Optional(CONF_DEFROST_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                icon=ICON_DEFROST_TEMPERATURE,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ).extend(
                {
                    cv.Optional(CONF_INTERNAL, default="true"): cv.boolean,
                }
            ),

            cv.Optional(CONF_VLOUVER_STATE): sensor.sensor_schema(
                icon=ICON_VLOUVER_STATE,
                accuracy_decimals=0,
            ).extend(
                {
                    cv.Optional(CONF_INTERNAL, default="true"): cv.boolean,
                }
            ),
            cv.Optional(CONF_DISPLAY_STATE): binary_sensor.binary_sensor_schema(
                icon=ICON_DISPLAY_STATE,
            ).extend(
                {
                    cv.Optional(CONF_INTERNAL, default="true"): cv.boolean,
                }
            ),
            cv.Optional(CONF_DEFROST_STATE): binary_sensor.binary_sensor_schema(
                icon=ICON_DEFROST_STATE,
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
            cv.Optional(CONF_INVERTER_POWER_LIMIT_VALUE): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                icon=ICON_INVERTER_POWER_LIMIT_VALUE,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_POWER_FACTOR,
                state_class=STATE_CLASS_MEASUREMENT,
            ).extend(
                {
                    cv.Optional(CONF_INTERNAL, default="true"): cv.boolean,
                }
            ),
            cv.Optional(CONF_INVERTER_POWER_LIMIT_STATE): binary_sensor.binary_sensor_schema(
                icon=ICON_INVERTER_POWER_LIMIT_STATE,
            ).extend(
                {
                    cv.Optional(CONF_INTERNAL, default="true"): cv.boolean,
                }
            ),
            cv.Optional(CONF_SUPPORTED_MODES): cv.ensure_list(validate_modes),
            cv.Optional(CONF_SUPPORTED_SWING_MODES): cv.ensure_list(
                validate_swing_modes
            ),
            cv.Optional(CONF_SUPPORTED_PRESETS): cv.ensure_list(validate_presets),
            cv.Optional(CONF_CUSTOM_PRESETS): cv.ensure_list(validate_custom_presets),
            cv.Optional(CONF_CUSTOM_FAN_MODES): cv.ensure_list(
                validate_custom_fan_modes
            ),
        }
    )
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA),
    output_info,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)

    parent = await cg.get_variable(config[CONF_UART_ID])
    cg.add(var.set_uart(parent))

    if CONF_INDOOR_AMBIENT_TEMPERATURE in config:
        conf = config[CONF_INDOOR_AMBIENT_TEMPERATURE]
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_sensor_temperature_indoor_ambient(sens))

    if CONF_INDOOR_COIL_TEMPERATURE in config:
        conf = config[CONF_INDOOR_COIL_TEMPERATURE]
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_sensor_temperature_indoor_coil(sens))

    if CONF_OUTDOOR_AMBIENT_TEMPERATURE in config:
        conf = config[CONF_OUTDOOR_AMBIENT_TEMPERATURE]
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_sensor_temperature_outdoor_ambient(sens))

    if CONF_OUTDOOR_CONDENSER_TEMPERATURE in config:
        conf = config[CONF_OUTDOOR_CONDENSER_TEMPERATURE]
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_sensor_temperature_outdoor_condenser_middle(sens))

    if CONF_DEFROST_TEMPERATURE in config:
        conf = config[CONF_DEFROST_TEMPERATURE]
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_sensor_temperature_outdoor_defrost(sens))

    if CONF_COMPRESSOR_DISCHARGE_TEMPERATURE in config:
        conf = config[CONF_COMPRESSOR_DISCHARGE_TEMPERATURE]
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_sensor_temperature_outdoor_discharge(sens))

    if CONF_COMPRESSOR_SUCTION_TEMPERATURE in config:
        conf = config[CONF_COMPRESSOR_SUCTION_TEMPERATURE]
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_sensor_temperature_outdoor_suction(sens))

    if CONF_VLOUVER_STATE in config:
        conf = config[CONF_VLOUVER_STATE]
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_sensor_vlouver_state(sens))

    if CONF_DISPLAY_STATE in config:
        conf = config[CONF_DISPLAY_STATE]
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(var.set_sensor_display(sens))

    if CONF_DEFROST_STATE in config:
        conf = config[CONF_DEFROST_STATE]
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(var.set_sensor_defrost_state(sens))

    if CONF_PRESET_REPORTER in config:
        conf = config[CONF_PRESET_REPORTER]
        sens = await text_sensor.new_text_sensor(conf)
        cg.add(var.set_sensor_preset_reporter(sens))

    if CONF_INVERTER_POWER in config:
        conf = config[CONF_INVERTER_POWER]
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_sensor_inverter_power(sens))

    if CONF_INVERTER_POWER_LIMIT_VALUE in config:
        conf = config[CONF_INVERTER_POWER_LIMIT_VALUE]
        sens = await sensor.new_sensor(conf)
        cg.add(var.set_sensor_inverter_power_limit_value(sens))

    if CONF_INVERTER_POWER_LIMIT_STATE in config:
        conf = config[CONF_INVERTER_POWER_LIMIT_STATE]
        sens = await binary_sensor.new_binary_sensor(conf)
        cg.add(var.set_sensor_inverter_power_limit_state(sens))

    cg.add(var.set_period(config[CONF_PERIOD].total_milliseconds))
    # cg.add(var.set_show_action(config[CONF_SHOW_ACTION]))
    cg.add(var.set_display_inversion(config[CONF_DISPLAY_INVERTED]))
    cg.add(var.set_packet_timeout(config[CONF_TIMEOUT]))
    cg.add(var.set_optimistic(config[CONF_OPTIMISTIC]))
    if CONF_SUPPORTED_MODES in config:
        cg.add(var.set_supported_modes(config[CONF_SUPPORTED_MODES]))
    if CONF_SUPPORTED_SWING_MODES in config:
        cg.add(var.set_supported_swing_modes(
            config[CONF_SUPPORTED_SWING_MODES]))
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


@automation.register_action(
    "aux_ac.display_off", AirConDisplayOffAction, DISPLAY_ACTION_SCHEMA
)
async def display_off_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, parent)


@automation.register_action(
    "aux_ac.display_on", AirConDisplayOnAction, DISPLAY_ACTION_SCHEMA
)
async def display_on_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, parent)


VLOUVER_ACTION_SCHEMA = maybe_simple_id(
    {
        cv.Required(CONF_ID): cv.use_id(AirCon),
    }
)


@automation.register_action(
    "aux_ac.vlouver_stop", AirConVLouverStopAction, VLOUVER_ACTION_SCHEMA
)
async def vlouver_stop_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, parent)


@automation.register_action(
    "aux_ac.vlouver_swing", AirConVLouverSwingAction, VLOUVER_ACTION_SCHEMA
)
async def vlouver_swing_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, parent)


@automation.register_action(
    "aux_ac.vlouver_top", AirConVLouverTopAction, VLOUVER_ACTION_SCHEMA
)
async def vlouver_top_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, parent)


@automation.register_action(
    "aux_ac.vlouver_middle_above", AirConVLouverMiddleAboveAction, VLOUVER_ACTION_SCHEMA
)
async def vlouver_middle_above_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, parent)


@automation.register_action(
    "aux_ac.vlouver_middle", AirConVLouverMiddleAction, VLOUVER_ACTION_SCHEMA
)
async def vlouver_middle_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, parent)


@automation.register_action(
    "aux_ac.vlouver_middle_below", AirConVLouverMiddleBelowAction, VLOUVER_ACTION_SCHEMA
)
async def vlouver_middle_below_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, parent)


@automation.register_action(
    "aux_ac.vlouver_bottom", AirConVLouverBottomAction, VLOUVER_ACTION_SCHEMA
)
async def vlouver_bottom_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, parent)


VLOUVER_SET_ACTION_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_ID): cv.use_id(AirCon),
        cv.Required(CONF_POSITION): cv.templatable(cv.int_range(0, 6)),
    }
)


@automation.register_action(
    "aux_ac.vlouver_set", AirConVLouverSetAction, VLOUVER_SET_ACTION_SCHEMA
)
async def vlouver_set_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    template_ = await cg.templatable(config[CONF_POSITION], args, int)
    cg.add(var.set_value(template_))
    return var


POWER_LIMITATION_OFF_ACTION_SCHEMA = maybe_simple_id(
    {
        cv.Required(CONF_ID): cv.use_id(AirCon),
    }
)


@automation.register_action(
    "aux_ac.power_limit_off", AirConPowerLimitationOffAction, POWER_LIMITATION_OFF_ACTION_SCHEMA
)
async def power_limit_off_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, parent)


POWER_LIMITATION_ON_ACTION_SCHEMA = maybe_simple_id(
    {
        cv.Required(CONF_ID): cv.use_id(AirCon),
        cv.Optional(CONF_LIMIT, default=AC_POWER_LIMIT_MIN): validate_power_limit_range,
    }
)


@automation.register_action(
    "aux_ac.power_limit_on", AirConPowerLimitationOnAction, POWER_LIMITATION_ON_ACTION_SCHEMA
)
async def power_limit_on_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    template_ = await cg.templatable(config[CONF_LIMIT], args, int)
    cg.add(var.set_value(template_))
    return var
