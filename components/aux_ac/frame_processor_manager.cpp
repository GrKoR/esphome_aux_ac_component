#include "frame_processor_manager.h"
#include "frame.h"
#include <algorithm>

namespace esphome
{
    namespace aux_airconditioner
    {

        void FrameProcessorManager::_update_map()
        {
            _processor_map.clear();
            for (FrameProcessorInterface *processor : _processors)
            {
                auto it = std::find(_processor_map[processor->get_applicable_frame_type()].begin(), _processor_map[processor->get_applicable_frame_type()].end(), processor);
                if (it == _processor_map[processor->get_applicable_frame_type()].end())
                    _processor_map[processor->get_applicable_frame_type()].push_back(processor);
            }
        }

        FrameProcessorManager::FrameProcessorManager()
        {
            _processors.clear();
            _processors.push_back(new FrameProcessorPing);
            _processors.push_back(new FrameProcessorResponse01);
            _processors.push_back(new FrameProcessorResponse11);
            _processors.push_back(new FrameProcessorResponse2x);

            this->_update_map();
        }

        void FrameProcessorManager::add_frame_processor(FrameProcessorInterface *frame_processor)
        {
            if (frame_processor == nullptr)
                return;

            _processors.push_back(frame_processor);
            this->_update_map();
        }

        void FrameProcessorManager::delete_all_processors()
        {
            while (!_processors.empty())
            {
                delete _processors.front();
                _processors.pop_front();
            }
            _processor_map.clear();
        }

        void FrameProcessorManager::process_frame(Frame &frame)
        {
            auto processor_it = _processor_map.find(frame.get_frame_type());
            if (processor_it == _processor_map.end())
            {
                ESP_LOGW(TAG, "No processor for frame type 0x%02X (%s). Frame: %s", frame.get_frame_type(), frame.type_to_string().c_str(), frame.to_string().c_str());
                return;
            }

            // check if list of processors is empty
            if (processor_it->second.size() == 0)
                return;

            for (FrameProcessorInterface *processor : processor_it->second)
            {
                if (processor->applicable(frame))
                {
                    processor->process(frame, *_aircon);
                }
            }
        }

    } // namespace aux_airconditioner
} // namespace esphome