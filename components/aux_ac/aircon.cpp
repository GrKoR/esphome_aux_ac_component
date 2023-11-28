#include "aircon.h"
#include "helpers.h"
#include "command_builder.h"
#include "frame_processor_manager.h"
#include "esphome/core/log.h"

namespace esphome
{
    namespace aux_airconditioner
    {
        /*************************************************************************************************\
        \*************************************************************************************************/
        uint32_t Capabilities::normilize_packet_timeout(uint32_t timeout)
        {
            uint32_t result = timeout;
            if (result > Capabilities::AC_PACKET_TIMEOUT_MAX)
                result = Capabilities::AC_PACKET_TIMEOUT_MAX;
            else if (result < Capabilities::AC_PACKET_TIMEOUT_MIN)
                result = Capabilities::AC_PACKET_TIMEOUT_MIN;
            return result;
        }

        float Capabilities::normilize_target_temperature(const float target_temperature)
        {
            float result = target_temperature;
            if (result > Capabilities::AC_MAX_TEMPERATURE)
                result = Capabilities::AC_MAX_TEMPERATURE;
            else if (result < Capabilities::AC_MIN_TEMPERATURE)
                result = Capabilities::AC_MIN_TEMPERATURE;
            return result;
        }

        uint8_t Capabilities::normilize_inverter_power_limit(const uint8_t power_limit_value)
        {
            uint8_t result = power_limit_value;
            if (result > Capabilities::AC_MAX_INVERTER_POWER_LIMIT)
                result = Capabilities::AC_MAX_INVERTER_POWER_LIMIT;
            else if (result < Capabilities::AC_MIN_INVERTER_POWER_LIMIT)
                result = Capabilities::AC_MIN_INVERTER_POWER_LIMIT;
            return result;
        }

        // **************************************************************************************************
        const std::string Capabilities::AC_FIRMWARE_VERSION = "1.0.0 beta 1";

        // **************************************************************************************************
        // custom fan modes
        const std::string Capabilities::CUSTOM_FAN_MODE_MUTE = "MUTE";
        const std::string Capabilities::CUSTOM_FAN_MODE_TURBO = "TURBO";
        // **************************************************************************************************
        // custom presets
        const std::string Capabilities::CUSTOM_PRESET_CLEAN = "CLEAN";
        const std::string Capabilities::CUSTOM_PRESET_HEALTH = "HEALTH";
        const std::string Capabilities::CUSTOM_PRESET_ANTIFUNGUS = "ANTIFUNGUS";
        // **************************************************************************************************
        // predefined default params
        const float Capabilities::AC_MIN_TEMPERATURE = 16.0;
        const float Capabilities::AC_MAX_TEMPERATURE = 32.0;
        const float Capabilities::AC_TEMPERATURE_STEP_TARGET = 0.5;
        const float Capabilities::AC_TEMPERATURE_STEP_CURRENT = 0.1;
        const uint8_t Capabilities::AC_MIN_INVERTER_POWER_LIMIT = 30;  // 30%
        const uint8_t Capabilities::AC_MAX_INVERTER_POWER_LIMIT = 100; // 100%
        const uint32_t Capabilities::AC_STATE_REQUEST_INTERVAL = 7000;
        const uint32_t Capabilities::AC_CONNECTION_LOST_TIMEOUT = 4000;
        const uint32_t Capabilities::AC_PACKET_TIMEOUT_MIN = 300;
        const uint32_t Capabilities::AC_PACKET_TIMEOUT_MAX = 800;

        // **************************************************************************************************
        using esphome::helpers::update_property;

        // **************************************************************************************************
        void AirCon::_send_frame_from_tx_queue()
        {
            if (!this->is_hardware_connected() || !this->has_ping())
                return;

            if (this->_tx_frames.empty())
                return;

            Frame *frame = this->_tx_frames.front();
            this->_tx_frames.pop();

            frame->send(*this->_uart);

            if (frame->get_frame_type() == FrameType::FRAME_TYPE_COMMAND)
            {
                _waiting_for_response_timer.reset();
                _waiting_for_response_timer.set_callback([this](TimerInterface *timer)
                                                         { ESP_LOGW(TAG, "Command response timeout!");
                                                     timer->stop();
                                                     this->set_receiver_callback(nullptr); });
                switch (frame->get_value(8))
                {
                case 0x01: // command: set AC mode
                {
                    uint8_t crc16_1 = 0, crc16_2 = 0;
                    frame->get_crc(crc16_1, crc16_2);
                    this->set_receiver_callback([this, crc16_1, crc16_2](Frame &frame)
                                                { if (this->_waiting_for_response_timer.is_enabled() &&
                                                      frame.get_frame_type() == FrameType::FRAME_TYPE_RESPONSE &&
                                                      frame.get_value(9) == 0x01)
                                                  {
                                                    // check the acknowledgement: should be equal to the command CRC
                                                    if (frame.get_value(10) != crc16_1 ||
                                                        frame.get_value(11) != crc16_2)
                                                    {
                                                        ESP_LOGW(TAG, "Command response acknowledgement error!");
                                                    }
                                                    this->_waiting_for_response_timer.stop();
                                                    this->_waiting_for_response_timer.set_callback(helpers::dummy_stopper);
                                                    this->set_receiver_callback(nullptr);
                                                    if (!this->_command_queue.empty() &&
                                                         this->_cmd_processor_state == command_processor_state_t::CMD_PROCESSOR_STATE_CMD_WAS_SENT)
                                                    {
                                                        this->_cmd_processor_state = command_processor_state_t::CMD_PROCESSOR_STATE_POSTCHECK_DONE;
                                                    }
                                                  } });
                    _waiting_for_response_timer.start(this->get_packet_timeout());
                    break;
                }

                case 0x11: // command: request frame 11
                {
                    this->set_receiver_callback([this](Frame &frame)
                                                { if (this->_waiting_for_response_timer.is_enabled() &&
                                                      frame.get_frame_type() == FrameType::FRAME_TYPE_RESPONSE &&
                                                      frame.get_value(9) == 0x11)
                                                  {
                                                    this->_waiting_for_response_timer.stop();
                                                    this->_waiting_for_response_timer.set_callback(helpers::dummy_stopper);
                                                    this->set_receiver_callback(nullptr);
                                                    if (!this->_command_queue.empty() &&
                                                         this->_cmd_processor_state == command_processor_state_t::CMD_PROCESSOR_STATE_WAITING_FOR_F11)
                                                    {
                                                        this->_cmd_processor_state = command_processor_state_t::CMD_PROCESSOR_STATE_PRECHECK_DONE;
                                                    }
                                                  } });
                    _waiting_for_response_timer.start(this->get_packet_timeout());
                    break;
                }

                case 0x21: // command: request frame 21
                {
                    this->set_receiver_callback([this](Frame &frame)
                                                { if (this->_waiting_for_response_timer.is_enabled() &&
                                                      frame.get_frame_type() == FrameType::FRAME_TYPE_RESPONSE &&
                                                      frame.get_value(9) == 0x21)
                                                  {
                                                    this->_waiting_for_response_timer.stop();
                                                    this->_waiting_for_response_timer.set_callback(helpers::dummy_stopper);
                                                    this->set_receiver_callback(nullptr);
                                                  } });
                    _waiting_for_response_timer.start(this->get_packet_timeout());
                    break;
                }

                default:
                    ESP_LOGW(TAG, "Unknown command: 0x%02X", frame->get_value(8));
                    this->_waiting_for_response_timer.set_callback(helpers::dummy_stopper);
                    break;
                }
            }

            delete frame;
        }

        // **************************************************************************************************
        void AirCon::_process_command_queue()
        {
            if (_command_queue.size() == 0)
                return;

            Frame frame;
            ClimateCall &cmd = _command_queue.front();
            switch (this->_cmd_processor_state)
            {
            case CMD_PROCESSOR_STATE_NOT_STARTED:
                this->_cmd_builder->init_new_command(command_type_t::COMMAND_TYPE_REQUEST_11).fill_frame_with_command(frame);
                this->schedule_frame_to_send(frame);
                this->_cmd_processor_state = command_processor_state_t::CMD_PROCESSOR_STATE_WAITING_FOR_F11;
                break;

            case CMD_PROCESSOR_STATE_WAITING_FOR_F11:
                ESP_LOGW(TAG, "It should never have happened: processing the command with state 'CMD_PROCESSOR_STATE_WAITING_FOR_F11'");
                break;

            case CMD_PROCESSOR_STATE_PRECHECK_DONE:
                this->_cmd_builder->init_new_command(cmd).fill_frame_with_command(frame);
                this->schedule_frame_to_send(frame);
                this->_cmd_processor_state = command_processor_state_t::CMD_PROCESSOR_STATE_CMD_WAS_SENT;
                break;

            case CMD_PROCESSOR_STATE_CMD_WAS_SENT:
                ESP_LOGW(TAG, "It should never have happened: processing the command with state 'CMD_PROCESSOR_STATE_CMD_WAS_SENT'");
                break;

            case CMD_PROCESSOR_STATE_POSTCHECK_DONE:
                this->schedule_frame_to_send(*_frame_11_request);
                this->schedule_frame_to_send(*_frame_2x_request);
                this->_cmd_processor_state = command_processor_state_t::CMD_PROCESSOR_STATE_NOT_STARTED;
                _command_queue.pop();
                break;

            default:
                ESP_LOGW(TAG, "unknown command state '0x%02X'!", this->_cmd_processor_state);
                this->_cmd_processor_state = command_processor_state_t::CMD_PROCESSOR_STATE_NOT_STARTED;
                _command_queue.pop();
                break;
            }
        }

        // **************************************************************************************************
        AirCon::AirCon()
        {
            _incoming_frame = new Frame;
            _last_frame_11 = new Frame;
            _last_frame_2x = new Frame;

            _frame_processor_manager = new FrameProcessorManager;
            _frame_processor_manager->set_aircon(*this);

            _cmd_builder = new CommandBuilder(*this);

            this->set_millis(&millis);
            _timer_manager.set_millis_func(&millis);

            _frame_11_request = new Frame;
            this->_cmd_builder->init_new_command(command_type_t::COMMAND_TYPE_REQUEST_11).fill_frame_with_command(*_frame_11_request);

            _frame_2x_request = new Frame;
            this->_cmd_builder->init_new_command(command_type_t::COMMAND_TYPE_REQUEST_21).fill_frame_with_command(*_frame_2x_request);

            _frame_ping_response = new Frame;
            _frame_ping_response->append_data({Frame::get_start_byte(), 0x00, FrameType::FRAME_TYPE_PING, FrameDirection::FRAME_DIR_TO_AC, 0x01, 0x00, 0x08, 0x00});
            _frame_ping_response->append_data({0x1C, 0x27, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
            _frame_ping_response->update_crc(true);
        }

        AirCon::~AirCon()
        {
            delete _cmd_builder;
            delete _frame_processor_manager;

            delete _incoming_frame;
            delete _last_frame_11;
            delete _last_frame_2x;
            delete _frame_11_request;
            delete _frame_2x_request;
            delete _frame_ping_response;
        }

        // **************************************************************************************************
        void AirCon::setup()
        {
            _traits.set_supports_current_temperature(true);
            _traits.set_supports_two_point_target_temperature(false);

            _traits.set_visual_min_temperature(Capabilities::AC_MIN_TEMPERATURE);
            _traits.set_visual_max_temperature(Capabilities::AC_MAX_TEMPERATURE);
            _traits.set_visual_current_temperature_step(Capabilities::AC_TEMPERATURE_STEP_CURRENT);
            _traits.set_visual_target_temperature_step(Capabilities::AC_TEMPERATURE_STEP_TARGET);

            /* + MINIMAL SET */
            _traits.add_supported_mode(ClimateMode::CLIMATE_MODE_OFF);
            _traits.add_supported_mode(ClimateMode::CLIMATE_MODE_FAN_ONLY);
            _traits.add_supported_fan_mode(ClimateFanMode::CLIMATE_FAN_AUTO);
            _traits.add_supported_fan_mode(ClimateFanMode::CLIMATE_FAN_LOW);
            _traits.add_supported_fan_mode(ClimateFanMode::CLIMATE_FAN_MEDIUM);
            _traits.add_supported_fan_mode(ClimateFanMode::CLIMATE_FAN_HIGH);
            _traits.add_supported_swing_mode(ClimateSwingMode::CLIMATE_SWING_OFF);
            _traits.add_supported_preset(ClimatePreset::CLIMATE_PRESET_NONE);

            // if the climate device supports reporting the active current action of the device with the action property.
            //_traits.set_supports_action(this->_show_action);

            _frame11_request_timer.set_callback([this](TimerInterface *timer)
                                                { this->schedule_frame_to_send(*this->_frame_11_request); });
            _timer_manager.register_timer(_frame11_request_timer);
            _frame11_request_timer.start(this->get_period());

            _frame2x_request_timer.set_callback([this](TimerInterface *timer)
                                                { this->schedule_frame_to_send(*this->_frame_2x_request); });
            _timer_manager.register_timer(_frame2x_request_timer);
            _frame2x_request_timer.start(this->get_period());

            _ping_timeout_timer.set_callback([this](TimerInterface *timer)
                                             { this->_has_ping = false;
                                               ESP_LOGW(TAG, "Air conditioner connection lost!"); });
            _timer_manager.register_timer(_ping_timeout_timer);
            _ping_timeout_timer.start(Capabilities::AC_CONNECTION_LOST_TIMEOUT);

            _timer_manager.register_timer(_waiting_for_response_timer);
            _waiting_for_response_timer.stop();

            // schedule initial requests
            this->schedule_frame_to_send(*this->_frame_11_request);
            this->schedule_frame_to_send(*this->_frame_2x_request);
        }

        // **************************************************************************************************
        void AirCon::loop()
        {
            if (!this->is_hardware_connected())
                return;

            _timer_manager.task();

            FrameState frame_state = _incoming_frame->get_frame_state();
            switch (frame_state)
            {
            case FRAME_STATE_BLANK:
            case FRAME_STATE_PARTIALLY_LOADED:
                if (_uart->available() > 0)
                {
                    _incoming_frame->load(*_uart);
                }
                else if (!this->_waiting_for_response_timer.is_enabled())
                {
                    if (!this->_tx_frames.empty())
                    {
                        this->_send_frame_from_tx_queue();
                    }
                    else if (!this->_command_queue.empty())
                    {
                        this->_process_command_queue();
                    }
                }
                break;

            case FRAME_STATE_ERROR:
                _incoming_frame->set_frame_time(this->ms());
                ESP_LOGW(TAG, "Incorrect frame! Frame state: %s, data: %s", _incoming_frame->state_to_string().c_str(), _incoming_frame->to_string(true).c_str());
                _incoming_frame->clear();
                break;

            case FRAME_STATE_OK:
                _incoming_frame->set_frame_time(this->ms());
                ESP_LOGD(TAG, "%s", _incoming_frame->to_string(true).c_str());

                this->_frame_processor_manager->process_frame(*_incoming_frame);

                if (this->_receiver_callback != nullptr)
                    this->_receiver_callback(*_incoming_frame);

                _incoming_frame->clear();
                break;

            default:
                ESP_LOGW(TAG, "Unknown frame state: %d (0x%02X)", frame_state, frame_state);
                break;
            }
        }

        // **************************************************************************************************
        void AirCon::dump_config()
        {
            ESP_LOGCONFIG(TAG, "AUX HVAC:");
            ESP_LOGCONFIG(TAG, "firmware version: %s", Capabilities::AC_FIRMWARE_VERSION.c_str());
            this->dump_traits_(TAG);

            LOG_SENSOR("  ", "Vertical louver state", this->_sensor_vlouver_state);
            LOG_BINARY_SENSOR("  ", "Display", this->_sensor_display_state);
            LOG_BINARY_SENSOR("  ", "Defrost status", this->_sensor_defrost_state);
            LOG_TEXT_SENSOR("  ", "Preset Reporter", this->_sensor_preset_reporter);

            ESP_LOGCONFIG(TAG, "  Temperatures:");
            LOG_SENSOR("    ", "Indoor Ambient", this->_sensor_temperature_indoor_ambient);
            LOG_SENSOR("    ", "Indoor Coil", this->_sensor_temperature_indoor_coil);
            LOG_SENSOR("    ", "Outdoor Ambient", this->_sensor_temperature_outdoor_ambient);
            LOG_SENSOR("    ", "Outdoor Condenser", this->_sensor_temperature_outdoor_condenser_middle);
            LOG_SENSOR("    ", "Outdoor Defrost", this->_sensor_temperature_outdoor_defrost);
            LOG_SENSOR("    ", "Outdoor Discharge", this->_sensor_temperature_outdoor_discharge);
            LOG_SENSOR("    ", "Outdoor Suction", this->_sensor_temperature_outdoor_suction);

            ESP_LOGCONFIG(TAG, "  Inverter Power:");
            LOG_SENSOR("    ", "Actual Value", this->_sensor_inverter_power_actual);
            LOG_SENSOR("    ", "Limit Value", this->_sensor_inverter_power_limit_value);
            LOG_BINARY_SENSOR("    ", "Limitation State", this->_sensor_inverter_power_limit_state);
        };

        // **************************************************************************************************
        void AirCon::control(const esphome::climate::ClimateCall &call)
        {
            bool has_command = false;

            // User requested mode change
            if (call.get_mode().has_value())
            {
                ClimateMode mode = *call.get_mode();
                update_property(this->mode, mode, has_command);
            }

            // User requested fan_mode change
            if (call.get_fan_mode().has_value())
            {
                ClimateFanMode fanmode = *call.get_fan_mode();
                update_property(this->fan_mode, fanmode, has_command);
            }
            else if (call.get_custom_fan_mode().has_value())
            {
                std::string customfanmode = *call.get_custom_fan_mode();
                if ((customfanmode == Capabilities::CUSTOM_FAN_MODE_TURBO) ||
                    (customfanmode == Capabilities::CUSTOM_FAN_MODE_MUTE) ||
                    (customfanmode == ""))
                {
                    update_property(this->custom_fan_mode, customfanmode, has_command);
                }
            }

            // User selected preset
            if (call.get_preset().has_value())
            {
                ClimatePreset preset = *call.get_preset();
                update_property(this->preset, preset, has_command);
            }
            else if (call.get_custom_preset().has_value())
            {
                std::string custom_preset = *call.get_custom_preset();
                if ((custom_preset == Capabilities::CUSTOM_PRESET_CLEAN) ||
                    (custom_preset == Capabilities::CUSTOM_PRESET_ANTIFUNGUS) ||
                    (custom_preset == Capabilities::CUSTOM_PRESET_HEALTH) ||
                    (custom_preset == ""))
                {
                    update_property(this->custom_preset, custom_preset, has_command);
                }
            }

            // User requested swing_mode change
            if (call.get_swing_mode().has_value())
            {
                ClimateSwingMode swingmode = *call.get_swing_mode();
                update_property(this->swing_mode, swingmode, has_command);
            }

            // User requested target temperature change
            if (call.get_target_temperature().has_value())
            {
                // it isn't allowed in FAN mode
                if (this->mode != ClimateMode::CLIMATE_MODE_FAN_ONLY)
                    update_property(this->target_temperature, *call.get_target_temperature(), has_command);
            }

            if (has_command)
            {
                this->schedule_command(call);

                if (this->get_optimistic())
                    this->publish_all_states();
            }
        }

        // **************************************************************************************************
        void AirCon::action_display_off()
        {
            _cmd_builder->init_new_command(command_type_t::COMMAND_TYPE_SET_STATE).set_display_state(false);
            Frame frame;
            _cmd_builder->fill_frame_with_command(frame);
            this->schedule_frame_to_send(frame);
        }

        // **************************************************************************************************
        void AirCon::action_display_on()
        {
            _cmd_builder->init_new_command(command_type_t::COMMAND_TYPE_SET_STATE).set_display_state(true);
            Frame frame;
            _cmd_builder->fill_frame_with_command(frame);
            this->schedule_frame_to_send(frame);
        }

        // **************************************************************************************************
        void AirCon::action_set_vlouver_swing()
        {
            _cmd_builder->init_new_command(command_type_t::COMMAND_TYPE_SET_STATE).set_vertical_louver(ac_louver_V::AC_LOUVERV_SWING_UPDOWN);
            Frame frame;
            _cmd_builder->fill_frame_with_command(frame);
            this->schedule_frame_to_send(frame);
        }

        // **************************************************************************************************
        void AirCon::action_set_vlouver_stop()
        {
            _cmd_builder->init_new_command(command_type_t::COMMAND_TYPE_SET_STATE).set_vertical_louver(ac_louver_V::AC_LOUVERV_OFF);
            Frame frame;
            _cmd_builder->fill_frame_with_command(frame);
            this->schedule_frame_to_send(frame);
        }

        // **************************************************************************************************
        void AirCon::action_set_vlouver_top_position()
        {
            _cmd_builder->init_new_command(command_type_t::COMMAND_TYPE_SET_STATE).set_vertical_louver(ac_louver_V::AC_LOUVERV_TOP);
            Frame frame;
            _cmd_builder->fill_frame_with_command(frame);
            this->schedule_frame_to_send(frame);
        }

        // **************************************************************************************************
        void AirCon::action_set_vlouver_middle_above_position()
        {
            _cmd_builder->init_new_command(command_type_t::COMMAND_TYPE_SET_STATE).set_vertical_louver(ac_louver_V::AC_LOUVERV_MIDDLE_ABOVE);
            Frame frame;
            _cmd_builder->fill_frame_with_command(frame);
            this->schedule_frame_to_send(frame);
        }

        // **************************************************************************************************
        void AirCon::action_set_vlouver_middle_position()
        {
            _cmd_builder->init_new_command(command_type_t::COMMAND_TYPE_SET_STATE).set_vertical_louver(ac_louver_V::AC_LOUVERV_MIDDLE);
            Frame frame;
            _cmd_builder->fill_frame_with_command(frame);
            this->schedule_frame_to_send(frame);
        }

        // **************************************************************************************************
        void AirCon::action_set_vlouver_middle_below_position()
        {
            _cmd_builder->init_new_command(command_type_t::COMMAND_TYPE_SET_STATE).set_vertical_louver(ac_louver_V::AC_LOUVERV_MIDDLE_BELOW);
            Frame frame;
            _cmd_builder->fill_frame_with_command(frame);
            this->schedule_frame_to_send(frame);
        }

        // **************************************************************************************************
        void AirCon::action_set_vlouver_bottom()
        {
            _cmd_builder->init_new_command(command_type_t::COMMAND_TYPE_SET_STATE).set_vertical_louver(ac_louver_V::AC_LOUVERV_BOTTOM);
            Frame frame;
            _cmd_builder->fill_frame_with_command(frame);
            this->schedule_frame_to_send(frame);
        }

        // **************************************************************************************************
        void AirCon::action_set_vlouver_position(vlouver_esphome_position_t position)
        {
            _cmd_builder->init_new_command(command_type_t::COMMAND_TYPE_SET_STATE).set_vertical_louver(vlouver_frontend_to_ac_louver_V(position));
            Frame frame;
            _cmd_builder->fill_frame_with_command(frame);
            this->schedule_frame_to_send(frame);
        }

        // **************************************************************************************************
        void AirCon::action_power_limitation_off()
        {
            _cmd_builder->init_new_command(command_type_t::COMMAND_TYPE_SET_STATE).set_inverter_power_limitation_state(false);
            Frame frame;
            _cmd_builder->fill_frame_with_command(frame);
            this->schedule_frame_to_send(frame);
        }

        // **************************************************************************************************
        void AirCon::action_power_limitation_on(uint8_t limit)
        {
            _cmd_builder->init_new_command(command_type_t::COMMAND_TYPE_SET_STATE)
                .set_inverter_power_limitation_state(true)
                .set_inverter_power_limitation_value(Capabilities::normilize_inverter_power_limit(limit));
            Frame frame;
            _cmd_builder->fill_frame_with_command(frame);
            this->schedule_frame_to_send(frame);
        }

        // **************************************************************************************************
        void AirCon::schedule_frame_to_send(const Frame &frame)
        {
            Frame *tx_frame = new Frame(frame);
            _tx_frames.push(tx_frame);
        }

        // **************************************************************************************************
        void AirCon::schedule_ping_response()
        {
            this->schedule_frame_to_send(*_frame_ping_response);
        }

        // **************************************************************************************************
        void AirCon::schedule_command(const ClimateCall &cmd)
        {
            _command_queue.push(cmd);
        }

        // **************************************************************************************************
        Frame &AirCon::get_last_frame_11()
        {
            return *(this->_last_frame_11);
        }

        // **************************************************************************************************
        Frame &AirCon::get_last_frame_2x()
        {
            return *(this->_last_frame_2x);
        }

        // **************************************************************************************************
        void AirCon::set_last_frame(const Frame &frame)
        {
            if (frame.get_frame_type() != FrameType::FRAME_TYPE_RESPONSE)
                return;

            if (frame.get_body_length() < 2) // filter out frames without CMD byte
                return;

            Frame *target_frame = nullptr;
            if (frame.get_value(9) == 0x11)
            {
                target_frame = _last_frame_11;
            }
            else if (frame.get_value(9, 0b11110000) == 0x20)
            {
                target_frame = _last_frame_2x;
            }

            if (target_frame != nullptr)
            {
                target_frame->clear();
                target_frame->append_data(frame.data(), frame.size(), true);
                target_frame->set_frame_time(this->ms());
            }
        }

        // **************************************************************************************************
        void AirCon::_update_sensor_unit_of_measurement(Sensor *sensor)
        {
            if (sensor == nullptr)
                return;

            if (this->temperature_in_fahrenheit && sensor->get_unit_of_measurement() != "°F")
                sensor->set_unit_of_measurement("°F");
            else if (!this->temperature_in_fahrenheit && sensor->get_unit_of_measurement() != "°C")
                sensor->set_unit_of_measurement("°C");
        }

        void AirCon::update_all_sensors_unit_of_measurement()
        {
            this->_update_sensor_unit_of_measurement(_sensor_temperature_indoor_ambient);
            this->_update_sensor_unit_of_measurement(_sensor_temperature_indoor_coil);
            this->_update_sensor_unit_of_measurement(_sensor_temperature_outdoor_condenser_middle);
            this->_update_sensor_unit_of_measurement(_sensor_temperature_outdoor_ambient);
            this->_update_sensor_unit_of_measurement(_sensor_temperature_outdoor_defrost);
            this->_update_sensor_unit_of_measurement(_sensor_temperature_outdoor_discharge);
            this->_update_sensor_unit_of_measurement(_sensor_temperature_outdoor_suction);
        }

        // **************************************************************************************************
        template <typename T>
        void publish_sensor_state(Sensor *sensor, optional<T> new_state)
        {
            if (sensor == nullptr)
                return;

            if (new_state.has_value() && !std::isnan((float)(new_state.value())))
            {
                if (sensor->get_raw_state() == (float)(new_state.value()))
                    return;

                sensor->publish_state((float)(new_state.value()));
                return;
            }

            if (std::isnan(sensor->get_raw_state()))
                return;

            sensor->publish_state(NAN);
        }

        // **************************************************************************************************
        void publish_sensor_state(BinarySensor *sensor, optional<bool> new_state)
        {
            if (sensor == nullptr)
                return;

            if (!new_state.has_value())
                return;

            if (sensor->state == new_state.value())
                return;

            sensor->publish_state(new_state.value());
        }

        // **************************************************************************************************
        void publish_sensor_state(TextSensor *sensor, optional<std::string> new_state)
        {
            if (sensor == nullptr)
                return;

            if (!new_state.has_value())
                return;

            if (sensor->get_raw_state() == new_state.value())
                return;

            sensor->publish_state(new_state.value());
        }

        // **************************************************************************************************
        void AirCon::publish_all_states()
        {
            this->publish_state();

            publish_sensor_state(_sensor_temperature_indoor_ambient, optional<float>(this->current_temperature));
            publish_sensor_state(_sensor_temperature_indoor_coil, this->temperature_indoor_coil);
            publish_sensor_state(_sensor_temperature_outdoor_condenser_middle, this->temperature_condenser_middle);
            publish_sensor_state(_sensor_temperature_outdoor_ambient, this->temperature_outdoor_ambient);
            publish_sensor_state(_sensor_temperature_outdoor_defrost, this->temperature_outdoor_defrost);
            publish_sensor_state(_sensor_temperature_outdoor_discharge, this->temperature_outdoor_discharge);
            publish_sensor_state(_sensor_temperature_outdoor_suction, this->temperature_outdoor_suction);

            publish_sensor_state(_sensor_vlouver_state, optional<uint8_t>(this->get_current_vlouver_frontend_state()));
            publish_sensor_state(_sensor_display_state, optional<bool>(this->display_enabled));
            publish_sensor_state(_sensor_defrost_state, optional<bool>(this->defrost_enabled));

            publish_sensor_state(_sensor_inverter_power_actual, this->inverter_power);
            publish_sensor_state(_sensor_inverter_power_limit_value, this->inverter_power_limitation_value);
            publish_sensor_state(_sensor_inverter_power_limit_state, this->inverter_power_limitation_on);

            std::string state_str = "";
            if (this->preset == ClimatePreset::CLIMATE_PRESET_SLEEP)
            {
                state_str += "SLEEP";
            }
            else if (this->custom_preset.has_value())
            {
                state_str += this->custom_preset.value().c_str();
            }
            else
            {
                state_str += "NONE";
            }
            publish_sensor_state(_sensor_preset_reporter, optional<std::string>(state_str));
        }

        // **************************************************************************************************
        // converts vertical louver state from hardware codes to frontend code
        vlouver_esphome_position_t AirCon::aux_vlouver_to_frontend(const ac_louver_V vLouver)
        {
            switch (vLouver)
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
                ESP_LOGW(TAG, "aux_vlouver_to_frontend: unknown vertical louver hardware state = %u", vLouver);
                return AC_VLOUVER_FRONTEND_STOP;
            }
        }

        // **************************************************************************************************
        // current vertical louver position in esphome codes
        vlouver_esphome_position_t AirCon::get_current_vlouver_frontend_state()
        {
            return aux_vlouver_to_frontend(this->louver_vertical);
        }

        // **************************************************************************************************
        // converts vertical louver position from frontend codes to hardware code
        ac_louver_V AirCon::frontend_vlouver_to_aux(const vlouver_esphome_position_t vLouver)
        {
            switch (vLouver)
            {
            case AC_VLOUVER_FRONTEND_SWING:
                return AC_LOUVERV_SWING_UPDOWN;

            case AC_VLOUVER_FRONTEND_STOP:
                return AC_LOUVERV_OFF;

            case AC_VLOUVER_FRONTEND_TOP:
                return AC_LOUVERV_TOP;

            case AC_VLOUVER_FRONTEND_MIDDLE_ABOVE:
                return AC_LOUVERV_MIDDLE_ABOVE;

            case AC_VLOUVER_FRONTEND_MIDDLE:
                return AC_LOUVERV_MIDDLE;

            case AC_VLOUVER_FRONTEND_MIDDLE_BELOW:
                return AC_LOUVERV_MIDDLE_BELOW;

            case AC_VLOUVER_FRONTEND_BOTTOM:
                return AC_LOUVERV_BOTTOM;

            default:
                ESP_LOGW(TAG, "frontend_vlouver_to_aux: unknown frontend vertical louver state = %u", vLouver);
                return AC_LOUVERV_OFF;
            }
        }
    } // namespace aux_airconditioner
} // namespace esphome
