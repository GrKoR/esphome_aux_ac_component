#pragma once

#include <map>
#include <list>
#include "esphome.h"
#include "esphome/components/climate/climate.h"
#include "aircon_common.h"
#include "frame_constants.h"

namespace esphome
{
    namespace aux_airconditioner
    {

        using esphome::climate::ClimateAction;
        using esphome::climate::ClimateFanMode;
        using esphome::climate::ClimateMode;
        using esphome::climate::ClimatePreset;
        using esphome::climate::ClimateSwingMode;

        class AirCon;
        class Frame;

        //*********************************************************************************************
        class FrameProcessorInterface
        {
        protected:
            virtual void _specific_process(const Frame &frame, AirCon &aircon) const = 0;

        public:
            virtual ~FrameProcessorInterface() = default;
            virtual bool applicable(const Frame &frame) const = 0;
            virtual FrameType get_applicable_frame_type() const = 0;
            void process(const Frame &frame, AirCon &aircon) const;
        };

        //*********************************************************************************************
        class FrameProcessorPing : public FrameProcessorInterface
        {
        protected:
            void _specific_process(const Frame &frame, AirCon &aircon) const override;

        public:
            FrameProcessorPing() = default;
            bool applicable(const Frame &frame) const override;
            FrameType get_applicable_frame_type() const override;
        };

        //*********************************************************************************************
        class FrameProcessorResponse01 : public FrameProcessorInterface
        {
        protected:
            void _specific_process(const Frame &frame, AirCon &aircon) const override;

        public:
            FrameProcessorResponse01() = default;
            bool applicable(const Frame &frame) const override;
            FrameType get_applicable_frame_type() const override;
        };

        //*********************************************************************************************
        class FrameProcessorResponse11 : public FrameProcessorInterface
        {
        protected:
            ClimateMode _power_and_mode_to_climate_mode(bool power_on, ac_mode mode) const;
            void _specific_process(const Frame &frame, AirCon &aircon) const override;

        public:
            FrameProcessorResponse11() = default;
            bool applicable(const Frame &frame) const override;
            FrameType get_applicable_frame_type() const override;
        };

        //*********************************************************************************************
        class FrameProcessorResponse2x : public FrameProcessorInterface
        {
        protected:
            void _specific_process(const Frame &frame, AirCon &aircon) const override;

        public:
            FrameProcessorResponse2x() = default;
            bool applicable(const Frame &frame) const override;
            FrameType get_applicable_frame_type() const override;
        };

    } // namespace aux_airconditioner
} // namespace esphome