#include "aux_uart.h"

namespace esphome
{
    namespace aux_ac
    {
        void AuxUart::send_frame(const std::vector<uint8_t> &frame)
        {
            this->write_array(frame);
        }

        bool AuxUart::read_frame(std::vector<uint8_t> &frame)
        {
            // Check if enough data is available
            if (this->available() < 3)
                return false;

            // Peek at the first 3 bytes to determine the frame length
            uint8_t header[3];
            if (!this->peek_array(header, 3))
                return false;

            // Validate start byte
            if (header[0] != 0xAA)
            {
                // Invalid start byte, discard one byte and return false
                uint8_t discard;
                this->read_byte(&discard);
                return false;
            }

            // Determine frame length from the second byte
            size_t frame_length = header[1];
            if (frame_length < 3 || frame_length > AC_BUFFER_SIZE)
            {
                // Invalid length, discard one byte and return false
                uint8_t discard;
                this->read_byte(&discard);
                return false;
            }

            // Check if the full frame is available
            if (this->available() < frame_length)
                return false;

            // Read the full frame into the internal buffer
            if (!this->read_array(this->_data, frame_length))
                return false;

            // Validate checksum
            uint8_t checksum = 0;
            for (size_t i = 0; i < frame_length - 1; i++)
            {
                checksum += this->_data[i];
            }
            if (checksum != this->_data[frame_length - 1])
            {
                // Invalid checksum, discard the frame and return false
                return false;
            }

            // Copy the valid frame to the output vector
            frame.assign(this->_data, this->_data + frame_length);
            return true;
        }

    } // namespace aux_ac
} // namespace esphome