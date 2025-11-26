#pragma once

#include <stdint.h>
#include <string.h> // for memcpy and memset, move to .cpp later if needed

namespace esphome
{
    namespace aux_ac
    {

        class AuxFrame
        {
        private:
            // CRC of the AUX frame
            // https://github.com/GrKoR/AUX_HVAC_Protocol#packet_crc
            union frame_crc_t
            {
                uint16_t crc16;
                uint8_t crc[2];
            };

            // frame header
            // https://github.com/GrKoR/AUX_HVAC_Protocol#packet_header
            struct frame_header_t
            {
                uint8_t startByte;
                uint8_t _unknown1;
                uint8_t frameType;
                uint8_t wifi;
                uint8_t pingAnswer;
                uint8_t _unknown2;
                uint8_t bodyLength;
                uint8_t _unknown3;
            };

            uint8_t *_rawData = nullptr;
            frame_header_t *_header = nullptr;
            uint8_t *_body = nullptr;
            frame_crc_t *_crc = nullptr;

            bool _isValid = false;

            uint8_t const AC_PACKET_START_BYTE = 0xBB;
            uint8_t const AC_HEADER_SIZE = 8;
            uint8_t const AC_BODY_LENGTH_OFFSET = 6;

            uint8_t const AC_BUFFER_SIZE = 35; // TODO: integrate it with aux_uart.h

            uint16_t _CRC16(uint8_t *data, uint8_t len)
            {
                uint32_t crc = 0;

                uint8_t _crcBuffer[AC_BUFFER_SIZE];
                memset(_crcBuffer, 0, AC_BUFFER_SIZE);
                memcpy(_crcBuffer, data, len);

                if ((len % 2) == 1)
                    len++;

                uint32_t word = 0;
                for (uint8_t i = 0; i < len; i += 2)
                {
                    word = (_crcBuffer[i] << 8) + _crcBuffer[i + 1];
                    crc += word;
                }
                crc = (crc >> 16) + (crc & 0xFFFF);
                crc = ~crc;

                return crc & 0xFFFF;
            }

            bool _checkCRC()
            {
                frame_crc_t crc;
                crc.crc16 = _CRC16(this->_rawData, AC_HEADER_SIZE + this->_rawData[AC_BODY_LENGTH_OFFSET]);

                return ((this->_crc->crc[0] == crc.crc[1]) && (this->_crc->crc[1] == crc.crc[0]));
            }

            void _checkFrame()
            {
                this->_isValid = false;
                if (this->_rawData == nullptr)
                    return;

                if (this->_header->startByte != AC_PACKET_START_BYTE)
                    return;

                if (!this->_checkCRC())
                    return;

                this->_isValid = true;
            }

        public:
            AuxFrame() = default;
            ~AuxFrame() {};

            void set_data(uint8_t *data)
            {
                clearData();
                if (data == nullptr)
                    return;

                this->_rawData = data;
                this->_header = (frame_header_t *)this->_rawData;
                this->_crc = (frame_crc_t *)(this->_rawData + AC_HEADER_SIZE + this->_header->bodyLength);
                if (this->_header->bodyLength > 0)
                    this->_body = this->_rawData + AC_HEADER_SIZE;
                else
                    this->_body = nullptr;
                this->_checkFrame();
            }

            void clearData()
            {
                this->_rawData = nullptr;
                this->_header = nullptr;
                this->_crc = nullptr;
                this->_body = nullptr;
                this->_isValid = false;
            }

            bool isValid() const
            {
                return this->_isValid;
            };

            uint8_t frameSize() const
            {
                if (!this->isValid())
                    return 0;

                return AC_HEADER_SIZE + this->_header->bodyLength + 2;
            }

            uint8_t bodyLength() const
            {
                if (!this->isValid())
                    return 0;

                return this->_header->bodyLength;
            }
        };

    } // namespace aux_ac

} // namespace esphome
