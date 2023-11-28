#pragma once

#include "frame_processor.h"
#include <map>
#include <list>

namespace esphome
{
    namespace aux_airconditioner
    {
        class AirCon;
        class FrameProcessorInterface;
        class Frame;

        class FrameProcessorManager
        {
        protected:
            AirCon *_aircon = nullptr;
            std::map<uint8_t, std::list<FrameProcessorInterface *>> _processor_map;
            std::list<FrameProcessorInterface *> _processors;
            void _update_map();

        public:
            FrameProcessorManager();
            ~FrameProcessorManager() { this->delete_all_processors(); }

            void set_aircon(AirCon &aircon) { _aircon = &aircon; }
            void add_frame_processor(FrameProcessorInterface *frame_processor);
            void delete_all_processors();
            void process_frame(Frame &frame);
        };

    } // namespace aux_airconditioner
} // namespace esphome