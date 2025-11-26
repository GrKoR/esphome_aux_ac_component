#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"

#define AC_BUFFER_SIZE 35

using namespace esphome::uart;

namespace esphome
{
    namespace aux_ac
    {
        class AuxUart : public UARTDevice
        {
        public:
            AuxUart() = delete;
            explicit AuxUart(UARTComponent *parent) : UARTDevice(parent) {}
            ~AuxUart() = default;
            
            void send_frame(const std::vector<uint8_t> &frame);
            bool read_frame(std::vector<uint8_t> &frame);

        protected:
            // Internal buffer for incoming data
            uint8_t _data[AC_BUFFER_SIZE];
        };

    } // namespace aux_ac
} // namespace esphome