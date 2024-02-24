#pragma once

#include <math.h> // for NAN
#include <queue>

#include "esphome.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/uart/uart_component.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/core/optional.h"

#include "frame.h"
#include "aircon_common.h"
#include "helpers.h"

namespace esphome
{
    namespace aux_airconditioner
    {

        using esphome::Component;
        using esphome::binary_sensor::BinarySensor;
        using esphome::climate::Climate;
        using esphome::climate::ClimateCall;
        using esphome::climate::ClimatePreset;
        using esphome::climate::ClimateSwingMode;
        using esphome::climate::ClimateTraits;
        using esphome::sensor::Sensor;
        using esphome::text_sensor::TextSensor;
        using esphome::uart::UARTComponent;

        using esphome::helpers::Timer;
        using esphome::helpers::TimerInterface;
        using esphome::helpers::TimerManager;

        using millis_function_t = uint32_t (*)();

        static const char *const TAG = "AirCon";

        /*************************************************************************************************\
        \*************************************************************************************************/
        class Capabilities
        {
        public:
            // **************************************************************************************************
            static const std::string AC_FIRMWARE_VERSION;
            // **************************************************************************************************
            // custom fan modes
            static const std::string CUSTOM_FAN_MODE_MUTE;
            static const std::string CUSTOM_FAN_MODE_TURBO;
            // **************************************************************************************************
            // custom presets
            static const std::string CUSTOM_PRESET_CLEAN;
            static const std::string CUSTOM_PRESET_HEALTH;
            static const std::string CUSTOM_PRESET_ANTIFUNGUS;
            // **************************************************************************************************
            // predefined default params
            static const uint32_t AC_STATE_REQUEST_INTERVAL;
            static const uint32_t AC_CONNECTION_LOST_TIMEOUT;

            static const uint32_t AC_PACKET_TIMEOUT_MIN;
            static const uint32_t AC_PACKET_TIMEOUT_MAX;
            static uint32_t normilize_packet_timeout(uint32_t timeout);

            static const float AC_TEMPERATURE_STEP_TARGET;
            static const float AC_TEMPERATURE_STEP_CURRENT;
            static const float AC_MIN_TEMPERATURE;
            static const float AC_MAX_TEMPERATURE;
            static float normilize_target_temperature(const float target_temperature);

            static const uint8_t AC_MIN_INVERTER_POWER_LIMIT;
            static const uint8_t AC_MAX_INVERTER_POWER_LIMIT;
            static uint8_t normilize_inverter_power_limit(const uint8_t power_limit_value);
        };

        /*************************************************************************************************\
        \*************************************************************************************************/
        class FrameProcessorManager;
        class CommandBuilder;
        class Frame;

        class AirCon : public Component,
                       public Climate
        {
        private:
            void _update_sensor_unit_of_measurement(Sensor *sensor);

        protected:
            // esphome sensors that display the parameters of the air conditioner
            Sensor *_sensor_temperature_indoor_ambient{nullptr};
            Sensor *_sensor_temperature_indoor_coil{nullptr};
            Sensor *_sensor_temperature_outdoor_ambient{nullptr};
            Sensor *_sensor_temperature_outdoor_condenser_middle{nullptr};
            Sensor *_sensor_temperature_outdoor_defrost{nullptr};
            Sensor *_sensor_temperature_outdoor_discharge{nullptr};
            Sensor *_sensor_temperature_outdoor_suction{nullptr};

            Sensor *_sensor_vlouver_state{nullptr};
            BinarySensor *_sensor_display_state{nullptr};
            BinarySensor *_sensor_defrost_state{nullptr};
            TextSensor *_sensor_preset_reporter{nullptr};
            Sensor *_sensor_inverter_power_actual{nullptr};
            Sensor *_sensor_inverter_power_limit_value{nullptr};
            BinarySensor *_sensor_inverter_power_limit_state{nullptr};

            ClimateTraits _traits;
            UARTComponent *_uart{nullptr};
            bool _display_inverted{false};
            bool _optimistic{true}; // in optimistic mode, the entity states are updated immediately after receiving a command from Home Assistant/ESPHome
            uint32_t _update_period{Capabilities::AC_STATE_REQUEST_INTERVAL};
            uint32_t _packet_timeout{Capabilities::AC_PACKET_TIMEOUT_MIN};

            bool _has_ping{false};
            millis_function_t _millis_func{nullptr};

            std::queue<Frame *> _tx_frames;
            Frame *_incoming_frame{nullptr};
            Frame *_last_frame_11{nullptr};
            Frame *_last_frame_2x{nullptr};
            Frame *_frame_ping_response{nullptr};
            Frame *_frame_11_request{nullptr};
            Frame *_frame_2x_request{nullptr};
            FrameProcessorManager *_frame_processor_manager{nullptr};

            std::queue<ClimateCall> _command_queue;
            command_processor_state_t _cmd_processor_state{CMD_PROCESSOR_STATE_NOT_STARTED};

            CommandBuilder *_cmd_builder{nullptr};

            void _send_frame_from_tx_queue();
            void _process_command_queue();

            TimerManager _timer_manager;
            Timer _frame11_request_timer;
            Timer _frame2x_request_timer;
            Timer _waiting_for_response_timer;
            Timer _ping_timeout_timer;

            std::function<void(Frame &)> _receiver_callback = nullptr;

        public:
            AirCon();
            ~AirCon();

            // **************************************************************************************************
            // derived methods
            float get_setup_priority() const override { return esphome::setup_priority::DATA; }
            virtual ClimateTraits traits() override { return _traits; }
            virtual void setup() override;
            virtual void loop() override;
            virtual void dump_config() override;
            virtual void control(const esphome::climate::ClimateCall &call) override;

            // **************************************************************************************************
            // current state
            // ------- derived from Climate parameters -------
            // ClimateMode mode{CLIMATE_MODE_OFF};             /// The active mode of the climate device.
            // ClimateAction action{CLIMATE_ACTION_OFF};       /// The active state of the climate device.
            // float current_temperature{NAN};                 /// The current temperature of the climate device, as reported from the integration.
            // float target_temperature;                       /// The target temperature of the climate device.
            // ClimateFanMode fan_mode{CLIMATE_FAN_OFF};       /// The active fan mode of the climate device.
            // std::string custom_fan_mode{};                  /// The active custom fan mode of the climate device.
            // ClimateSwingMode swing_mode{CLIMATE_SWING_OFF}; /// The active swing mode of the climate device.
            // ClimatePreset preset{CLIMATE_PRESET_NONE};      /// The active preset of the climate device.
            // std::string custom_preset{};                    /// The active custom preset mode of the climate device.
            // ------- own parameters -------
            ac_louver_V louver_vertical{AC_LOUVERV_OFF};
            ac_louver_H louver_horizontal{AC_LOUVERH_OFF};
            bool temperature_in_fahrenheit{false};
            bool display_enabled{true};
            uint8_t last_IR_passed{0}; // time since last IR-remote command passed

            optional<bool> inverter_power_limitation_on{false};
            optional<uint8_t> inverter_power_limitation_value{100};
            bool ac_type_inverter{false};

            optional<uint8_t> temperature_indoor_coil{};       // byte 17, cmd=0x21
            optional<uint8_t> temperature_condenser_middle{};  // byte 20, cmd=0x21
            optional<uint8_t> temperature_outdoor_ambient{};   // byte 18, cmd=0x21
            optional<uint8_t> temperature_outdoor_suction{};   // byte 21, cmd=0x21
            optional<uint8_t> temperature_outdoor_discharge{}; // byte 22, cmd=0x21
            optional<uint8_t> temperature_outdoor_defrost{};   // byte 23, cmd=0x21
            ac_fanspeed_real real_fan_speed{AC_REAL_FAN_OFF};
            optional<uint8_t> inverter_power{0};
            bool defrost_enabled{false};

            // **************************************************************************************************
            // settings & config
            void set_uart(UARTComponent &uart) { _uart = &uart; }
            void set_uart(UARTComponent *uart) { _uart = uart; }
            UARTComponent &get_uart() { return *_uart; }
            bool is_hardware_connected() { return _uart != nullptr; }
            bool has_ping() { return this->_has_ping; }
            void reset_ping_timeout()
            {
                this->_has_ping = true;
                this->_ping_timeout_timer.reset();
            }
            void set_millis(millis_function_t millis) { _millis_func = millis; }
            uint32_t ms() { return (_millis_func != nullptr) ? _millis_func() : 0; }
            void set_display_inversion(bool inversion) { _display_inverted = inversion; }
            bool get_display_inversion() { return _display_inverted; }
            void set_optimistic(bool optimistic) { this->_optimistic = optimistic; }
            bool get_optimistic() { return this->_optimistic; }
            void set_period(uint32_t ms) { this->_update_period = ms; }
            uint32_t get_period() { return this->_update_period; }
            void set_packet_timeout(uint32_t ms) { this->_packet_timeout = Capabilities::normilize_packet_timeout(ms); }
            uint32_t get_packet_timeout() { return this->_packet_timeout; }
            void set_supported_modes(const std::set<ClimateMode> &modes) { _traits.set_supported_modes(modes); }
            void set_supported_swing_modes(const std::set<ClimateSwingMode> &modes) { _traits.set_supported_swing_modes(modes); }
            void set_supported_presets(const std::set<ClimatePreset> &presets) { _traits.set_supported_presets(presets); }
            void set_custom_presets(const std::set<std::string> &presets) { _traits.set_supported_custom_presets(presets); }
            void set_custom_fan_modes(const std::set<std::string> &modes) { _traits.set_supported_custom_fan_modes(modes); }

            // **************************************************************************************************
            // setters for sensors
            void set_sensor_temperature_indoor_ambient(Sensor *temperature_sensor) { _sensor_temperature_indoor_ambient = temperature_sensor; }
            void set_sensor_temperature_indoor_coil(Sensor *temperature_sensor) { _sensor_temperature_indoor_coil = temperature_sensor; }
            void set_sensor_temperature_outdoor_ambient(Sensor *temperature_sensor) { _sensor_temperature_outdoor_ambient = temperature_sensor; }
            void set_sensor_temperature_outdoor_condenser_middle(Sensor *temperature_sensor) { _sensor_temperature_outdoor_condenser_middle = temperature_sensor; }
            void set_sensor_temperature_outdoor_defrost(Sensor *temperature_sensor) { _sensor_temperature_outdoor_defrost = temperature_sensor; }
            void set_sensor_temperature_outdoor_discharge(Sensor *temperature_sensor) { _sensor_temperature_outdoor_discharge = temperature_sensor; }
            void set_sensor_temperature_outdoor_suction(Sensor *temperature_sensor) { _sensor_temperature_outdoor_suction = temperature_sensor; }

            void set_sensor_vlouver_state(Sensor *sensor) { _sensor_vlouver_state = sensor; }
            void set_sensor_display(BinarySensor *sensor) { _sensor_display_state = sensor; }
            void set_sensor_defrost_state(BinarySensor *sensor) { _sensor_defrost_state = sensor; }
            void set_sensor_preset_reporter(TextSensor *sensor) { _sensor_preset_reporter = sensor; }

            void set_sensor_inverter_power(Sensor *sensor) { _sensor_inverter_power_actual = sensor; }
            void set_sensor_inverter_power_limit_value(Sensor *sensor) { _sensor_inverter_power_limit_value = sensor; }
            void set_sensor_inverter_power_limit_state(BinarySensor *sensor) { _sensor_inverter_power_limit_state = sensor; }

            // **************************************************************************************************
            // actions
            void action_display_off();
            void action_display_on();
            void action_set_vlouver_swing();
            void action_set_vlouver_stop();
            void action_set_vlouver_top_position();
            void action_set_vlouver_middle_above_position();
            void action_set_vlouver_middle_position();
            void action_set_vlouver_middle_below_position();
            void action_set_vlouver_bottom();
            void action_set_vlouver_position(vlouver_esphome_position_t position);
            void action_power_limitation_off();
            void action_power_limitation_on(uint8_t limit);

            // **************************************************************************************************
            // other methods
            void schedule_frame_to_send(const Frame &frame);
            void schedule_ping_response();
            void schedule_command(const ClimateCall &cmd);
            Frame &get_last_frame_11();
            Frame &get_last_frame_2x();
            void set_last_frame(const Frame &frame);
            void update_all_sensors_unit_of_measurement();
            void publish_all_states();
            void set_receiver_callback(std::function<void(Frame &)> callback) { this->_receiver_callback = callback; }

            // converts vertical louver state from hardware codes to frontend codes
            vlouver_esphome_position_t aux_vlouver_to_frontend(const ac_louver_V vLouver);

            // current vertical louver position in esphome codes
            vlouver_esphome_position_t get_current_vlouver_frontend_state();

            // converts vertical louver position from frontend codes to hardware code
            ac_louver_V frontend_vlouver_to_aux(const vlouver_esphome_position_t vLouver);
        };

    } // namespace aux_airconditioner
} // namespace esphome
