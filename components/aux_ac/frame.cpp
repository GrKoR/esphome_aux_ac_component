#include "frame.h"
#include <sstream>
#include <iomanip>

namespace esphome
{
    namespace aux_airconditioner
    {

        bool Frame::_is_header_loaded() const
        {
            return this->size() >= Frame::FRAME_HEADER_SIZE;
        }

        Frame::crc16_t Frame::_calc_crc(uint8_t data_size) const
        {
            Frame::crc16_t crc16;

            uint8_t data_length = data_size;
            uint8_t corrected_data_length = data_length + (data_length % 2); // data length should be even for crc16

            uint8_t crc_buffer[corrected_data_length];
            memset(crc_buffer, 0, corrected_data_length);
            memcpy(crc_buffer, this->data(), data_length);

            data_length = corrected_data_length;

            uint32_t crc_tmp = 0;
            uint16_t *p_u16 = (uint16_t *)crc_buffer;
            while (data_length > 0)
            {
                crc_tmp += *p_u16;
                p_u16++;
                data_length -= 2;
            }
            crc_tmp = (crc_tmp >> 16) + (crc_tmp & 0xFFFF);
            crc_tmp = ~crc_tmp;

            crc16.crc16 = crc_tmp & 0xFFFF;
            return crc16;
        }

        FrameState Frame::_set_frame_state(FrameState state)
        {
            _state = state;
            return this->_state;
        }

        std::string Frame::_dump_data(const uint8_t *data, uint8_t data_length)
        {
            if (data == nullptr || data_length == 0)
                return "";

            uint8_t counter = 0;
            std::stringstream ss;
            ss << std::hex << std::uppercase;
            while (counter < data_length)
            {
                ss << std::setfill('0') << std::setw(2) << (int)*data;
                counter++;
                data++;
                if (counter < data_length)
                    ss << " ";
            }
            return ss.str();
        }

        FrameType Frame::get_frame_type() const
        {
            return (this->_is_header_loaded()) ? (FrameType)this->get_value(Frame::OFFSET_FRAME_TYPE) : (FrameType)0;
        }

        Frame &Frame::set_frame_type(FrameType frame_type)
        {
            if (this->_is_header_loaded())
                this->set_value(Frame::OFFSET_FRAME_TYPE, frame_type);

            return *this;
        }

        uint8_t Frame::get_body_length() const
        {
            return (this->_is_header_loaded()) ? this->get_value(Frame::OFFSET_BODY_LENGTH) : 0;
        }

        Frame &Frame::set_body_length(uint8_t body_length)
        {
            if (this->_is_header_loaded())
                this->set_value(Frame::OFFSET_BODY_LENGTH, body_length);

            return *this;
        }

        FrameDirection Frame::get_frame_dir() const
        {
            return (this->_is_header_loaded()) ? (FrameDirection)this->get_value(Frame::OFFSET_FRAME_DIRECTION) : (FrameDirection)0;
        }

        Frame &Frame::set_frame_dir(FrameDirection frame_direction)
        {
            if (this->_is_header_loaded())
                this->set_value(Frame::OFFSET_FRAME_DIRECTION, frame_direction);

            return *this;
        }

        Frame &Frame::set_frame_time(uint32_t time)
        {
            this->_frame_time = time;
            return *this;
        }

        Frame &Frame::clear()
        {
            this->_data.clear();
            this->_frame_time = 0;
            this->update_frame_state();
            return *this;
        }

        bool Frame::send(UARTComponent &uart)
        {
            uart.write_array(this->data(), this->size());
            ESP_LOGD(TAG, "%s", this->to_string(true).c_str());

            return true;
        }

        FrameState Frame::load(UARTComponent &uart)
        {
            if (!this->has_frame_state(FRAME_STATE_PARTIALLY_LOADED))
                this->clear();

            if (uart.available() == 0)
                return this->get_frame_state();

            uint8_t data_byte = 0;
            if (this->has_frame_state(FRAME_STATE_BLANK))
            {
                while (uart.available() &&
                       this->has_frame_state(FRAME_STATE_BLANK))
                {
                    if (!uart.read_byte(&data_byte))
                    {
                        ESP_LOGW(TAG, "uart read error");
                        break;
                    }

                    if (data_byte == this->get_start_byte())
                    {
                        this->append_data(data_byte);
                        this->update_frame_state();
                    }
                }
            }

            while (uart.available() &&
                   this->has_frame_state(FRAME_STATE_PARTIALLY_LOADED))
            {
                this->update_frame_state();
                if (this->has_frame_state(FRAME_STATE_OK))
                    break;

                if (this->has_frame_state(FRAME_STATE_ERROR))
                {
                    ESP_LOGW(TAG, "Broken frame received: %s", this->to_string(true).c_str());
                    break;
                }

                if (!uart.read_byte(&data_byte))
                {
                    ESP_LOGW(TAG, "UART read error");
                    break;
                }

                this->append_data(data_byte);
            }

            return this->update_frame_state();
        }

        Frame &Frame::append_data(uint8_t data, bool update_state)
        {
            this->_data.insert(this->_data.end(), data);
            if (update_state)
                this->update_frame_state();
            return *this;
        }

        Frame &Frame::append_data(const uint8_t data, const uint8_t count, bool update_state)
        {
            this->_data.insert(this->_data.end(), count, data);
            if (update_state)
                this->update_frame_state();
            return *this;
        }

        Frame &Frame::append_data(std::vector<uint8_t> data, bool update_state)
        {
            this->_data.insert(this->_data.end(), data.begin(), data.end());
            if (update_state)
                this->update_frame_state();
            return *this;
        }

        Frame &Frame::append_data(const uint8_t *data, uint8_t data_length, bool update_state)
        {
            if (data != nullptr && data_length != 0)
                std::copy(data, data + data_length, std::back_inserter(this->_data));

            if (update_state)
                this->update_frame_state();
            return *this;
        }

        Frame &Frame::trim_data(uint8_t first_element_index)
        {
            if (first_element_index < this->size())
            {
                this->_data.erase(this->_data.begin() + first_element_index, this->_data.end());
            }
            return *this;
        }

        Frame &Frame::update_crc(bool update_state)
        {
            if (!this->_is_header_loaded())
                return *this;

            uint8_t expected_frame_size = Frame::FRAME_HEADER_SIZE + this->get_body_length() + sizeof(crc16_t);
            if (this->size() < expected_frame_size - 2 ||
                this->size() > expected_frame_size)
                return *this;

            if (this->size() > expected_frame_size)
            {
                this->_data.erase(this->_data.begin() + expected_frame_size, this->_data.end());
            }
            else
            {
                this->_data.insert(this->_data.end(), expected_frame_size - this->size(), 0x00);
            }

            crc16_t crc = this->_calc_crc(this->size() - sizeof(crc16_t));
            this->_data.erase(this->_data.end() - 2, this->_data.end());
            this->_data.insert(this->_data.end(), {crc.crc[0], crc.crc[1]});

            if (update_state)
                this->update_frame_state();
            return *this;
        }

        bool Frame::is_valid_crc() const
        {
            if (this->size() < 2)
                return false;

            crc16_t crc;
            memcpy(&crc, &(this->_data.rbegin()[1]), 2);
            return this->_calc_crc(this->size() - 2).crc16 == crc.crc16;
        }

        bool Frame::get_bit(uint8_t data_index, uint8_t bit_index) const
        {
            if (bit_index > 7)
                return false;

            return get_value(data_index, (1 << bit_index)) >> bit_index == 1;
        }

        Frame &Frame::set_bit(uint8_t data_index, uint8_t bit_index, bool value)
        {
            if (bit_index > 7)
                return *this;

            this->set_value(data_index, (value << bit_index), (1 << bit_index));
            return *this;
        }

        uint8_t Frame::get_value(uint8_t index, uint8_t mask, uint8_t shift) const
        {
            if (index >= this->size())
                return 0;

            return (this->_data[index] & mask) >> shift;
        }

        Frame &Frame::set_value(uint8_t index, uint8_t value, uint8_t mask, uint8_t shift)
        {
            if (index >= this->size())
                return *this;

            this->_data[index] &= ~mask;
            this->_data[index] |= (value << shift) & mask;
            return *this;
        }

        bool Frame::get_crc(uint16_t &crc16) const
        {
            if (this->size() < 2)
                return false;

            memcpy(&crc16, &(this->_data.rbegin()[1]), 2);
            return true;
        }

        bool Frame::get_crc(uint8_t &crc16_1, uint8_t &crc16_2) const
        {
            if (this->size() < 2)
                return false;

            crc16_1 = this->_data.rbegin()[1];
            crc16_2 = this->_data.rbegin()[0];
            return true;
        }

        FrameState Frame::update_frame_state()
        {
            this->_state = FRAME_STATE_ERROR;
            if (this->size() == 0)
                return this->_set_frame_state(FRAME_STATE_BLANK);

            if (this->_data[0] != this->get_start_byte())
                return this->_set_frame_state(FRAME_STATE_ERROR);

            if (this->size() < Frame::FRAME_HEADER_SIZE)
                return this->_set_frame_state(FRAME_STATE_PARTIALLY_LOADED);

            if (this->size() >= Frame::FRAME_HEADER_SIZE)
            {
                if (this->size() < Frame::FRAME_HEADER_SIZE + this->get_body_length() + sizeof(crc16_t))
                    return this->_set_frame_state(FRAME_STATE_PARTIALLY_LOADED);

                if (this->size() > Frame::FRAME_HEADER_SIZE + this->get_body_length() + sizeof(crc16_t))
                    return this->_set_frame_state(FRAME_STATE_ERROR);

                if (this->size() == Frame::FRAME_HEADER_SIZE + this->get_body_length() + sizeof(crc16_t))
                {
                    return this->_set_frame_state(this->is_valid_crc() ? FRAME_STATE_OK : FRAME_STATE_ERROR);
                }
            }
            return this->_state;
        }

        std::string Frame::to_string(bool show_time) const
        {
            std::stringstream ss;
            if (show_time)
                ss << std::setfill('0') << std::setw(10) << _frame_time << ": ";

            if (this->has_frame_state(FRAME_STATE_OK))
            {
                ss << this->direction_to_string()
                   << "[" << _dump_data(this->data(), Frame::FRAME_HEADER_SIZE) << "] "
                   << _dump_data(this->data() + Frame::FRAME_HEADER_SIZE, this->get_body_length()) << ((this->get_body_length() != 0) ? " " : "")
                   << "[" << _dump_data(this->data() + Frame::FRAME_HEADER_SIZE + this->get_body_length(), sizeof(crc16_t)) << "]";
            }
            else
            {
                ss << "[--] " << _dump_data(this->data(), this->size());
            }

            return ss.str();
        }

        std::string Frame::state_to_string() const
        {
            switch (this->get_frame_state())
            {
            case FRAME_STATE_BLANK:
                return "blank";

            case FRAME_STATE_ERROR:
                return "error";

            case FRAME_STATE_PARTIALLY_LOADED:
                return "partially loaded";

            case FRAME_STATE_OK:
                return "ok";

            default:
                return "unknown";
            }
        }

        std::string Frame::type_to_string() const
        {
            switch (this->get_frame_type())
            {
            case FRAME_TYPE_COMMAND:
                return "command";

            case FRAME_TYPE_INIT:
                return "init";

            case FRAME_TYPE_PING:
                return "ping";

            case FRAME_TYPE_RESPONSE:
                return "response";

            case FRAME_TYPE_STRANGE:
                return "strange";

            default:
                return "unknown";
            }
        }

        std::string Frame::direction_to_string() const
        {
            return (this->get_frame_dir() == FRAME_DIR_TO_AC) ? "[=>] " : "[<=] ";
        }

    } // namespace aux_ac
} // namespace esphome
