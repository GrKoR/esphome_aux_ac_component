#pragma once

#include "aircon.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"

namespace esphome
{
    namespace aux_airconditioner
    {
        class AirCon;

        // **************************************** DISPLAY ACTIONS ****************************************
        template <typename... Ts>
        class AirConDisplayOffAction : public Action<Ts...>
        {
        public:
            explicit AirConDisplayOffAction(AirCon *ac) : ac_(ac) {}

            void play(Ts... x) override { this->ac_->action_display_off(); }

        protected:
            AirCon *ac_;
        };

        template <typename... Ts>
        class AirConDisplayOnAction : public Action<Ts...>
        {
        public:
            explicit AirConDisplayOnAction(AirCon *ac) : ac_(ac) {}

            void play(Ts... x) override { this->ac_->action_display_on(); }

        protected:
            AirCon *ac_;
        };

        // **************************************** VERTICAL LOUVER ACTIONS ****************************************
        template <typename... Ts>
        class AirConVLouverSwingAction : public Action<Ts...>
        {
        public:
            explicit AirConVLouverSwingAction(AirCon *ac) : ac_(ac) {}

            void play(Ts... x) override { this->ac_->action_set_vlouver_swing(); }

        protected:
            AirCon *ac_;
        };

        template <typename... Ts>
        class AirConVLouverStopAction : public Action<Ts...>
        {
        public:
            explicit AirConVLouverStopAction(AirCon *ac) : ac_(ac) {}

            void play(Ts... x) override { this->ac_->action_set_vlouver_stop(); }

        protected:
            AirCon *ac_;
        };

        template <typename... Ts>
        class AirConVLouverTopAction : public Action<Ts...>
        {
        public:
            explicit AirConVLouverTopAction(AirCon *ac) : ac_(ac) {}

            void play(Ts... x) override { this->ac_->action_set_vlouver_top_position(); }

        protected:
            AirCon *ac_;
        };

        template <typename... Ts>
        class AirConVLouverMiddleAboveAction : public Action<Ts...>
        {
        public:
            explicit AirConVLouverMiddleAboveAction(AirCon *ac) : ac_(ac) {}

            void play(Ts... x) override { this->ac_->action_set_vlouver_middle_above_position(); }

        protected:
            AirCon *ac_;
        };

        template <typename... Ts>
        class AirConVLouverMiddleAction : public Action<Ts...>
        {
        public:
            explicit AirConVLouverMiddleAction(AirCon *ac) : ac_(ac) {}

            void play(Ts... x) override { this->ac_->action_set_vlouver_middle_position(); }

        protected:
            AirCon *ac_;
        };

        template <typename... Ts>
        class AirConVLouverMiddleBelowAction : public Action<Ts...>
        {
        public:
            explicit AirConVLouverMiddleBelowAction(AirCon *ac) : ac_(ac) {}

            void play(Ts... x) override { this->ac_->action_set_vlouver_middle_below_position(); }

        protected:
            AirCon *ac_;
        };

        template <typename... Ts>
        class AirConVLouverBottomAction : public Action<Ts...>
        {
        public:
            explicit AirConVLouverBottomAction(AirCon *ac) : ac_(ac) {}

            void play(Ts... x) override { this->ac_->action_set_vlouver_bottom(); }

        protected:
            AirCon *ac_;
        };

        template <typename... Ts>
        class AirConVLouverSetAction : public Action<Ts...>
        {
        public:
            AirConVLouverSetAction(AirCon *ac) : ac_(ac) {}
            TEMPLATABLE_VALUE(uint8_t, value);

            void play(Ts... x)
            {
                this->ac_->action_set_vlouver_position((vlouver_esphome_position_t)this->value_.value(x...));
            }

        protected:
            AirCon *ac_;
        };

        // **************************************** POWER LIMITATION ACTIONS ****************************************
        template <typename... Ts>
        class AirConPowerLimitationOffAction : public Action<Ts...>
        {
        public:
            explicit AirConPowerLimitationOffAction(AirCon *ac) : ac_(ac) {}

            void play(Ts... x) override { this->ac_->action_power_limitation_off(); }

        protected:
            AirCon *ac_;
        };

        template <typename... Ts>
        class AirConPowerLimitationOnAction : public Action<Ts...>
        {
        public:
            AirConPowerLimitationOnAction(AirCon *ac) : ac_(ac) {}
            TEMPLATABLE_VALUE(uint8_t, value);

            void play(Ts... x)
            {
                this->ac_->action_power_limitation_on(this->value_.value(x...));
            }

        protected:
            AirCon *ac_;
        };

    } // namespace aux_airconditioner
} // namespace esphome