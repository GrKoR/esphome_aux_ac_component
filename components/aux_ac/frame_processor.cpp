#include "frame_processor.h"

#include "frame.h"
#include "helpers.h"
#include <algorithm>

namespace esphome
{
    namespace aux_airconditioner
    {

        using esphome::helpers::update_property;

        /*********************************************************************************************\
        \*********************************************************************************************/
        void FrameProcessorInterface::process(const Frame &frame, AirCon &aircon) const
        {
            if (!this->applicable(frame))
                return;

            if (!aircon.is_hardware_connected())
                return;

            aircon.reset_ping_timeout();
            this->_specific_process(frame, aircon);
        }

        /*********************************************************************************************\
        \*********************************************************************************************/
        bool FrameProcessorPing::applicable(const Frame &frame) const
        {
            return frame.has_type(FrameType::FRAME_TYPE_PING);
        }

        FrameType FrameProcessorPing::get_applicable_frame_type() const
        {
            return FrameType::FRAME_TYPE_PING;
        }

        void FrameProcessorPing::_specific_process(const Frame &frame, AirCon &aircon) const
        {
            aircon.schedule_ping_response();
        }

        /*********************************************************************************************\
        \*********************************************************************************************/
        bool FrameProcessorResponse01::applicable(const Frame &frame) const
        {
            return frame.has_type(FrameType::FRAME_TYPE_RESPONSE) &&
                   frame.get_body_length() == 0x04 &&
                   frame.get_value(9) == 0x01;
        }

        FrameType FrameProcessorResponse01::get_applicable_frame_type() const
        {
            return FrameType::FRAME_TYPE_RESPONSE;
        }

        void FrameProcessorResponse01::_specific_process(const Frame &frame, AirCon &aircon) const
        {
        }

        /*********************************************************************************************\
        \*********************************************************************************************/
        ClimateMode FrameProcessorResponse11::_power_and_mode_to_climate_mode(bool power_on, ac_mode mode) const
        {
            ClimateMode result = ClimateMode::CLIMATE_MODE_OFF;
            if (power_on)
            {
                switch (mode)
                {
                case AC_MODE_AUTO:
                    result = ClimateMode::CLIMATE_MODE_HEAT_COOL;
                    break;

                case AC_MODE_COOL:
                    result = ClimateMode::CLIMATE_MODE_COOL;
                    break;

                case AC_MODE_DRY:
                    result = ClimateMode::CLIMATE_MODE_DRY;
                    break;

                case AC_MODE_HEAT:
                    result = ClimateMode::CLIMATE_MODE_HEAT;
                    break;

                case AC_MODE_FAN:
                    result = ClimateMode::CLIMATE_MODE_FAN_ONLY;
                    break;

                default:
                    ESP_LOGW(TAG, "Warning: unknown air conditioner mode: 0x%02X", mode);
                    break;
                }
            }

            return result;
        }

        bool FrameProcessorResponse11::applicable(const Frame &frame) const
        {
            return frame.has_type(FrameType::FRAME_TYPE_RESPONSE) &&
                   frame.get_body_length() == 0x0F &&
                   frame.get_value(9) == 0x11;
        }

        FrameType FrameProcessorResponse11::get_applicable_frame_type() const
        {
            return FrameType::FRAME_TYPE_RESPONSE;
        }

        void FrameProcessorResponse11::_specific_process(const Frame &frame, AirCon &aircon) const
        {
            aircon.set_last_frame(frame);

            bool state_changed = false;

            // target temperature:
            // byte 10: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_11_b10
            // byte 12: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_11_b12
            update_property(aircon.target_temperature, (float)(8.0 + (float)frame.get_value(10, 0b11111000, 3) + (frame.get_bit(12, 7) ? 0.5 : 0.0)), state_changed);

            // vertical louver state:
            // byte 10: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_11_b10
            // horizontal louver state:
            // byte 11: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_11_b11
            update_property(aircon.louver_vertical, (ac_louver_V)frame.get_value(10, 0b00000111), state_changed);
            update_property(aircon.louver_horizontal, (ac_louver_H)frame.get_value(11, 0b11100000), state_changed);
            if (aircon.louver_vertical == AC_LOUVERV_SWING_UPDOWN && aircon.louver_horizontal != AC_LOUVERH_SWING_LEFTRIGHT)
                update_property(aircon.swing_mode, ClimateSwingMode::CLIMATE_SWING_VERTICAL, state_changed);
            else if (aircon.louver_vertical != AC_LOUVERV_SWING_UPDOWN && aircon.louver_horizontal == AC_LOUVERH_SWING_LEFTRIGHT)
                update_property(aircon.swing_mode, ClimateSwingMode::CLIMATE_SWING_HORIZONTAL, state_changed);
            else if (aircon.louver_vertical == AC_LOUVERV_SWING_UPDOWN && aircon.louver_horizontal == AC_LOUVERH_SWING_LEFTRIGHT)
                update_property(aircon.swing_mode, ClimateSwingMode::CLIMATE_SWING_BOTH, state_changed);
            else if (aircon.louver_vertical != AC_LOUVERV_SWING_UPDOWN && aircon.louver_horizontal != AC_LOUVERH_SWING_LEFTRIGHT)
                update_property(aircon.swing_mode, ClimateSwingMode::CLIMATE_SWING_OFF, state_changed);

            // last IR-command was this time ago (minutes)
            // byte 12: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_11_b12
            update_property(aircon.last_IR_passed, frame.get_value(12, 0b00111111), state_changed);

            // fan speed:
            // byte 13: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_11_b13
            update_property(aircon.fan_mode, ac_fanspeed_to_climate_fan_mode((ac_fanspeed)frame.get_value(13, 0b11100000)), state_changed);

            // timer activation & delay:
            // byte 13: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_11_b13
            // byte 14: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_11_b14
            // byte 18: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_11_b18
            // timer.delay_minutes_uint16 = frame.get_value(13, 0b00011111) * 60 + frame.get_value(14, 0b00011111);
            // timer.enabled_bool = frame.get_bit(18, 6);
            // update_property(aircon.???, ???, state_changed);

            // fan TURBO mode:
            // byte 14: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_11_b14
            if (frame.get_bit(14, 6))
            {
                update_property(aircon.custom_fan_mode, Capabilities::CUSTOM_FAN_MODE_TURBO, state_changed);
            }
            else if (aircon.custom_fan_mode == Capabilities::CUSTOM_FAN_MODE_TURBO)
            {
                update_property(aircon.custom_fan_mode, (std::string) "", state_changed);
            }

            // fan MUTE mode:
            // byte 14: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_11_b14
            if (frame.get_bit(14, 7))
            {
                update_property(aircon.custom_fan_mode, Capabilities::CUSTOM_FAN_MODE_MUTE, state_changed);
            }
            else if (aircon.custom_fan_mode == Capabilities::CUSTOM_FAN_MODE_MUTE)
            {
                update_property(aircon.custom_fan_mode, (std::string) "", state_changed);
            }

            // power & mode:
            // byte 15: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_11_b15
            // byte 18: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_11_b18
            update_property(aircon.mode, _power_and_mode_to_climate_mode(frame.get_bit(18, 5), (ac_mode)frame.get_value(15, 0b11100000)), state_changed);

            // temperature: Celsius or Fahrenheit
            // byte 15: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_11_b15
            if (update_property(aircon.temperature_in_fahrenheit, frame.get_bit(15, 1), state_changed))
                aircon.update_all_sensors_unit_of_measurement();

            // SLEEP preset:
            // byte 15: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_11_b15
            if (frame.get_bit(15, 2))
            {
                update_property(aircon.preset, ClimatePreset::CLIMATE_PRESET_SLEEP, state_changed);
            }
            else if (aircon.preset == ClimatePreset::CLIMATE_PRESET_SLEEP)
            {
                update_property(aircon.preset, ClimatePreset::CLIMATE_PRESET_NONE, state_changed);
            }

            // iFeel function: disabled due to uselessness (and doesn't work with wi-fi)
            // byte 15: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_11_b15
            // update_property(aircon.iFeel, frame.get_bit(15, 3), state_changed);

            // iClean function:
            // byte 18: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_11_b18
            if (frame.get_bit(18, 2) && !frame.get_bit(18, 5)) // iClean + Power_Off
            {
                update_property(aircon.custom_preset, Capabilities::CUSTOM_PRESET_CLEAN, state_changed);
            }
            else if (aircon.custom_preset == Capabilities::CUSTOM_PRESET_CLEAN)
            {
                update_property(aircon.custom_preset, (std::string) "", state_changed);
            }

            // Health function:
            // byte 18: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_11_b18
            if (frame.get_bit(18, 1) && frame.get_bit(18, 5)) // Health + Power_On
            {
                update_property(aircon.custom_preset, Capabilities::CUSTOM_PRESET_HEALTH, state_changed);
            }
            else if (aircon.custom_preset == Capabilities::CUSTOM_PRESET_HEALTH)
            {
                update_property(aircon.custom_preset, (std::string) "", state_changed);
            }

            // Antifungus function:
            // byte 20: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_11_b20
            if (frame.get_bit(20, 3))
            {
                update_property(aircon.custom_preset, Capabilities::CUSTOM_PRESET_ANTIFUNGUS, state_changed);
            }
            else if (aircon.custom_preset == Capabilities::CUSTOM_PRESET_ANTIFUNGUS)
            {
                update_property(aircon.custom_preset, (std::string) "", state_changed);
            }

            // Display:
            // byte 20: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_11_b20
            update_property(aircon.display_enabled, (bool)(frame.get_bit(20, 4) ^ aircon.get_display_inversion()), state_changed);

            // Power limitation for inverters:
            // byte 21: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_11_b21
            if (aircon.ac_type_inverter)
            {
                update_property(aircon.inverter_power_limitation_value, frame.get_value(21, 0b01111111), state_changed);
                update_property(aircon.inverter_power_limitation_on, frame.get_bit(21, 7), state_changed);
            }
            else
            {
                aircon.inverter_power_limitation_value.reset();
                aircon.inverter_power_limitation_on.reset();
            }

            if (state_changed)
            {
                aircon.publish_all_states();
            }
        }

        /*********************************************************************************************\
        \*********************************************************************************************/
        bool FrameProcessorResponse2x::applicable(const Frame &frame) const
        {
            return frame.has_type(FrameType::FRAME_TYPE_RESPONSE) &&
                   frame.get_body_length() == 0x18 &&
                   frame.get_value(9, 0b11110000) == 0x20;
        }

        FrameType FrameProcessorResponse2x::get_applicable_frame_type() const
        {
            return FrameType::FRAME_TYPE_RESPONSE;
        }

        void FrameProcessorResponse2x::_specific_process(const Frame &frame, AirCon &aircon) const
        {
            aircon.set_last_frame(frame);

            bool state_changed = false;

            // TODO: doublecheck the temperature bytes. Probably here is a mess...

            // air conditioner type:
            // byte 10: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b10
            update_property(aircon.ac_type_inverter, frame.get_bit(10, 5), state_changed);

            // byte 11: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b11

            // iClean + defrost
            // byte 12: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b12
            update_property(aircon.defrost_enabled, frame.get_bit(12, 5), state_changed);

            // real FAN speed
            // byte 13: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b13
            update_property(aircon.real_fan_speed, (ac_fanspeed_real)frame.get_value(13, 0b00000111), state_changed);

            // byte 14: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b14

            // ambient indoor temperature:
            // byte 15: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b15
            // byte 31: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b31
            update_property(aircon.current_temperature, (float)(frame.get_value(31, 0b00001111) / 10.0 + frame.get_value(15) - 0x20), state_changed);

            // byte 16: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b16

            // indoor coil temperature:
            // byte 17: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b17
            if (frame.get_value(17) >= 0x20)
                update_property(aircon.temperature_indoor_coil, (uint8_t)(frame.get_value(17) - 0x20), state_changed);
            else
                aircon.temperature_indoor_coil.reset();

            // byte 18: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b18
            // TODO:  maybe this is a mess
            if (frame.get_value(18) >= 0x20)
                update_property(aircon.temperature_outdoor_ambient, (uint8_t)(frame.get_value(18) - 0x20), state_changed);
            else
                aircon.temperature_outdoor_ambient.reset();

            // byte 19: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b19

            // condenser middle temperature sensor:
            // byte 20: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b20
            if (frame.get_value(20) >= 0x20)
                update_property(aircon.temperature_condenser_middle, (uint8_t)(frame.get_value(20) - 0x20), state_changed);
            else
                aircon.temperature_condenser_middle.reset();

            // temperature sensor #2 "PIPE"?:
            // byte 21: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b21
            // This byte is equal to 0x20 for inverters without this sensor.
            // This byte is equal to 0x00 for on-off air conditioners.
            if (frame.get_value(21) >= 0x20)
                update_property(aircon.temperature_outdoor_suction, (uint8_t)(frame.get_value(21) - 0x20), state_changed);
            else
                aircon.temperature_outdoor_suction.reset();

            // compressor temperature:
            // byte 22: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b22
            if (frame.get_value(22, 0b01111111) >= 0x20)
                update_property(aircon.temperature_outdoor_discharge, (uint8_t)(frame.get_value(22, 0b01111111) - 0x20), state_changed);
            else
                aircon.temperature_outdoor_discharge.reset();

            // byte 23: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b23
            // TODO:  maybe this is a mess
            if (frame.get_value(23) >= 0x20)
                update_property(aircon.temperature_outdoor_defrost, (uint8_t)(frame.get_value(23) - 0x20), state_changed);
            else
                aircon.temperature_outdoor_defrost.reset();

            // inverter power (0..100 %)
            // byte 24: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b24
            if (aircon.ac_type_inverter)
                update_property(aircon.inverter_power, frame.get_value(24, 0b01111111), state_changed);
            else
                aircon.inverter_power.reset();

            // byte 25: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b25
            // byte 26: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b26
            // byte 27: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b27
            // byte 28: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b28
            // byte 29: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b29
            // byte 30: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b30

            //  ambient temperature fractional part (see byte 15)
            // byte 31: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b31

            if (state_changed)
                aircon.publish_all_states();
        }

    } // namespace aux_airconditioner
} // namespace esphome