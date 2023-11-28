#pragma once

#include "esphome/components/climate/climate.h"
#include "esphome/core/optional.h"

namespace esphome
{
    namespace aux_airconditioner
    {
        enum ac_mode : uint8_t
        {
            AC_MODE_AUTO = 0x00,
            AC_MODE_COOL = 0x20,
            AC_MODE_DRY = 0x40,
            AC_MODE_HEAT = 0x80,
            AC_MODE_FAN = 0xC0
        };

        std::string ac_mode_to_string(ac_mode mode);

        using esphome::climate::ClimateMode;
        ClimateMode ac_mode_to_climate_mode(ac_mode mode);
        ac_mode climate_mode_to_ac_mode(ClimateMode mode);

        // vertical louvers position in esphome / HA frontend
        enum vlouver_esphome_position_t : uint8_t
        {
            AC_VLOUVER_FRONTEND_SWING = 0x00,
            AC_VLOUVER_FRONTEND_STOP = 0x01,
            AC_VLOUVER_FRONTEND_TOP = 0x02,
            AC_VLOUVER_FRONTEND_MIDDLE_ABOVE = 0x03,
            AC_VLOUVER_FRONTEND_MIDDLE = 0x04,
            AC_VLOUVER_FRONTEND_MIDDLE_BELOW = 0x05,
            AC_VLOUVER_FRONTEND_BOTTOM = 0x06,
        };

        enum ac_louver_V : uint8_t
        {
            AC_LOUVERV_SWING_UPDOWN = 0x00,
            AC_LOUVERV_TOP = 0x01,
            AC_LOUVERV_MIDDLE_ABOVE = 0x02,
            AC_LOUVERV_MIDDLE = 0x03,
            AC_LOUVERV_MIDDLE_BELOW = 0x04,
            AC_LOUVERV_BOTTOM = 0x05,
            // 0x06 tested and doing nothing
            AC_LOUVERV_OFF = 0x07
        };

        ac_louver_V vlouver_frontend_to_ac_louver_V(const vlouver_esphome_position_t vlouver_frontend);

        std::string ac_louver_V_to_string(ac_louver_V louver);

        enum ac_louver_H : uint8_t
        {
            AC_LOUVERH_SWING_LEFTRIGHT = 0x00,
            // AC_LOUVERH_OFF_AUX = 0x20,  // 0b00100000
            AC_LOUVERH_OFF = 0xE0 // 0b11100000
        };

        std::string ac_louver_H_to_string(ac_louver_H louver);

        enum ac_fanspeed : uint8_t
        {
            AC_FANSPEED_HIGH = 0x20,
            AC_FANSPEED_MEDIUM = 0x40,
            AC_FANSPEED_LOW = 0x60,
            AC_FANSPEED_AUTO = 0xA0
        };

        std::string ac_fanspeed_to_string(ac_fanspeed fanspeed);

        using esphome::climate::ClimateFanMode;
        ClimateFanMode ac_fanspeed_to_climate_fan_mode(ac_fanspeed fanspeed);
        ac_fanspeed climate_fan_mode_to_ac_fanspeed(ClimateFanMode fanmode);

        enum ac_fanspeed_real : uint8_t
        {
            AC_REAL_FAN_OFF = 0x00,
            AC_REAL_FAN_MUTE = 0x01,
            AC_REAL_FAN_LOW = 0x02,
            AC_REAL_FAN_MID = 0x04,
            AC_REAL_FAN_HIGH = 0x06,
            AC_REAL_FAN_TURBO = 0x07
        };

        std::string ac_fanspeed_real_to_string(ac_fanspeed_real real_fanspeed);

        enum command_type_t : uint8_t
        {
            COMMAND_TYPE_NONE = 0x00,
            COMMAND_TYPE_REQUEST_11 = 0x01,
            COMMAND_TYPE_REQUEST_21 = 0x02,
            COMMAND_TYPE_SET_STATE = 0x03,
        };

        enum command_processor_state_t : uint8_t
        {
            CMD_PROCESSOR_STATE_NOT_STARTED = 0x00,
            CMD_PROCESSOR_STATE_WAITING_FOR_F11 = 0x01,
            CMD_PROCESSOR_STATE_PRECHECK_DONE = 0x02,
            CMD_PROCESSOR_STATE_CMD_WAS_SENT = 0x03,
            CMD_PROCESSOR_STATE_POSTCHECK_DONE = 0x04,
        };

    } // namespace aux_airconditioner
} // namespace GrKoR