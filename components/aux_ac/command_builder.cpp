#include "command_builder.h"
#include "aircon.h"
#include "frame.h"
#include "esphome/core/optional.h"

namespace esphome
{
    namespace aux_airconditioner
    {

        CommandBuilder::CommandBuilder(AirCon &aircon)
        {
            _aircon = &aircon;
            _command_frame = new Frame;
        }

        CommandBuilder::~CommandBuilder()
        {
            delete _command_frame;
        }

        CommandBuilder &CommandBuilder::init_new_command(command_type_t command_type)
        {
            _command_frame->clear();

            switch (command_type)
            {
            case COMMAND_TYPE_SET_STATE:
                _command_frame->append_data(_aircon->get_last_frame_11().data(), _aircon->get_last_frame_11().size());
                _command_frame->set_frame_dir(FrameDirection::FRAME_DIR_TO_AC).set_frame_type(FrameType::FRAME_TYPE_COMMAND);
                _command_frame->set_value(8, COMMAND).set_value(9, FLAG);
                break;

            case COMMAND_TYPE_REQUEST_11:
                _command_frame->append_data({_command_frame->get_start_byte(), 0x00, FrameType::FRAME_TYPE_COMMAND, FrameDirection::FRAME_DIR_TO_AC, 0x00, 0x00, COMMAND_REQUEST_BODY_LENGTH, 0x00});
                _command_frame->append_data({0x11, FLAG});
                break;

            case COMMAND_TYPE_REQUEST_21:
                _command_frame->append_data({_command_frame->get_start_byte(), 0x00, FrameType::FRAME_TYPE_COMMAND, FrameDirection::FRAME_DIR_TO_AC, 0x00, 0x00, COMMAND_REQUEST_BODY_LENGTH, 0x00});
                _command_frame->append_data({0x21, FLAG});
                break;

            case COMMAND_TYPE_NONE:
            default:
                ESP_LOGW(TAG, "Command type 0x%02X is unsupported", command_type);
                break;
            }

            _command_frame->update_crc(true);
            if (_command_frame->get_frame_state() == FRAME_STATE_OK)
                _command_frame->set_frame_time(_aircon->ms());
            return *this;
        }

        CommandBuilder &CommandBuilder::init_new_command(ClimateCall &cmd)
        {
            this->init_new_command(command_type_t::COMMAND_TYPE_SET_STATE);

            if (cmd.get_mode().has_value())
                this->set_climate_mode(*cmd.get_mode());

            if (cmd.get_fan_mode().has_value())
                this->set_climate_fan_mode(*cmd.get_fan_mode());
            else if (cmd.get_custom_fan_mode().has_value())
                this->set_climate_custom_fan_mode(*cmd.get_custom_fan_mode());

            if (cmd.get_preset().has_value())
                this->set_climate_preset(*cmd.get_preset());
            else if (cmd.get_custom_preset().has_value())
                this->set_climate_custom_preset(*cmd.get_custom_preset());

            if (cmd.get_swing_mode().has_value())
                this->set_climate_swing_mode(*cmd.get_swing_mode());

            if (cmd.get_target_temperature().has_value())
                if (this->_aircon->mode != ClimateMode::CLIMATE_MODE_FAN_ONLY)
                    this->set_target_temperature(*cmd.get_target_temperature());

            return *this;
        }

        CommandBuilder &CommandBuilder::fill_frame_with_command(Frame &frame)
        {
            _command_frame->update_crc(true);
            if (_command_frame->get_frame_state() != FRAME_STATE_OK)
                return *this;

            frame.clear();
            frame.append_data(_command_frame->data(), 8 + _command_frame->get_body_length() + 2, true);
            frame.set_frame_time(_command_frame->get_frame_time());
            return *this;
        }

        Frame CommandBuilder::get_builder_result()
        {
            return *_command_frame;
        }

        CommandBuilder &CommandBuilder::set_climate_mode(ClimateMode value)
        {
            if (_command_frame->get_body_length() != COMMAND_SET_BODY_LENGTH)
                return *this;

            this->set_power(value != ClimateMode::CLIMATE_MODE_OFF);

            if (value == ClimateMode::CLIMATE_MODE_OFF)
                return *this;

            this->set_mode(climate_mode_to_ac_mode(value));
            if (value == ClimateMode::CLIMATE_MODE_FAN_ONLY)
            {
                this->set_sleep_mode(false);
            }
            else if (value == ClimateMode::CLIMATE_MODE_DRY)
            {
                this->set_sleep_mode(false);
                this->set_fan_turbo(false);
            }

            return *this;
        }

        CommandBuilder &CommandBuilder::set_climate_fan_mode(ClimateFanMode value)
        {
            if (_command_frame->get_body_length() != COMMAND_SET_BODY_LENGTH)
                return *this;

            switch (value)
            {
            case ClimateFanMode::CLIMATE_FAN_AUTO:
                this->set_fan_speed(ac_fanspeed::AC_FANSPEED_AUTO);
                this->set_fan_turbo(false);
                this->set_fan_mute(false);
                break;

            case ClimateFanMode::CLIMATE_FAN_LOW:
                this->set_fan_speed(ac_fanspeed::AC_FANSPEED_LOW);
                this->set_fan_turbo(false);
                this->set_fan_mute(false);
                break;

            case ClimateFanMode::CLIMATE_FAN_MEDIUM:
                this->set_fan_speed(ac_fanspeed::AC_FANSPEED_MEDIUM);
                this->set_fan_turbo(false);
                this->set_fan_mute(false);
                break;

            case ClimateFanMode::CLIMATE_FAN_HIGH:
                this->set_fan_speed(ac_fanspeed::AC_FANSPEED_HIGH);
                this->set_fan_turbo(false);
                this->set_fan_mute(false);
                break;

            // Other possible values should be ignored
            case ClimateFanMode::CLIMATE_FAN_ON:
            case ClimateFanMode::CLIMATE_FAN_OFF:
            case ClimateFanMode::CLIMATE_FAN_MIDDLE:
            case ClimateFanMode::CLIMATE_FAN_FOCUS:
            case ClimateFanMode::CLIMATE_FAN_DIFFUSE:
            default:
                break;
            }

            return *this;
        }

        CommandBuilder &CommandBuilder::set_climate_custom_fan_mode(std::string value)
        {
            if (_command_frame->get_body_length() != COMMAND_SET_BODY_LENGTH)
                return *this;

            if (value == Capabilities::CUSTOM_FAN_MODE_TURBO)
            {
                this->set_fan_turbo(true);
                this->set_fan_mute(false);
            }
            else if (value == Capabilities::CUSTOM_FAN_MODE_MUTE)
            {
                this->set_fan_turbo(false);
                this->set_fan_mute(true);
            }
            else
            {
                this->set_fan_turbo(false);
                this->set_fan_mute(false);
            }

            return *this;
        }

        CommandBuilder &CommandBuilder::set_climate_preset(ClimatePreset value)
        {
            if (_command_frame->get_body_length() != COMMAND_SET_BODY_LENGTH)
                return *this;

            switch (value)
            {
            case ClimatePreset::CLIMATE_PRESET_SLEEP:
                // SLEEP function works in COOL and HEAT modes. Some air conditioners allow it in AUTO and DRY mode also.
                // We ignore this. Trying to enable it in any mode.
                this->set_sleep_mode(true);
                this->set_health_mode(false);
                break;

            case ClimatePreset::CLIMATE_PRESET_NONE:
                this->set_health_mode(false);
                this->set_sleep_mode(false);
                this->set_antifungus_mode(false);
                this->set_iClean_mode(false);
                break;

            // all other presets are ignored
            case ClimatePreset::CLIMATE_PRESET_HOME:
            case ClimatePreset::CLIMATE_PRESET_AWAY:
            case ClimatePreset::CLIMATE_PRESET_BOOST:
            case ClimatePreset::CLIMATE_PRESET_COMFORT:
            case ClimatePreset::CLIMATE_PRESET_ECO:
            case ClimatePreset::CLIMATE_PRESET_ACTIVITY:
            default:
                break;
            }

            return *this;
        }

        CommandBuilder &CommandBuilder::set_climate_custom_preset(std::string value)
        {
            if (_command_frame->get_body_length() != COMMAND_SET_BODY_LENGTH)
                return *this;

            if (value == Capabilities::CUSTOM_PRESET_CLEAN)
            {
                this->set_iClean_mode(true);
                this->set_antifungus_mode(false);
            }
            else if (value == Capabilities::CUSTOM_PRESET_HEALTH)
            {
                this->set_health_mode(true);
                this->set_fan_turbo(false);
                this->set_fan_mute(false);
                this->set_sleep_mode(false);
            }
            else if (value == Capabilities::CUSTOM_PRESET_ANTIFUNGUS)
            {
                this->set_antifungus_mode(true);
                this->set_iClean_mode(false);
            }

            return *this;
        }

        CommandBuilder &CommandBuilder::set_climate_swing_mode(ClimateSwingMode value)
        {
            if (_command_frame->get_body_length() != COMMAND_SET_BODY_LENGTH)
                return *this;

            switch (value)
            {
            case ClimateSwingMode::CLIMATE_SWING_OFF:
                this->set_vertical_louver(ac_louver_V::AC_LOUVERV_OFF);
                this->set_horizontal_louver(ac_louver_H::AC_LOUVERH_OFF);
                break;

            case ClimateSwingMode::CLIMATE_SWING_BOTH:
                this->set_vertical_louver(ac_louver_V::AC_LOUVERV_SWING_UPDOWN);
                this->set_horizontal_louver(ac_louver_H::AC_LOUVERH_SWING_LEFTRIGHT);
                break;

            case ClimateSwingMode::CLIMATE_SWING_VERTICAL:
                this->set_vertical_louver(ac_louver_V::AC_LOUVERV_SWING_UPDOWN);
                this->set_horizontal_louver(ac_louver_H::AC_LOUVERH_OFF);
                break;

            case ClimateSwingMode::CLIMATE_SWING_HORIZONTAL:
                this->set_vertical_louver(ac_louver_V::AC_LOUVERV_OFF);
                this->set_horizontal_louver(ac_louver_H::AC_LOUVERH_SWING_LEFTRIGHT);
                break;

            default:
                break;
            }

            return *this;
        }

        CommandBuilder &CommandBuilder::set_target_temperature(float value)
        {
            if (_command_frame->get_body_length() != COMMAND_SET_BODY_LENGTH)
                return *this;

            value = Capabilities::normilize_target_temperature(value);
            _command_frame->set_value(10, (uint8_t)(value - 8), 0b1111'1000, 3);
            _command_frame->set_bit(12, 7, (value - (uint8_t)(value) >= 0.5));
            return *this;
        }

        CommandBuilder &CommandBuilder::set_vertical_louver(ac_louver_V value)
        {
            if (_command_frame->get_body_length() != COMMAND_SET_BODY_LENGTH)
                return *this;

            _command_frame->set_value(10, (uint8_t)value, 0b0000'0111);
            return *this;
        }

        CommandBuilder &CommandBuilder::set_horizontal_louver(ac_louver_H value)
        {
            if (_command_frame->get_body_length() != COMMAND_SET_BODY_LENGTH)
                return *this;

            _command_frame->set_value(11, (uint8_t)value, 0b1110'0000);
            return *this;
        }

        CommandBuilder &CommandBuilder::set_fan_speed(ac_fanspeed value)
        {
            if (_command_frame->get_body_length() != COMMAND_SET_BODY_LENGTH)
                return *this;

            _command_frame->set_value(13, (uint8_t)value, 0b1110'0000);
            return *this;
        }

        CommandBuilder &CommandBuilder::set_fan_turbo(bool value)
        {
            if (_command_frame->get_body_length() != COMMAND_SET_BODY_LENGTH)
                return *this;

            _command_frame->set_bit(14, 6, value);
            if (value)
                _command_frame->set_bit(14, 7, false); // MUTE off
            return *this;
        }

        CommandBuilder &CommandBuilder::set_fan_mute(bool value)
        {
            if (_command_frame->get_body_length() != COMMAND_SET_BODY_LENGTH)
                return *this;

            _command_frame->set_bit(14, 7, value);
            if (value)
                _command_frame->set_bit(14, 6, false); // TURBO off
            return *this;
        }

        CommandBuilder &CommandBuilder::set_mode(ac_mode value)
        {
            if (_command_frame->get_body_length() != COMMAND_SET_BODY_LENGTH)
                return *this;

            _command_frame->set_value(15, (uint8_t)value, 0b1110'0000);

            return *this;
        }

        CommandBuilder &CommandBuilder::set_fahrenheit_temperature(bool value)
        {
            if (_command_frame->get_body_length() != COMMAND_SET_BODY_LENGTH)
                return *this;

            _command_frame->set_bit(15, 1, value);
            return *this;
        }

        CommandBuilder &CommandBuilder::set_sleep_mode(bool value)
        {
            if (_command_frame->get_body_length() != COMMAND_SET_BODY_LENGTH)
                return *this;

            _command_frame->set_bit(15, 2, value);
            return *this;
        }

        CommandBuilder &CommandBuilder::set_power(bool value)
        {
            if (_command_frame->get_body_length() != COMMAND_SET_BODY_LENGTH)
                return *this;

            _command_frame->set_bit(18, 5, value);     // power
            if (value)                                 // iClean should be off in power on mode
                _command_frame->set_bit(18, 2, false); //
            else                                       // Health function should be off in power down mode
                _command_frame->set_bit(18, 1, false); //

            return *this;
        }

        CommandBuilder &CommandBuilder::set_iClean_mode(bool value)
        {
            if (_command_frame->get_body_length() != COMMAND_SET_BODY_LENGTH)
                return *this;

            _command_frame->set_bit(18, 2, value);
            if (value) // iClean works in power off mode only
                _command_frame->set_bit(18, 5, false);
            return *this;
        }

        CommandBuilder &CommandBuilder::set_health_mode(bool value)
        {
            if (_command_frame->get_body_length() != COMMAND_SET_BODY_LENGTH)
                return *this;

            _command_frame->set_bit(18, 1, value);
            if (value) // Health function works in power on mode only
                _command_frame->set_bit(18, 5, true);
            return *this;
        }

        CommandBuilder &CommandBuilder::set_antifungus_mode(bool value)
        {
            if (_command_frame->get_body_length() != COMMAND_SET_BODY_LENGTH)
                return *this;

            _command_frame->set_bit(20, 3, value);
            return *this;
        }

        CommandBuilder &CommandBuilder::set_display_state(bool value)
        {
            if (_command_frame->get_body_length() != COMMAND_SET_BODY_LENGTH)
                return *this;

            _command_frame->set_bit(20, 4, value ^ _aircon->get_display_inversion());
            return *this;
        }

        CommandBuilder &CommandBuilder::set_inverter_power_limitation_state(bool enabled)
        {
            if (_command_frame->get_body_length() != COMMAND_SET_BODY_LENGTH)
                return *this;

            if (!_aircon->ac_type_inverter)
                return *this;

            _command_frame->set_bit(21, 7, enabled);
            return *this;
        }

        CommandBuilder &CommandBuilder::set_inverter_power_limitation_value(uint8_t value)
        {
            if (_command_frame->get_body_length() != COMMAND_SET_BODY_LENGTH)
                return *this;

            if (!_aircon->ac_type_inverter)
                return *this;

            _command_frame->set_value(21, Capabilities::normilize_inverter_power_limit(value), 0b0111'1111);
            return *this;
        }

    } // namespace aux_airconditioner
} // namespace esphome
