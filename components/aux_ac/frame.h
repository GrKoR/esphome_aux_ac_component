#pragma once

#include <string>
#include <stdint.h>
#include <stddef.h>
#include <vector>
#include "esphome.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/uart/uart_component.h"

#include "frame_constants.h"

namespace esphome
{
    namespace aux_airconditioner
    {

        using esphome::uart::UARTComponent;

        // **************************************************************************************************
        class Frame
        {
        protected:
            // Frame header params
            static const uint8_t FRAME_HEADER_SIZE = 8;
            static const uint8_t OFFSET_START_BYTE = 0;
            static const uint8_t OFFSET_FRAME_TYPE = 2;
            static const uint8_t OFFSET_FRAME_DIRECTION = 3;
            static const uint8_t OFFSET_BODY_LENGTH = 6;

            union crc16_t
            {
                uint16_t crc16;
                uint8_t crc[2];
            } __attribute__((packed));
            static_assert(sizeof(crc16_t) == 2);

            uint32_t _frame_time = 0;
            std::vector<uint8_t> _data = {};
            FrameState _state = FRAME_STATE_BLANK;
            static const uint8_t START_BYTE = 0xBB;

            bool _is_header_loaded() const;
            crc16_t _calc_crc(uint8_t data_size) const;
            FrameState _set_frame_state(FrameState state);
            static std::string _dump_data(const uint8_t *data, uint8_t data_length);

        public:
            Frame() : _frame_time(0){};
            Frame(uint32_t time) : _frame_time(time){};
            Frame(uint32_t time, FrameType frame_type, FrameDirection frame_direction)
                : _frame_time(time),
                  _data({START_BYTE, 0x00, frame_type, frame_direction, 0x00, 0x00, 0x00, 0x00}) { this->update_frame_state(); }
            Frame(uint32_t time, std::vector<uint8_t> data)
                : _frame_time(time),
                  _data(data) { this->update_frame_state(); }
            ~Frame() = default;

            static uint8_t get_start_byte() { return Frame::START_BYTE; };

            bool has_type(FrameType frame_type) const { return get_frame_type() == frame_type; };
            FrameType get_frame_type() const;
            Frame &set_frame_type(FrameType frame_type);

            uint8_t get_body_length() const;
            Frame &set_body_length(uint8_t body_length);

            FrameDirection get_frame_dir() const;
            Frame &set_frame_dir(FrameDirection frame_direction);

            uint32_t get_frame_time() { return this->_frame_time; };
            Frame &set_frame_time(uint32_t time);

            Frame &clear();

            bool send(UARTComponent &uart);
            FrameState load(UARTComponent &uart);

            Frame &append_data(uint8_t data, bool update_state = false);
            Frame &append_data(const uint8_t data, const uint8_t count, bool update_state = false);
            Frame &append_data(std::vector<uint8_t> data, bool update_state = false);
            Frame &append_data(const uint8_t *data, uint8_t data_length, bool update_state = false);
            Frame &trim_data(uint8_t first_element_index);
            Frame &update_crc(bool update_state = false);
            bool is_valid_crc() const;
            bool is_valid_frame() const { return this->has_frame_state(FRAME_STATE_OK); };

            bool get_bit(uint8_t data_index, uint8_t bit_index) const;
            Frame &set_bit(uint8_t data_index, uint8_t bit_index, bool value);
            uint8_t get_value(uint8_t index, uint8_t mask = 255, uint8_t shift = 0) const;
            Frame &set_value(uint8_t index, uint8_t value, uint8_t mask = 255, uint8_t shift = 0);

            bool get_crc(uint16_t &crc16) const;
            bool get_crc(uint8_t &crc16_1, uint8_t &crc16_2) const;

            FrameState get_frame_state() const { return this->_state; };
            bool has_frame_state(FrameState frame_state) const { return this->get_frame_state() == frame_state; };
            FrameState update_frame_state();

            const uint8_t *data() const { return this->_data.data(); };
            uint8_t size() const { return this->_data.size(); };

            std::string to_string(bool show_time = false) const;
            std::string state_to_string() const;
            std::string type_to_string() const;
            std::string direction_to_string() const;
        };

    } // namespace aux_ac
} // namespace esphome
