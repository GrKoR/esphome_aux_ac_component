#include "aircon_common.h"

namespace esphome
{
    namespace aux_airconditioner
    {
        std::string ac_mode_to_string(ac_mode mode)
        {
            switch (mode)
            {
            case AC_MODE_AUTO:
                return "AC_MODE_AUTO";

            case AC_MODE_COOL:
                return "AC_MODE_COOL";

            case AC_MODE_DRY:
                return "AC_MODE_DRY";

            case AC_MODE_HEAT:
                return "AC_MODE_HEAT";

            case AC_MODE_FAN:
                return "AC_MODE_FAN";

            default:
                return "mode unknown";
            }
        }

        ClimateMode ac_mode_to_climate_mode(ac_mode mode)
        {
            switch (mode)
            {
            case AC_MODE_AUTO:
                return ClimateMode::CLIMATE_MODE_HEAT_COOL;

            case AC_MODE_COOL:
                return ClimateMode::CLIMATE_MODE_COOL;

            case AC_MODE_DRY:
                return ClimateMode::CLIMATE_MODE_DRY;

            case AC_MODE_HEAT:
                return ClimateMode::CLIMATE_MODE_HEAT;

            case AC_MODE_FAN:
                return ClimateMode::CLIMATE_MODE_FAN_ONLY;

            default:
                return ClimateMode::CLIMATE_MODE_OFF;
            }
        }

        ac_mode climate_mode_to_ac_mode(ClimateMode mode)
        {
            switch (mode)
            {
            case ClimateMode::CLIMATE_MODE_HEAT_COOL:
                return AC_MODE_AUTO;

            case ClimateMode::CLIMATE_MODE_COOL:
                return AC_MODE_COOL;

            case ClimateMode::CLIMATE_MODE_DRY:
                return AC_MODE_DRY;

            case ClimateMode::CLIMATE_MODE_HEAT:
                return AC_MODE_HEAT;

            case ClimateMode::CLIMATE_MODE_FAN_ONLY:
                return AC_MODE_FAN;

            default:
                return AC_MODE_FAN;
            }
        }

        ac_louver_V vlouver_frontend_to_ac_louver_V(const vlouver_esphome_position_t vlouver_frontend)
        {
            switch (vlouver_frontend)
            {
            case AC_VLOUVER_FRONTEND_SWING:
                return ac_louver_V::AC_LOUVERV_SWING_UPDOWN;

            case AC_VLOUVER_FRONTEND_STOP:
                return ac_louver_V::AC_LOUVERV_OFF;

            case AC_VLOUVER_FRONTEND_TOP:
                return ac_louver_V::AC_LOUVERV_SWING_UPDOWN;

            case AC_VLOUVER_FRONTEND_MIDDLE_ABOVE:
                return ac_louver_V::AC_LOUVERV_MIDDLE_ABOVE;

            case AC_VLOUVER_FRONTEND_MIDDLE:
                return ac_louver_V::AC_LOUVERV_MIDDLE;

            case AC_VLOUVER_FRONTEND_MIDDLE_BELOW:
                return ac_louver_V::AC_LOUVERV_MIDDLE_BELOW;

            case AC_VLOUVER_FRONTEND_BOTTOM:
                return ac_louver_V::AC_LOUVERV_BOTTOM;

            default:
                return ac_louver_V::AC_LOUVERV_OFF;
            }
        }

        vlouver_esphome_position_t ac_louver_V_to_vlouver_frontend(const ac_louver_V aux_vlouver)
        {
            switch (aux_vlouver)
            {
            case AC_LOUVERV_SWING_UPDOWN:
                return AC_VLOUVER_FRONTEND_SWING;

            case AC_LOUVERV_OFF:
                return AC_VLOUVER_FRONTEND_STOP;

            case AC_LOUVERV_TOP:
                return AC_VLOUVER_FRONTEND_TOP;

            case AC_LOUVERV_MIDDLE_ABOVE:
                return AC_VLOUVER_FRONTEND_MIDDLE_ABOVE;

            case AC_LOUVERV_MIDDLE:
                return AC_VLOUVER_FRONTEND_MIDDLE;

            case AC_LOUVERV_MIDDLE_BELOW:
                return AC_VLOUVER_FRONTEND_MIDDLE_BELOW;

            case AC_LOUVERV_BOTTOM:
                return AC_VLOUVER_FRONTEND_BOTTOM;

            default:
                return AC_VLOUVER_FRONTEND_STOP;
            }
        }

        std::string ac_louver_V_to_string(ac_louver_V louver)
        {
            switch (louver)
            {
            case AC_LOUVERV_SWING_UPDOWN:
                return "AC_LOUVERV_SWING_UPDOWN";

            case AC_LOUVERV_TOP:
                return "AC_LOUVERV_TOP";

            case AC_LOUVERV_MIDDLE_ABOVE:
                return "AC_LOUVERV_MIDDLE_ABOVE";

            case AC_LOUVERV_MIDDLE:
                return "AC_LOUVERV_MIDDLE";

            case AC_LOUVERV_MIDDLE_BELOW:
                return "AC_LOUVERV_MIDDLE_BELOW";

            case AC_LOUVERV_BOTTOM:
                return "AC_LOUVERV_BOTTOM";

            case AC_LOUVERV_OFF:
                return "AC_LOUVERV_OFF";

            default:
                return "unknown vertical louver position";
            }
        }

        std::string ac_louver_H_to_string(ac_louver_H louver)
        {
            switch (louver)
            {
            case AC_LOUVERH_SWING_LEFTRIGHT:
                return "AC_LOUVERH_SWING_LEFTRIGHT";

            case AC_LOUVERH_OFF:
                return "AC_LOUVERH_OFF";

            default:
                return "unknown horizontal louver position";
            }
        }

        std::string ac_fanspeed_to_string(ac_fanspeed fanspeed)
        {
            switch (fanspeed)
            {
            case AC_FANSPEED_HIGH:
                return "AC_FANSPEED_HIGH";

            case AC_FANSPEED_MEDIUM:
                return "AC_FANSPEED_MEDIUM";

            case AC_FANSPEED_LOW:
                return "AC_FANSPEED_LOW";

            case AC_FANSPEED_AUTO:
                return "AC_FANSPEED_AUTO";

            default:
                return "unknown";
            }
        }

        ClimateFanMode ac_fanspeed_to_climate_fan_mode(ac_fanspeed fanspeed)
        {
            switch (fanspeed)
            {
            case AC_FANSPEED_HIGH:
                return ClimateFanMode::CLIMATE_FAN_HIGH;

            case AC_FANSPEED_MEDIUM:
                return ClimateFanMode::CLIMATE_FAN_MEDIUM;

            case AC_FANSPEED_LOW:
                return ClimateFanMode::CLIMATE_FAN_LOW;

            case AC_FANSPEED_AUTO:
                return ClimateFanMode::CLIMATE_FAN_AUTO;

            default:
                return ClimateFanMode::CLIMATE_FAN_LOW;
            }
        }

        ac_fanspeed climate_fan_mode_to_ac_fanspeed(ClimateFanMode fanmode)
        {
            switch (fanmode)
            {
            case ClimateFanMode::CLIMATE_FAN_AUTO:
                return AC_FANSPEED_AUTO;

            case ClimateFanMode::CLIMATE_FAN_LOW:
                return AC_FANSPEED_LOW;

            case ClimateFanMode::CLIMATE_FAN_MEDIUM:
                return AC_FANSPEED_MEDIUM;

            case ClimateFanMode::CLIMATE_FAN_HIGH:
                return AC_FANSPEED_HIGH;

            default:
                return ac_fanspeed::AC_FANSPEED_LOW;
            }
        }

        std::string ac_fanspeed_real_to_string(ac_fanspeed_real real_fanspeed)
        {
            switch (real_fanspeed)
            {
            case AC_REAL_FAN_OFF:
                return "AC_REAL_FAN_OFF";

            case AC_REAL_FAN_MUTE:
                return "AC_REAL_FAN_MUTE";

            case AC_REAL_FAN_LOW:
                return "AC_REAL_FAN_LOW";

            case AC_REAL_FAN_MID:
                return "AC_REAL_FAN_MID";

            case AC_REAL_FAN_HIGH:
                return "AC_REAL_FAN_HIGH";

            case AC_REAL_FAN_TURBO:
                return "AC_REAL_FAN_TURBO";

            default:
                return "unknown";
            }
        }

    } // namespace aux_airconditioner
} // namespace esphome