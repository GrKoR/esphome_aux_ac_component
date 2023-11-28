#pragma once

#include <stdint.h>

namespace esphome
{
    namespace aux_airconditioner
    {

        enum FrameType : uint8_t
        {
            FRAME_TYPE_PING = 0x01,
            FRAME_TYPE_COMMAND = 0x06,
            FRAME_TYPE_RESPONSE = 0x07,
            FRAME_TYPE_INIT = 0x09,
            FRAME_TYPE_STRANGE = 0x0b,
        };

        enum FrameDirection : uint8_t
        {
            FRAME_DIR_TO_DONGLE = 0x00,
            FRAME_DIR_TO_AC = 0x80,
        };

        enum FrameState : uint8_t
        {
            FRAME_STATE_BLANK = 0x00,
            FRAME_STATE_PARTIALLY_LOADED = 0x01,
            FRAME_STATE_OK = 0x0F,
            FRAME_STATE_ERROR = 0xFF,
        };

    } // namespace aux_airconditioner
} // namespace esphome
