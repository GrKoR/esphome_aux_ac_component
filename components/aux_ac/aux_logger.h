#pragma once

#include <stdint.h>
#include "esphome/core/log.h"

namespace esphome
{
    namespace aux_ac
    {
        using packet_t = uint8_t; // TODO: Replace with actual packet_t definition

        void debugMsg(const char *msg, uint8_t dbgLevel, unsigned int line = 0, ...);
        void debugPrintPacket(packet_t *packet, uint8_t dbgLevel = ESPHOME_LOG_LEVEL_DEBUG, unsigned int line = __LINE__);
    } // namespace aux_ac
} // namespace esphome