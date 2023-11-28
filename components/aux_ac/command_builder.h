#pragma once

#include "aircon_common.h"
#include "helpers.h"
#include "esphome.h"
#include "esphome/components/climate/climate.h"

namespace esphome
{
    namespace aux_airconditioner
    {

        using esphome::climate::ClimateCall;
        using esphome::climate::ClimateFanMode;
        using esphome::climate::ClimateMode;
        using esphome::climate::ClimatePreset;
        using esphome::climate::ClimateSwingMode;

        class AirCon;
        class Frame;

        class CommandBuilder
        {
        private:
            static const uint8_t COMMAND = 0x01;
            static const uint8_t FLAG = 0x01;
            static const uint8_t COMMAND_SET_BODY_LENGTH = 0x0F;
            static const uint8_t COMMAND_REQUEST_BODY_LENGTH = 0x02;

            AirCon *_aircon{nullptr};
            Frame *_command_frame{nullptr};

        public:
            CommandBuilder() = delete;
            CommandBuilder(AirCon &aircon);
            ~CommandBuilder();

            CommandBuilder &init_new_command(command_type_t command_type = COMMAND_TYPE_SET_STATE);
            CommandBuilder &init_new_command(ClimateCall &cmd);
            CommandBuilder &fill_frame_with_command(Frame &frame);
            Frame get_builder_result();

            // ESPHome climate setters (high level)
            CommandBuilder &set_climate_mode(ClimateMode value);
            CommandBuilder &set_climate_fan_mode(ClimateFanMode value);
            CommandBuilder &set_climate_custom_fan_mode(std::string value);
            CommandBuilder &set_climate_preset(ClimatePreset value);
            CommandBuilder &set_climate_custom_preset(std::string value);
            CommandBuilder &set_climate_swing_mode(ClimateSwingMode value);

            // basic setters (low level)
            CommandBuilder &set_target_temperature(float value);
            CommandBuilder &set_vertical_louver(ac_louver_V value);
            CommandBuilder &set_horizontal_louver(ac_louver_H value);
            CommandBuilder &set_fan_speed(ac_fanspeed value);
            CommandBuilder &set_fan_turbo(bool value);
            CommandBuilder &set_fan_mute(bool value);
            CommandBuilder &set_mode(ac_mode value);
            CommandBuilder &set_fahrenheit_temperature(bool value);
            CommandBuilder &set_sleep_mode(bool value);
            CommandBuilder &set_power(bool value);
            CommandBuilder &set_iClean_mode(bool value);
            CommandBuilder &set_health_mode(bool value);
            CommandBuilder &set_antifungus_mode(bool value);
            CommandBuilder &set_display_state(bool value);
            CommandBuilder &set_inverter_power_limitation_state(bool enabled);
            CommandBuilder &set_inverter_power_limitation_value(uint8_t value);
        };

    } // namespace aux_airconditioner
} // namespace esphome
